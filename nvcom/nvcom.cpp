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

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <string>
#include <sstream>

#ifdef WITH_OPEN3D
#   include <open3d/Open3D.h>
#endif

#ifdef _WIN32
#   include <windows.h>
#else
#   include <unistd.h>
#endif

#include "nvcom.h"

using namespace std;
using namespace std::chrono;
using namespace cv;
using namespace visiontransfer;

NVCom::NVCom(const Settings& newSettings):
    terminateThreads(false), captureNextFrame(false), captureSequence(false), captureIndex(0),
    sendSeqNum(0), receivedSeqNum(-1), minDisparity(0), maxDisparity(0), lastImageCount(0), lastFormats{},
    needColorCoderReinit(true), resetColorScale(true)
{

    for (int i=0; i<ImageSet::MAX_SUPPORTED_IMAGES; ++i) {
        receivedFrames.emplace_back(cv::Mat());
        convertedFrames.emplace_back(cv::Mat());
        lastFormats.push_back(-1);
    }
    updateSettings(newSettings);
}

NVCom::~NVCom() {
    terminate();
}

void NVCom::terminate() {
    joinAllThreads();
    imageReader.reset(nullptr);
    asyncTrans.reset();
}

void NVCom::joinAllThreads() {
    terminateThreads = true;

    if(writingThread.joinable()) {
        writingCond.notify_all();
        writingThread.join();
    }

    if(mainLoopThread.joinable()) {
        mainLoopThread.join();
    }
}

void NVCom::connect() {
    // Just need to start the threads
    terminateThreads = false;
    mainLoopThread = std::thread(std::bind(&NVCom::mainLoop, this));
    writingThread = std::thread(std::bind(&NVCom::writeLoop, this));
}

void NVCom::mainLoop() {
    bool connected = false;

    try {
        while(!terminateThreads) {
            // First establish network connection
            if(asyncTrans == nullptr) {
                asyncTrans.reset(new AsyncTransfer(
                    settings.remoteHost.c_str(), to_string(settings.remotePort).c_str(),
                    settings.tcp ? ImageProtocol::PROTOCOL_TCP : ImageProtocol::PROTOCOL_UDP));
                connectedCallback();
            }

            // Detect disconnects
            if(connected && !asyncTrans->isConnected()) {
                disconnectCallback();
            }
            connected = asyncTrans->isConnected();

            if(!transmitFrame()) {
                if(settings.readImages && receivedSeqNum == sendSeqNum-1) {
                    // Sending is complete when we received the last frame back
                    settings.readImages = false;
                    sendCompleteCallback();
                }
            }

            if(!settings.disableReception) {
                ImageSet imageSet;
                bool gotFrame = receiveFrame(imageSet);
                system_clock::time_point stamp = system_clock::now();

                if(!gotFrame) {
                    continue;
                }

                int minDisp = 0, maxDisp = 0;
                imageSet.getDisparityRange(minDisp, maxDisp);

                if(minDisp != minDisparity || maxDisp != maxDisparity || needColorCoderReinit) {
                    minDisparity = minDisp + settings.disparityOffset;
                    maxDisparity = maxDisp + settings.disparityOffset;

                    // Force update of legend
                    std::pair<unsigned short, unsigned short> invalidRange =
                        settings.disparityOffset == 0 ? std::pair<unsigned short, unsigned short>(0xFFF, 0xFFFF) :
                        std::pair<unsigned short, unsigned short>(0xFFFF, 0xFFFF);
                    redBlueCoder.reset(new ColorCoder(ColorCoder::COLOR_RED_BLUE_RGB,
                        minDisparity*imageSet.getSubpixelFactor(),
                        maxDisparity*imageSet.getSubpixelFactor(), false, false, invalidRange));
                    rainbowCoder.reset(new ColorCoder(ColorCoder::COLOR_RAINBOW_RGB, minDisparity*imageSet.getSubpixelFactor(),
                        maxDisparity*imageSet.getSubpixelFactor(), false, false, invalidRange));
                    needColorCoderReinit = false;
                    resetColorScale = true;
                }

                if(!settings.view3D || captureNextFrame || captureSequence) {
                    colorCodeAndDisplay(imageSet);
                }

                if(settings.view3D && imageSet.hasImageType(ImageSet::IMAGE_DISPARITY)) {
                    display3D(imageSet);
                }
                captureFrameIfRequested(imageSet, stamp);

                if(settings.displayCoordinate) {
                    unique_lock<mutex> lock(imageSetMutex);
                    imageSet.copyTo(lastImageSet);
                }
            }

            // Sleep for a while if we are processing too fast
            if(settings.readImages) {
                frameRateLimit->next();
            }
        }
    } catch(const std::exception& ex) {
        exceptionCallback(ex);
    }
}

