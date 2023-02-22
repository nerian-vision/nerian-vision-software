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

#ifndef NVCOM_H
#define NVCOM_H

#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <chrono>

#include <visiontransfer/asynctransfer.h>
#include <visiontransfer/reconstruct3d.h>
#include "ratelimit.h"
#include "imagereader.h"
#include "colorcoder.h"
#include "settings.h"

namespace open3d {
    namespace geometry {
        class PointCloud;
    }
}

/*
 * All functionality of NVCom except the GUI
 */
class NVCom {
public:
    NVCom(const Settings& newSettings);
    ~NVCom();

    void terminate();
    void connect();

    unsigned int getWriteQueueSize() {return writeQueue.size();}
    void updateSettings(const Settings& newSettings);

    void captureSingleFrame() {captureNextFrame = true;}
    void setCaptureSequence(bool capture) {captureSequence = capture;}

    // Methods for setting callback functions
    void setFrame2DDisplayCallback(const std::function<void(int, int, const std::vector<cv::Mat_<cv::Vec3b>>& images,
            int imageCount, bool resize)>& f) {
        frame2DDisplayCallback = f;
    }
    void setFrame3DDisplayCallback(const std::function<void(std::shared_ptr<open3d::geometry::PointCloud> pointcloud, double fovHoriz)>& f) {
        frame3DDisplayCallback = f;
    }
    void setExceptionCallback(const std::function<void(const std::exception& ex)>& f) {
        exceptionCallback = f;
    }
    void setSendCompleteCallback(const std::function<void()>& f) {
        sendCompleteCallback = f;
    }
    void setConnectedCallback(const std::function<void()>& f) {
        connectedCallback = f;
    }
    void setDisconnectCallback(const std::function<void()>& f) {
        disconnectCallback = f;
    }

    // A mutex that should be used when accessing the display frames
    // outside the callback
    std::mutex& getDisplayMutex() {return displayMutex;}

    std::vector<int> getBitDepths();

    // Projects the given point of the disparity map to 3D
    cv::Point3f getDisparityMapPoint(int x, int y);

    int getNumDroppedFrames() {
        return asyncTrans->getNumDroppedFrames();
    }

    // Set whether the color coders should adapt to the observed disparity ranges
    inline void setAdaptiveColorScale(bool adaptive) {
        settings.adaptiveColorScale = adaptive; // TODO: remove
        needColorCoderReinit = true;
    }

private:
    enum ImageType {
        IMG_NONE,
        IMG_LEFT,
        IMG_RIGHT,
        IMG_THIRD_COLOR,
        IMG_DISPARITY_COLOR,
        IMG_DISPARITY_RAW,
        IMG_POINTCLOUD
    };

    Settings settings;

    volatile bool terminateThreads;
    volatile bool captureNextFrame;
    volatile bool captureSequence;
    int captureIndex;
    int sendSeqNum;
    int receivedSeqNum;
    int minDisparity;
    int maxDisparity;

    std::unique_ptr<ImageReader> imageReader;
    std::unique_ptr<RateLimit> frameRateLimit;
    std::unique_ptr<visiontransfer::AsyncTransfer> asyncTrans;
    std::queue<std::pair<std::string, cv::Mat> > writeQueue;
    std::unique_ptr<ColorCoder> redBlueCoder;
    std::unique_ptr<ColorCoder> rainbowCoder;
    visiontransfer::Reconstruct3D recon3d;

    cv::Size2i lastFrameSize;
    int lastImageCount;
    std::vector<int> lastFormats;
    std::vector<cv::Mat> receivedFrames;
    std::vector<cv::Mat_<cv::Vec3b>> convertedFrames;
    std::fstream timestampFile;
    visiontransfer::ImageSet lastImageSet;

    // Callback functions
    std::function<void(int, int, const std::vector<cv::Mat_<cv::Vec3b>>&, int, bool)> frame2DDisplayCallback;
    std::function<void(std::shared_ptr<open3d::geometry::PointCloud>, double)> frame3DDisplayCallback;
    std::function<void(const std::exception& ex)> exceptionCallback;
    std::function<void()> sendCompleteCallback;
    std::function<void()> connectedCallback;
    std::function<void()> disconnectCallback;

    // Variables for main loop thread
    std::thread mainLoopThread;
    std::mutex displayMutex;
    std::mutex imageSetMutex;

    // Variables for writing thread
    std::thread writingThread;
    std::mutex writingMutex;
    std::condition_variable writingCond;

    // Signal that the color coders must be reset (adaptive mode change)
    bool needColorCoderReinit;
    bool resetColorScale;

    void mainLoop();
    void writeLoop();

    void framePause();
    void scheduleWrite(const cv::Mat& frame, int index, ImageType type, std::chrono::system_clock::time_point timeStamp);
    void captureFrameIfRequested(const visiontransfer::ImageSet& imageSet, std::chrono::system_clock::time_point timeStamp);
    bool transmitFrame();
    bool receiveFrame(visiontransfer::ImageSet& imageSet);
    void convertFrame(const cv::Mat& src, cv::Mat_<cv::Vec3b>& dst, bool colorCode, int subpixFactor);
    void colorCodeAndDisplay(visiontransfer::ImageSet& imageSet);
    void display3D(visiontransfer::ImageSet& imageSet);
    void joinAllThreads();
    std::string getFileName(int index, ImageType type, bool color, std::chrono::system_clock::time_point timeStamp);
};

#endif
