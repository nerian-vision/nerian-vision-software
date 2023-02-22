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

#ifdef WITH_OPEN3D

#define _USE_MATH_DEFINES
#include <cmath>

#include "qtopen3dvisualizer.h"
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <QMouseEvent>
#include <QMessageBox>

using namespace std::chrono;

#define ROTATION_CENTER             1.5
#define DEFAULT_POINT_SIZE          2

#define DEMO_ZOOM                   3
#define DEMO_FAR_LIMIT              50.0
#define DEMO_VIEWER_OFFST           0, 0, 0
#define DEMO_ROTATION_X_SPEED       0.41
#define DEMO_ROTATION_Y_SPEED       0.31
#define DEMO_ROTATION_X_AMPLITUDE   80
#define DEMO_ROTATION_Y_AMPLITUDE   35

QtOpen3DVisualizer::QtOpen3DVisualizer(QWidget *parent)
    :QOpenGLWidget(parent) {
    lastMouseX = 0;
    lastMouseY = 0;
    fov = 60 * M_PI / 180.0;
    updateCloud = false;
    initFailed = true; // Assume faild until executed
    validCloud = false;
    pendingUpdate = false;
}

void QtOpen3DVisualizer::initializeGL() {
    std::unique_lock<std::mutex> lock(cloudMutex);

    // Initialize Visualizer
    if(!InitOpenGL()) {
        // Failed
        return;
    }

    InitViewControl();
    view_control_ptr_->ChangeWindowSize(width(), height());
    updateView(true);

    InitRenderOption();
    render_option_ptr_->SetPointSize(DEFAULT_POINT_SIZE);
    render_option_ptr_->background_color_ = Eigen::Vector3d(0.25, 0.25, 0.25);

    is_initialized_ = true;

    // Add Geometry. Open3D doesn't like empty pointclouds, hence
    // we add some dummy points.
    cloud.reset(new open3d::geometry::PointCloud);
    cloud->points_ = std::vector<Eigen::Vector3d>({
            Eigen::Vector3d(0, 0, 0),
            Eigen::Vector3d(0, -1, 2*ROTATION_CENTER),
            Eigen::Vector3d(0, 1, 2*ROTATION_CENTER)
            });
    AddGeometry(cloud);
    UpdateGeometry();

    initFailed = false;
}

void QtOpen3DVisualizer::updateView(bool resetExtrinsic) {
    if(view_control_ptr_ == nullptr) {
        return;
    }

    double fPixel = view_control_ptr_->GetWindowWidth() / (2.0 * tan(fov/2));

    open3d::camera::PinholeCameraParameters cameraParameters;
    view_control_ptr_->ConvertToPinholeCameraParameters(cameraParameters);

    cameraParameters.intrinsic_.SetIntrinsics(
        view_control_ptr_->GetWindowWidth(),
        view_control_ptr_->GetWindowHeight(),
        fPixel, fPixel,
        (double)view_control_ptr_->GetWindowWidth()/2 - 0.5,
        (double)view_control_ptr_->GetWindowHeight()/2 - 0.5);

    if(resetExtrinsic) {
        cameraParameters.extrinsic_.setIdentity();
    }

    view_control_ptr_->ConvertFromPinholeCameraParameters(cameraParameters/*, true*/);

    view_control_ptr_->SetConstantZFar(1000.0);
    view_control_ptr_->SetConstantZNear(0.01);

#ifdef DEMO_MODE
    if(resetExtrinsic) {
        // Copying the view control object is dangerous if Open3D wasn't
        // built with the same compiler settings! Hence this is only
        // done for demo builds.
        defaultView = *view_control_ptr_;
    }
#endif
}

