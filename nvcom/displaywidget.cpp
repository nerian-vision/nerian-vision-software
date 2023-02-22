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

#include "displaywidget.h"
#include "nvcom.h"

#include <cmath>

#include <QMouseEvent>
#include <QTextDocument>

constexpr int DisplayWidget::IMAGE_PADDING;

DisplayWidget::DisplayWidget(QWidget * parent): QWidget(parent),
        resizeWidget(false), zoomPercent(100),
        mouseX(-1), mouseY(-1), mouseOnImageIndex(-1), numDisplayFrames(0),
        labelFont("Courier New", 12, QFont::Bold) {

    labelFont.setStyleHint(QFont::Monospace);
    labelFont.setWeight(2);
    setMouseTracking(true);

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setMinimumSize(320,240);
}

void DisplayWidget::setNVCom(std::shared_ptr<NVCom> nvcom) {
    this->nvcom = nvcom;
}

void DisplayWidget::setDisplayFrame(const std::vector<cv::Mat_<cv::Vec3b>>& frames, int numFrames, bool resize) {
    for (int i=0; i<numFrames; ++i) {
        displayFrames[i] = frames[i];
    }
    numDisplayFrames = numFrames;
    if(resize) {
        resizeWidget = true;
    }
}

void DisplayWidget::setZoom(int percent) {
    zoomPercent = percent;
    resizeWidget = true;

    // Adapt label font to correct for surface scaling
    int pts = (int) ((12*100)/zoomPercent);
    labelFont = QFont("Courier New", pts, QFont::Bold);
    labelFont.setStyleHint(QFont::Monospace);
    labelFont.setWeight(2);
}

void DisplayWidget::paintEvent(QPaintEvent *) {
    if(nvcom == nullptr) {
        // Should never happen
        return;
    }

    std::unique_lock<std::mutex> lock(nvcom->getDisplayMutex());
    if(numDisplayFrames==0 || displayFrames[0].data==nullptr) {
        return; // No frame available yet
    }

    const double scale = zoomPercent/100.0;

    // Resize widget if necessary
    if(resizeWidget) {
        int width = 0;
        for(int i=0; i<numDisplayFrames; i++) {
            if(i> 0) {
                width += IMAGE_PADDING;
            }
            width += displayFrames[i].cols*scale;
        }

        setFixedSize(width, displayFrames[0].rows*scale);
        resizeWidget = false;
    }

    painter.begin(this);
    painter.scale(scale, scale);

    // Display the received images
    int xpos = 0;
    for (int i=0; i<numDisplayFrames; ++i) {
        QImage img(displayFrames[i].data, displayFrames[i].cols, displayFrames[i].rows,
            displayFrames[i].step[0], QImage::Format_RGB888);
        painter.drawImage(xpos, 0, img);
        xpos += displayFrames[i].cols + IMAGE_PADDING/scale;
    }
    drawMouseLabel();

    painter.end();
}