void NVCom::colorCodeAndDisplay(visiontransfer::ImageSet& imageSet) {
    if(receivedFrames[0].data == nullptr) {
        return; // Can't display anything
    }

    bool formatChanged = false;
    for(int i=0; i<ImageSet::MAX_SUPPORTED_IMAGES; i++) {
        formatChanged = formatChanged || (lastFormats[i] != receivedFrames[i].type());
    }

    // Prepare window resizing
    bool resize = false;
    if(formatChanged || lastFrameSize != receivedFrames[0].size() ||
            lastImageCount != imageSet.getNumberOfImages()) {
        // Size of buffers will change
        lastFrameSize = receivedFrames[0].size();
        for(int i=0; i<ImageSet::MAX_SUPPORTED_IMAGES; i++) {
            convertedFrames[i] = cv::Mat();
            lastFormats[i] = receivedFrames[i].size().height ? receivedFrames[i].type() : -1;
        }
        lastImageCount = imageSet.getNumberOfImages();
        resize = true;
    }

    // Display the image
    {
        unique_lock<mutex> lock(displayMutex);
        for (int i=0; i<imageSet.getNumberOfImages(); ++i) {
            convertFrame(receivedFrames[i], convertedFrames[i], i==imageSet.getIndexOf(ImageSet::ImageType::IMAGE_DISPARITY),
                imageSet.getSubpixelFactor());
        }
        frame2DDisplayCallback(receivedFrames[0].cols, receivedFrames[0].rows, convertedFrames, imageSet.getNumberOfImages(), resize);
    }
}

void NVCom::display3D(visiontransfer::ImageSet& imageSet) {
#ifdef WITH_OPEN3D
    unique_lock<mutex> lock(displayMutex);

    double fPixel = imageSet.getQMatrix()[11];
    double fovHoriz = 2*atan((imageSet.getWidth()/2) / fPixel);
    frame3DDisplayCallback(recon3d.createOpen3DCloud(imageSet, Reconstruct3D::COLOR_AUTO, 0,
        settings.disparityOffset == 0 ? 0xFFF : 0xFFF0), fovHoriz);
#endif
}

bool NVCom::transmitFrame() {
    std::shared_ptr<ImageReader::StereoFrame> stereoSendFrame;

    if(receivedSeqNum < sendSeqNum-2) {
        return true; // Allow a maximum of 2 frames lead for sending
    }

    // Get frame to send if an image queue exists
    if(imageReader != nullptr) {
        stereoSendFrame = imageReader->pop();
        if(stereoSendFrame == nullptr) {
            // No more frames
            return false;
        }
    }

    // Transmit frame
    if(stereoSendFrame != nullptr) {
        ImageSet pair;
        pair.setWidth(stereoSendFrame->first.cols);
        pair.setHeight(stereoSendFrame->first.rows);
        pair.setRowStride(0, stereoSendFrame->first.step[0]);
        pair.setRowStride(1, stereoSendFrame->second.step[0]);

        ImageSet::ImageFormat format;
        switch(stereoSendFrame->first.type()) {
            case CV_8UC1:
                format = ImageSet::FORMAT_8_BIT_MONO;
                break;
            case CV_16UC1:
                format = ImageSet::FORMAT_12_BIT_MONO;
                break;
            case CV_8UC3:
                format = ImageSet::FORMAT_8_BIT_RGB;
                break;
            default: throw std::runtime_error("Unsupported pixel format!");

        }
        pair.setPixelFormat(0, format);
        pair.setPixelFormat(1, format);
        pair.setSequenceNumber(sendSeqNum++);

        steady_clock::time_point time = steady_clock::now();
        long long microSecs = duration_cast<microseconds>(time.time_since_epoch()).count();
        pair.setTimestamp(microSecs / 1000000, microSecs % 1000000);

        // Clone image data such that we can delete the original
        const int extraBufferSpace = 16;
        unsigned char* leftPixel = new unsigned char[stereoSendFrame->first.step[0] * stereoSendFrame->first.rows + extraBufferSpace];
        unsigned char* rightPixel = new unsigned char[stereoSendFrame->second.step[0] * stereoSendFrame->second.rows + extraBufferSpace];
        memcpy(leftPixel, stereoSendFrame->first.data, stereoSendFrame->first.step[0] * stereoSendFrame->first.rows);
        memcpy(rightPixel, stereoSendFrame->second.data, stereoSendFrame->second.step[0] * stereoSendFrame->second.rows);

        pair.setPixelData(0, leftPixel);
        pair.setPixelData(1, rightPixel);

        asyncTrans->sendImageSetAsync(pair, true);
    }

    return true;
}

