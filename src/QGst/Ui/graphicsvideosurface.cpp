/*
    Copyright (C) 2012 Collabora Ltd. <info@collabora.com>
      @author George Kiagiadakis <george.kiagiadakis@collabora.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "graphicsvideosurface_p.h"
#include "../elementfactory.h"
#include "../../QGlib/connect.h"

#ifndef QTGSTREAMER_UI_NO_OPENGL
# include <QtOpenGL/QGLWidget>
#endif

namespace QGst {
namespace Ui {

GraphicsVideoSurface::GraphicsVideoSurface(QGraphicsView *parent)
    : QObject(parent), d(new GraphicsVideoSurfacePrivate)
{
    d->view = parent;
}

GraphicsVideoSurface::~GraphicsVideoSurface()
{
    if (!d->videoSink.isNull()) {
        d->videoSink->setState(QGst::StateNull);
    }

    delete d;
}

bool GraphicsVideoSurface::setVideoSink(ElementPtr sink)
{
    if (!d->videoSink.isNull()) {
        QGlib::disconnect(d->videoSink, "update",
                const_cast<GraphicsVideoSurface*>(this),
                &GraphicsVideoSurface::onUpdate);
    }

    d->videoSink = sink;

    if (d->videoSink.isNull()) {
        return true;
    }

#ifndef QTGSTREAMER_UI_NO_OPENGL
    //if the viewport is a QGLWidget, try to set the glcontext property.
    if (d->view)
    {
        QGLWidget *glw = qobject_cast<QGLWidget*>(d->view->viewport());
        if (glw) {
            glw->makeCurrent();
            d->videoSink->setProperty("glcontext", (void*) QGLContext::currentContext());
            glw->doneCurrent();
        }
    }
#endif

    if (d->videoSink->setState(QGst::StateReady) != QGst::StateChangeSuccess) {
        qDebug("Failed to set qt video sink state to ready.");
        d->videoSink.clear();
        return false;
    }

    QGlib::connect(d->videoSink, "update",
            const_cast<GraphicsVideoSurface*>(this),
            &GraphicsVideoSurface::onUpdate);

    return true;
}

ElementPtr GraphicsVideoSurface::videoSink()
{
    if (d->videoSink.isNull()) {
        ElementPtr sink;

#ifndef QTGSTREAMER_UI_NO_OPENGL
        //if the viewport is a QGLWidget, try to use a qtglvideosink.
        if (d->view)
        {
            QGLWidget *glw = qobject_cast<QGLWidget*>(d->view->viewport());
            if (glw) {
                sink = QGst::ElementFactory::make(QTGLVIDEOSINK_NAME);
                if (sink.isNull()) {
                    qDebug("Failed to create qtglvideosink. Will try qtvideosink instead.");
                }
                else if (!setVideoSink(sink)) {
                    qDebug("Failed to use qtglvideosink. Will try qtvideosink instead.");
                }
            }
        }
#endif

        if (d->videoSink.isNull()) {
            sink = QGst::ElementFactory::make(QTVIDEOSINK_NAME);
            if (sink.isNull()) {
                qCritical("Failed to create qtvideosink. Make sure it is installed correctly");
                return ElementPtr();
            }
            setVideoSink(sink); // Might reset d->videoSink to null if state change fails
        }

    }

    return d->videoSink;
}

void GraphicsVideoSurface::onUpdate()
{
    Q_FOREACH(GraphicsVideoWidget *item, d->items) {
        item->update(item->rect());
    }
}

} // namespace Ui
} // namespace QGst