void DisplayWidget::drawMouseLabel() {
    if(mouseX < 0 || mouseY < 0 || mouseOnImageIndex < 0) {
        return; // No position to draw
    }

    cv::Point3f point = nvcom->getDisparityMapPoint(mouseX, mouseY);
    if(point == cv::Point3f(0, 0, 0)) {
        // No point available
        return;
    }

    // Calculate standard deviation for Z values for current pixel
    bool hasSD = false;
    double sdZ = 0.0;
    zValues.push_back(point.z);
    if (zValues.size() > 10) { // Must linger on a pixel for 10 frames
        if (zValues.size() > 500) { // Prevent time series from getting too long
            zValues.erase(zValues.begin(), zValues.begin() + 250);
        }
        double zMean = 0.0;
        double sumSqrErr = 0.0;
        int validPts = 0;
        for (auto zval: zValues) {
            if (std::isfinite(zval)) {
                zMean += zval;
                validPts++;
            }
        }
        if (validPts > 5) {
            zMean /= validPts;
            for (auto zval: zValues) {
                if (std::isfinite(zval)) {
                    sumSqrErr += (zval-zMean) * (zval-zMean);
                }
            }
            sdZ = std::sqrt(1.0 * sumSqrErr / validPts);
            hasSD = true;
        }
    }

    char labelStr[100];
    char labelMarkup[300];

    int pts = (int) ((12*100)/zoomPercent); // font size for HTML markup
    if (hasSD) {
        // Generate text for bounding box (using suitable placeholders for HTML characters / sections)
        snprintf(labelStr, sizeof(labelStr), "x = %7.3lf m\ny = %7.3lf m\nz = %7.3lf m\nSz = %0.3lf m",
            point.x, point.y, point.z, sdZ);
        // Generate the actual markup (the HTML renderer does not report a bounding box itself)
        snprintf(labelMarkup, sizeof(labelMarkup), "<span style=\"font-family:Courier New,Courier,fixed;font-weight:bold;font-size:%dpt\">x<sub>&nbsp;</sub> = %7.3lf m<br/>y<sub>&nbsp;</sub> = %7.3lf m<br/>z<sub>&nbsp;</sub> = %7.3lf m<br/>&sigma;<sub>z</sub> = %0.3lf m</span>",
            pts, point.x, point.y, point.z, sdZ);
    } else {
        snprintf(labelStr, sizeof(labelStr), "x = %7.3lf m\ny = %7.3lf m\nz = %7.3lf m",
            point.x, point.y, point.z);
        snprintf(labelMarkup, sizeof(labelMarkup), "<span style=\"font-family:Courier New,Courier,fixed;font-weight:bold;font-size:%dpt\">x<sub>&nbsp;</sub> = %7.3lf m<br/>y<sub>&nbsp;</sub> = %7.3lf m<br/>z<sub>&nbsp;</sub> = %7.3lf m</span>",
            pts, point.x, point.y, point.z);
    }

    const double scale = zoomPercent/100.0;

    int drawX = mouseX + mouseOnImageIndex * (displayFrames[0].cols + IMAGE_PADDING/scale);
    int drawY = mouseY;

    painter.setPen(QPen(QColor::fromRgb(255, 0, 0), 2));
    painter.drawEllipse(QPointF(drawX, mouseY), 6, 6);

    int offsetX = 25, offsetY = 0;
    painter.setPen(Qt::black);
    painter.setFont(labelFont);
    QRectF boundingBox;
    painter.drawText(QRectF(drawX + offsetX, drawY + offsetY, 1000, 1000), Qt::AlignLeft | Qt::TextDontPrint,
        labelStr, &boundingBox);

    if((mouseOnImageIndex==0 && boundingBox.x() + boundingBox.width() > displayFrames[0].cols) ||
            boundingBox.x() + boundingBox.width() > width()/scale) {
        // Display label to the left if there isn't enough space on the right
        boundingBox.translate(-boundingBox.width() - 2*offsetX, 0);
        drawX -= boundingBox.width();
        offsetX = -offsetX;
    }

    if(boundingBox.y() + boundingBox.height() > height()/scale) {
        // Display label on top if there isn't enough space below
        boundingBox.translate(0, -boundingBox.height() - 2*offsetY);
        drawY -= boundingBox.height();
        offsetY = -offsetY;
    }

    const int border = 6 * 1.0/scale;
    painter.fillRect(boundingBox.x() - border, boundingBox.y() - border,
        boundingBox.width() + 2*border, boundingBox.height() + 2*border, QColor::fromRgbF(1.0, 1.0, 1.0, 0.5));
    QTextDocument td;
    td.setHtml(QString::fromUtf8(labelMarkup));
    painter.save();
    painter.translate(QPointF(drawX + offsetX - 2, drawY + offsetY - pts/4));
    td.drawContents(&painter);
    painter.restore();
}


void DisplayWidget::mouseMoveEvent(QMouseEvent *event) {
    if(event != nullptr) {
        double scale = zoomPercent/100.0;
        int x =      ((int) event->x() % (int) (displayFrames[0].cols*scale + IMAGE_PADDING)) / scale;
        int rawIdx = (int) event->x() / (int) (displayFrames[0].cols*scale + IMAGE_PADDING);
        if (rawIdx >=0 && rawIdx < numDisplayFrames) {
            mouseOnImageIndex = rawIdx;
        } else {
            mouseOnImageIndex = -1;
        }

        if (mouseOnImageIndex==2) {
            // Explicitly disable depth tooltip on right image in three-image mode
            mouseOnImageIndex = -1;
        }

        // Start time series anew for new pixel position
        zValues.clear();

        int y = event->y() / scale;

        if(rawIdx >=0 && rawIdx < numDisplayFrames) {
            mouseX = x;
            mouseY = y;
            repaint();
        }
    }
}