bool NVCom::receiveFrame(ImageSet& imageSet) {
    if(!asyncTrans->collectReceivedImageSet(imageSet, 0.1)) {
        // No image received yet
        imageSet = ImageSet();
        lastImageCount = 0;
        for(int i=0; i<ImageSet::MAX_SUPPORTED_IMAGES; i++) {
            receivedFrames[i] = cv::Mat();
        }
        return false;
    }

    if(settings.printTimestamps) {
        int secs = 0, microsecs = 0;
        imageSet.getTimestamp(secs, microsecs);

        int ppsSecs = 0, ppsMicrosecs = 0;
        imageSet.getLastSyncPulse(ppsSecs, ppsMicrosecs);

        cout.fill('0');
        cout << "Timestamp: " << secs << "." << setw(6) << microsecs
            << "; Last PPS pulse: " << ppsSecs << "." << setw(6) << ppsMicrosecs
            << "; Exposure Time: " << imageSet.getExposureTime()*1e-3 << " ms" << endl;
    }

    receivedSeqNum = imageSet.getSequenceNumber();

    // Convert received data to opencv images
    for (int i=0; i<imageSet.getNumberOfImages(); ++i) {
        imageSet.toOpenCVImage(i, receivedFrames[i], false);

        if(imageSet.getImageType(i) == ImageSet::IMAGE_DISPARITY && settings.disparityOffset != 0) {
            Mat mask;
            inRange(receivedFrames[i], Scalar((unsigned short)0xFFF), Scalar((unsigned short)0xFFFF), mask);
            receivedFrames[i] += cv::Scalar((unsigned short)(settings.disparityOffset*imageSet.getSubpixelFactor() + 0.5));
            receivedFrames[i].setTo(Scalar((unsigned short)0xFFFF), mask);
        }
    }

    return true;
}

