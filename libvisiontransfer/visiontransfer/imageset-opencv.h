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

#ifndef VISIONTRANSFER_IMAGESET_OPENCV_H
#define VISIONTRANSFER_IMAGESET_OPENCV_H

#ifdef CV_MAJOR_VERSION

namespace visiontransfer {

/*
 * OpenCV-specific implementations that need to be inlined in order to avoid
 * dependencies for projects that do not make use of OpenCV
 */

inline void ImageSet::toOpenCVImage(int imageNumber, cv::Mat& dest, bool convertRgbToBgr) {
    assert(imageNumber >= 0 && imageNumber < getNumberOfImages());

    switch(formats[imageNumber]) {
        case ImageSet::FORMAT_12_BIT_MONO:
            dest= cv::Mat_<unsigned short>(height, width,
                reinterpret_cast<unsigned short*>(data[imageNumber]), rowStride[imageNumber]);
            break;
        case ImageSet::FORMAT_8_BIT_MONO:
            dest = cv::Mat_<unsigned char>(height, width,
                data[imageNumber], rowStride[imageNumber]);
            break;
        case ImageSet::FORMAT_8_BIT_RGB:
            dest = cv::Mat_<cv::Vec3b>(height, width,
                reinterpret_cast<cv::Vec3b*>(data[imageNumber]), rowStride[imageNumber]);
            if(convertRgbToBgr) {
                cv::cvtColor(dest, dest, cv::COLOR_RGB2BGR);
            }
            break;
    }
}

} // namespace

#endif
#endif