void QtOpen3DVisualizer::paintGL() {
    if(view_control_ptr_ == nullptr) {
        return; // Not yet initialized
    }

    std::unique_lock<std::mutex> lock(cloudMutex);
    if(updateCloud) {
        UpdateGeometry();
        updateCloud = false;
    }
    Render();

#ifdef DEMO_MODE
    if(validCloud) {
        static steady_clock::time_point startTime = steady_clock::now();
        long long microSecs = duration_cast<microseconds>(steady_clock::now() - startTime).count();

        double alpha = DEMO_ROTATION_X_AMPLITUDE*sin(microSecs/1.0e6 * DEMO_ROTATION_X_SPEED);
        double beta = DEMO_ROTATION_Y_AMPLITUDE*sin(microSecs/1.0e6 * DEMO_ROTATION_Y_SPEED);

        *view_control_ptr_ = defaultView;
        view_control_ptr_->Translate(DEMO_VIEWER_OFFST);
        view_control_ptr_->Scale(DEMO_ZOOM);
        view_control_ptr_->Rotate(alpha, beta);
        view_control_ptr_->SetConstantZFar(DEMO_FAR_LIMIT);
        update();
    }
#endif

    pendingUpdate = false;
}

void QtOpen3DVisualizer::resizeGL(int w, int h) {
    if(view_control_ptr_ != nullptr) {
        view_control_ptr_->ChangeWindowSize(w, h);
        updateView(false);
    }
}

void QtOpen3DVisualizer::mousePressEvent(QMouseEvent * event) {
    lastMouseX = event->globalX();
    lastMouseY = event->globalY();
}

void QtOpen3DVisualizer::mouseReleaseEvent(QMouseEvent * event) {
    lastMouseX = event->globalX();
    lastMouseY = event->globalY();
}

void QtOpen3DVisualizer::mouseMoveEvent(QMouseEvent *event) {
    if(view_control_ptr_ == nullptr) {
        return;
    }

    int dx = event->globalX() - lastMouseX;
    int dy = event->globalY() - lastMouseY;

    if(event->buttons() & Qt::LeftButton) {
        if(event->modifiers() & Qt::ControlModifier) {
            view_control_ptr_->Translate(dx, dy, event->x(), event->y());
        } else if (event->modifiers() & Qt::ShiftModifier) {
            view_control_ptr_->Roll(dx);
        } else if (event->modifiers() & Qt::AltModifier) {
            view_control_ptr_->CameraLocalRotate(
                    dx, dy, event->x(), event->y());
        } else {
            view_control_ptr_->Rotate(dx, dy, event->x(), event->y());
        }

        update();
    }
    if (event->buttons() & Qt::MiddleButton) {
        view_control_ptr_->Translate(dx, dy, event->y(), event->y());
        update();
    }

    lastMouseX = event->globalX();
    lastMouseY = event->globalY();
}

void QtOpen3DVisualizer::wheelEvent(QWheelEvent *event) {
    if(view_control_ptr_ == nullptr) {
        return;
    }

    if((event->modifiers() & Qt::AltModifier) || (event->modifiers() & Qt::ShiftModifier)) {
        // Change point-size
        int step = event->angleDelta().y()+event->angleDelta().x();
        int inc = 0;
        if(step > 0) {
            inc = 1;
        } else if(step < 0) {
            inc = -1;
        }

        render_option_ptr_->ChangePointSize(inc);
    } else {
        // Zoom
        view_control_ptr_->Scale(event->angleDelta().y()/60.0);
    }
    update();
}

bool QtOpen3DVisualizer::setDisplayCloud(std::shared_ptr<open3d::geometry::PointCloud> pointcloud, double fovHoriz) {
    if(pendingUpdate) {
        // Still haven't drawn the last frame
        return false;
    }

    std::unique_lock<std::mutex> lock(cloudMutex);
    if(cloud == nullptr || view_control_ptr_ == nullptr || initFailed) {
        return false;
    }

    *cloud = *pointcloud;

    updateCloud = true;
    if(fovHoriz != fov || !validCloud) {
        fov = fovHoriz;
        updateView(!validCloud);
        validCloud = true;
    }

    pendingUpdate = true;
    update();
    return true;
}

void QtOpen3DVisualizer::showEvent(QShowEvent *event) {
    if(initFailed || !isValid()) {
        initFailed = false;
        QMessageBox msgBox(QMessageBox::Critical, "NVCom Exception!", "Error initializing OpenGL!\n"
            "Please make sure that your graphics driver supports OpenGL 3.3 or later.");
        msgBox.exec();
        if(errorCallback != nullptr) {
            errorCallback();
        }
    }
}

#endif