void NVCom::convertFrame(const cv::Mat& src, cv::Mat_<cv::Vec3b>& dst, bool colorCode, int subpixFactor) {
    if(src.type() == CV_16U) {
        if(!colorCode || settings.colorScheme == Settings::COLOR_SCHEME_NONE) {
            // Convert 16 to 8 bit
            cv::Mat_<unsigned char> image8Bit;
            src.convertTo(image8Bit, CV_8U, 1.0/16.0);
            cvtColor(image8Bit, dst, cv::COLOR_GRAY2RGB);
        } else {

            std::unique_ptr<ColorCoder>& coder = (
                settings.colorScheme == Settings::COLOR_SCHEME_RED_BLUE ? redBlueCoder : rainbowCoder);
            if(coder != nullptr) {
                // Perform histogram adaptation (if desired)
                cv::Mat hist;
                float histMin=9e19f, histMax=0;
                int effectiveLow=0, effectiveHigh=maxDisparity-1;
                if (settings.adaptiveColorScale) {
                    const int SKIP_PIXEL_PERMILLE_HIGH = 1; // proportion of histogram fringes that are considered noise for the adaptation
                    const int SKIP_PIXEL_PERMILLE_LOW = 2;  //
                    const int ADAPTATION_MOMENTUM = 10; // weight for previous range as opposed to current range (slower, less jerky adaptation)
                    // Histogram
                    int numBins = maxDisparity+1;
                    int histSize[] = { numBins };
                    float histRange[] = { 0, 1.0f*(maxDisparity+1)*subpixFactor };
                    const float* histRanges[] = { histRange };
                    int histChannels[] = { 0 };
                    // Select a region excluding the left border and calculate the disparity histogram
                    cv::Mat roi = src(cv::Range::all(), cv::Range(min(maxDisparity, src.cols/4), src.cols));
                    cv::calcHist(&roi, 1, histChannels, cv::Mat(), hist, 1, histSize, histRanges, true, false);
                    // Obtain min/max, but skip invalid values
                    for (int i=0; i<maxDisparity; ++i) { // skip D-th bin
                        auto hv = hist.at<float>(i, 0);
                        histMin = std::min(histMin, hv);
                        histMax = std::max(histMax, hv);
                    }
                    // Obtain the effective min and max bins by cropping the specified
                    //  percentage from the borders
                    const int skipPixelNumLow = SKIP_PIXEL_PERMILLE_LOW*(src.cols*src.rows)/1000;
                    const int skipPixelNumHigh = SKIP_PIXEL_PERMILLE_HIGH*(src.cols*src.rows)/1000;
                    int accum = 0;
                    for (int i=0; i<maxDisparity-1; ++i) {
                        accum += hist.at<float>(i, 0);
                        if (accum > skipPixelNumLow) break;
                        effectiveLow++;
                    }
                    accum = 0;
                    for (int i=maxDisparity-1; i>effectiveLow; --i) {
                        accum += hist.at<float>(i, 0);
                        if (accum > skipPixelNumHigh) break;
                        effectiveHigh--;
                    }

                    // Leave extra margin
                    effectiveLow *= 0.9;
                    effectiveHigh *= 1.1;
                    if(effectiveHigh > maxDisparity) {
                        effectiveHigh = maxDisparity;
                    }
                    if(effectiveLow < minDisparity) {
                        effectiveLow = minDisparity;
                    }

                    // Finally, update the coder range with the effective range
                    int adjustedMomentum = resetColorScale ? 0 : ADAPTATION_MOMENTUM;
                    coder->setMin(floor((coder->getMin()*adjustedMomentum + effectiveLow*subpixFactor)/(adjustedMomentum+1)));
                    coder->setMax(ceil((coder->getMax()*adjustedMomentum + effectiveHigh*subpixFactor)/(adjustedMomentum+1)));
                    coder->recalculateLookups();
                    resetColorScale = false;
                }

                // Perform color coding
                if(dst.data == nullptr || settings.adaptiveColorScale) {
                    dst = coder->createLegendBorder(src.cols, src.rows, 1.0/subpixFactor);
                }
                cv::Mat_<cv::Vec3b> dstSection = dst(Rect(0, 0, src.cols, src.rows));
                coder->codeImage((cv::Mat_<unsigned short>)src, dstSection);
            }
        }
    } else {
        // Just convert grey to 8 bit RGB
        if(src.channels() == 1) {
            cvtColor(src, dst, cv::COLOR_GRAY2RGB);
        } else {
            dst = src;
        }
    }
}

