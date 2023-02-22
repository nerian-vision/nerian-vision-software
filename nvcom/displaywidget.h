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

#ifndef DISPLAYWIDGET_H
#define DISPLAYWIDGET_H

#include <QWidget>
#include <QPainter>
#include <opencv2/opencv.hpp>
#include <memory>
#include <mutex>
#include <visiontransfer/imageset.h>

class NVCom;

class DisplayWidget: public QWidget {
public:
    DisplayWidget(QWidget * parent = nullptr);

    void setNVCom(std::shared_ptr<NVCom> nvcom);
    void setDisplayFrame(const std::vector<cv::Mat_<cv::Vec3b>>& frames, int numFrames, bool resize);
    void setZoom(int percent);
    virtual void paintEvent(QPaintEvent *) override;

private:
    static constexpr int IMAGE_PADDING = 5;

    bool resizeWidget;
    int zoomPercent;

    int mouseX, mouseY;
    int mouseOnImageIndex;
    std::vector<double> zValues;

    std::shared_ptr<NVCom> nvcom;
    cv::Mat_<cv::Vec3b> displayFrames[visiontransfer::ImageSet::MAX_SUPPORTED_IMAGES];
    int numDisplayFrames;
    QPainter painter;
    QFont labelFont;

    void mouseMoveEvent(QMouseEvent *event) override;
    void drawMouseLabel();
};

#endif
