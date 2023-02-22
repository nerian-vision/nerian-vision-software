/*******************************************************************************
 * Copyright (c) 2022 Nerian Vision GmbH
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *******************************************************************************/

#ifndef QTOPEN3DVISUALIZER_H
#define QTOPEN3DVISUALIZER_H

#ifndef WITH_OPEN3D
// Dummy implementation if Open3D is not available
#include <QLabel>
#include <functional>

namespace open3d {
    namespace geometry {
        class PointCloud;
    }
}


class QtOpen3DVisualizer: public QLabel {
public:
    QtOpen3DVisualizer(QWidget *parent)
        :QLabel("3D display is not available!\nPlease re-compile NVCom with Open3D support.", parent) {
        setAlignment(Qt::AlignCenter);
    }
    bool setDisplayCloud(std::shared_ptr<open3d::geometry::PointCloud> pointcloud, double fovHoriz){return false;}
    void setErrorCallback(const std::function<void()>& f) {}
};

#else
// Actual implementation
#include <open3d/Open3D.h>
#include <QOpenGLWidget>
#include <mutex>

class QtOpen3DVisualizer: public QOpenGLWidget, private open3d::visualization::Visualizer {
public:
    QtOpen3DVisualizer(QWidget *parent);
    bool setDisplayCloud(std::shared_ptr<open3d::geometry::PointCloud> pointcloud, double fovHoriz);
    void setErrorCallback(const std::function<void()>& f) {
        errorCallback = f;
    }

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    std::function<void()> errorCallback;
    std::shared_ptr<open3d::geometry::PointCloud> cloud;
    std::mutex cloudMutex;
    bool updateCloud;
    bool validCloud;
    open3d::visualization::ViewControl defaultView;

    int lastMouseX;
    int lastMouseY;
    double fov;
    bool initFailed;
    bool pendingUpdate;

    void updateView(bool resetExtrinsic);
};

#endif
#endif