void NVCom::captureFrameIfRequested(const ImageSet& imageSet, system_clock::time_point timeStamp) {

    if(captureNextFrame || captureSequence) {
        if(!captureSequence) {
            cout << "Writing frame " << captureIndex << endl;
        }

        captureNextFrame = false;

        for (int i=0; i<imageSet.getNumberOfImages(); ++i) {
            // Schedule write for i-th frame
            auto& srcFrame = receivedFrames[i];
            ImageType typeRaw = IMG_NONE;
            ImageType typeColor = IMG_NONE;

            if(imageSet.getImageType(i) == ImageSet::IMAGE_DISPARITY) {
                if(settings.writeDisparityRaw) {
                    typeRaw = IMG_DISPARITY_RAW;
                }
                if(settings.writeDisparityColor) {
                    typeColor = IMG_DISPARITY_COLOR;
                }
            } else {
                ImageType type = IMG_NONE;
                if(imageSet.getImageType(i) == ImageSet::IMAGE_LEFT && settings.writeLeft) {
                    type = IMG_LEFT;
                } else if(imageSet.getImageType(i) == ImageSet::IMAGE_RIGHT && settings.writeRight) {
                    type = IMG_RIGHT;
                } else if(imageSet.getImageType(i) == ImageSet::IMAGE_COLOR && settings.writeThirdColor) {
                    type = IMG_THIRD_COLOR;
                }
                if(srcFrame.channels() > 1) {
                    typeColor = type;
                } else {
                    typeRaw = type;
                }
            }

            if(typeRaw != IMG_NONE) {
                scheduleWrite(srcFrame.clone(), captureIndex, typeRaw, timeStamp);
            }
            if(typeColor != IMG_NONE && convertedFrames[i].cols != 0) {
                cv::Mat_<cv::Vec3b> rgbImage;
                cvtColor(convertedFrames[i], rgbImage, cv::COLOR_RGB2BGR);
                scheduleWrite(rgbImage, captureIndex, typeColor, timeStamp);
            }
        }

        // Write timestamps
        int secs = 0, microsecs = 0;
        imageSet.getTimestamp(secs, microsecs);
        if(!timestampFile.is_open())  {
            timestampFile.open(settings.writeDir + "/timestamps.txt", ios::out);
            if(timestampFile.fail()) {
                throw std::runtime_error("Unable to create timestamp file!");
            }
            timestampFile.fill('0');
        }
        timestampFile << captureIndex << "; " << secs << "." << setw(6) << microsecs << endl;

        // For simplicity, point clouds are not written asynchroneously
        if(settings.writePointCloud && imageSet.hasImageType(ImageSet::IMAGE_DISPARITY)) {
            std::string fileName = getFileName(captureIndex, IMG_POINTCLOUD, false, timeStamp);
            recon3d.writePlyFile((settings.writeDir + "/" + fileName).c_str(), imageSet,
                settings.pointCloudMaxDist, settings.binaryPointCloud,
                Reconstruct3D::COLOR_AUTO, settings.disparityOffset == 0 ? 0xFFF : 0xFFFF);
        }
        captureIndex++;
    }
}

void NVCom::writeLoop() {
    try {
        while(!terminateThreads || writeQueue.size() > 0) {
            pair<string, Mat> frame;
            {
                unique_lock<mutex> lock(writingMutex);
                while(writeQueue.size() == 0 && !terminateThreads) {
                    writingCond.wait(lock);
                }
                if(terminateThreads && writeQueue.size() == 0) {
                    return;
                }
                frame = writeQueue.front();
                writeQueue.pop();
            }
            if(frame.second.data != NULL && frame.second.cols != 0) {
                string filePath = settings.writeDir + "/" + string(frame.first);
                if(!imwrite(filePath, frame.second))
                    cerr << "Error writing file: " << filePath << endl;
            }
        }
    } catch(const std::exception& ex) {
        exceptionCallback(ex);
    }
}

std::string NVCom::getFileName(int index, ImageType type, bool color, system_clock::time_point timeStamp) {
    // Get the file extension
    std::string fileType;
    if(type == IMG_POINTCLOUD) {
        fileType=".ply";
    } else if(!settings.writePgm) {
        fileType=".png";
    } else if(color) {
        fileType=".ppm";
    } else {
        fileType=".pgm";
    }

    // Get the name suffix
    const char* suffix;
    switch(type) {
        case IMG_LEFT: suffix = "left"; break;
        case IMG_RIGHT: suffix = "right"; break;
        case IMG_THIRD_COLOR: suffix = "color"; break;
        case IMG_DISPARITY_COLOR: suffix = "disp_color"; break;
        case IMG_DISPARITY_RAW: suffix ="disp_raw"; break;
        case IMG_POINTCLOUD: suffix = "3d"; break;
        default: throw std::runtime_error("Illegal image type");
    }

    // Get the name
    std::string name;
    if(settings.fileNameDateTime) {
        time_t time = system_clock::to_time_t(timeStamp);
        auto duration = timeStamp.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d_%H-%M-%S-")
            <<  std::setw(3) << std::setfill('0') << millis;
        name = ss.str();
    } else {
        char str[16];
        snprintf(str, sizeof(str), "image%06d", index);
        name = str;
    }

    return name + "_" + suffix + fileType;
}

void NVCom::scheduleWrite(const cv::Mat& frame, int index, ImageType type, system_clock::time_point timeStamp) {
    if(terminateThreads) {
        return; //Write thread is already terminating
    }

    if(settings.convert12Bit && frame.type() == CV_16U) {
        // Convert 12 to 16 bit
        frame *= 16;
    }

    std::string fileName = getFileName(index, type, (frame.channels() == 3), timeStamp);

    unique_lock<mutex> lock(writingMutex);
    writeQueue.push(pair<string, Mat>(fileName, frame));
    writingCond.notify_one();
}

void NVCom::updateSettings(const Settings& newSettings) {
    bool restartThreads = false;
    if(mainLoopThread.joinable() || writingThread.joinable()) {
        restartThreads = true;
        joinAllThreads();
    }

    if(newSettings.colorScheme != settings.colorScheme) {
        unique_lock<mutex> lock(displayMutex);

        // Make sure that a new color legend is created
        for(int i=0; i<ImageSet::MAX_SUPPORTED_IMAGES; i++) {
            convertedFrames[i] = cv::Mat();
        }
        lastFrameSize = cv::Size2i(0,0);
    }

    if(newSettings.writeDir  != settings.writeDir) {
        timestampFile.close();
        captureIndex = 0;
    }

    if(newSettings.readImages != settings.readImages ||
            newSettings.readDir != settings.readDir) {
        if(newSettings.readDir != "" && newSettings.readImages) {
            imageReader.reset(new ImageReader(newSettings.readDir.c_str(), 10, true));
        } else {
            imageReader.reset();
        }
        sendSeqNum = 0;
        receivedSeqNum = -1;
    }

    if(newSettings.adaptiveColorScale != settings.adaptiveColorScale) {
        needColorCoderReinit = true;
    }

    frameRateLimit.reset(new RateLimit(newSettings.maxFrameRate));

    settings = newSettings;

    if(restartThreads) {
        connect();
    }
}

std::vector<int> NVCom::getBitDepths() {
    std::vector<int> ret(ImageSet::MAX_SUPPORTED_IMAGES);
    for(int i=0; i<ImageSet::MAX_SUPPORTED_IMAGES; i++) {

        int bits;
        int format = lastFormats[i];

        switch(format) {
            case CV_8U: bits = 8; break;
            case CV_16U: bits = 12; break;
            case CV_8UC3: bits = 24; break;
            default: bits = -1;
        }
        ret[i] = bits;
    }
    return ret;
}

cv::Point3f NVCom::getDisparityMapPoint(int x, int y) {
    unique_lock<mutex> lock(imageSetMutex);

    if(!settings.displayCoordinate || !lastImageSet.hasImageType(ImageSet::IMAGE_DISPARITY) ||
        lastImageSet.getPixelFormat(ImageSet::IMAGE_DISPARITY) != ImageSet::FORMAT_12_BIT_MONO
        || x < 0 || y < 0 || x >= lastImageSet.getWidth() || y >= lastImageSet.getHeight()) {
        // This is not a valid disparity map
        return cv::Point3f(0, 0, 0);
    } else {
        unsigned short disp = reinterpret_cast<const unsigned short*>(
            lastImageSet.getPixelData(ImageSet::IMAGE_DISPARITY))[y*lastImageSet.getRowStride(ImageSet::IMAGE_DISPARITY)/2 + x];

        if((settings.disparityOffset == 0 && disp == 0xFFF) || disp == 0xFFFF) {
            // Invalid
            return cv::Point3f(0, 0, 0);
        }

        static Reconstruct3D recon3d;

        float x3d, y3d, z3d;
        recon3d.projectSinglePoint(x, y, disp, lastImageSet.getQMatrix(),
            x3d, y3d, z3d, lastImageSet.getSubpixelFactor());
        return cv::Point3f(x3d, y3d, z3d);
    }
}
