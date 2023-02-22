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

#include "imagereader.h"
#include <functional>
#include <exception>
#include <algorithm>
#include <cstring>

#include <QDirIterator>

using namespace cv;
using namespace std;

ImageReader::ImageReader(const char* directory, int queueSize, bool multiThreaded)
    :queueSize(queueSize), queue(queueSize),
        readIndex(0), writeIndex(0), done(false), multiThreaded(multiThreaded),
        threadCreated(false), fileIndex(0) {

    // Crate file list
    QDirIterator dirIter(directory);
    while(dirIter.hasNext()) {
        std::string file = dirIter.next().toLocal8Bit().toStdString();
        if(file[0] != '.') {
            imageFiles.push_back(file);
        }
    }

    // Sort files
    sort(imageFiles.begin(), imageFiles.end());
}

ImageReader::~ImageReader() {
    stopThreads();
}

void ImageReader::stopThreads() {
    done = true;

    // Notify queue thread if blocked
    pushCond.notify_all();
    popCond.notify_all();

    if(threadCreated) {
        imageThread.join();
        threadCreated = false;
    }
}

std::shared_ptr<ImageReader::StereoFrame> ImageReader::pop(double timeout) {
    if(!multiThreaded) {
        // Synchroneously queue frame
        while(!done && queue[readIndex] == nullptr)
            queueFrame();
    // Start the thread if not yet running
    } else if(!done && !threadCreated) {
        imageThread = thread(bind(&ImageReader::threadMain, this));
        threadCreated = true;
    }

    {
        unique_lock<mutex> lock(queueMutex);

        while(queue[readIndex] == nullptr) {
            if(done) {
                // No more images available. Return a nullptr
                return nullptr;
            }
            else {
                // Queue underrun! Wait for more images
                if(timeout < 0) {
                    popCond.wait(lock);
                } else {
                    popCond.wait_for(lock, std::chrono::microseconds(static_cast<unsigned int>(timeout*1e6)));
                }
                if(timeout >= 0 && queue[readIndex] == nullptr) {
                    return nullptr;
                }
            }
        }

        // Get image from queue
        std::shared_ptr<StereoFrame> ret = queue[readIndex];
        // Clear queue slot
        queue[readIndex].reset();
        // Notify queue thread if blocked
        pushCond.notify_all();
        // Advance index
        readIndex = (readIndex + 1)%queueSize;

        return ret;
    }
}

void ImageReader::threadMain() {
    try {
        while(!done)
            queueFrame();
    } catch(const std::exception &e) {
        cerr << "Exception in image queueing thread: " << e.what() << endl;
    }
}

void ImageReader::queueFrame() {
    cv::Mat left = readNextFile();
    cv::Mat right = readNextFile();
    std::shared_ptr<StereoFrame> stereoFrame(new StereoFrame(left, right));

    if(stereoFrame->first.data != nullptr && stereoFrame->second.data != nullptr) {
        push(stereoFrame);
    } else {
        closeQueue();
    }
}

const char* ImageReader::getNextFileName() {
    if(fileIndex >= imageFiles.size())
        return nullptr;
    else return imageFiles[fileIndex++].c_str();
}

cv::Mat ImageReader::readNextFile() {
    const char* fileName;

    while((fileName = getNextFileName()) != nullptr) {
        Mat frame = imread(fileName, cv::IMREAD_ANYDEPTH | cv::IMREAD_ANYCOLOR);
        if(frame.data == nullptr) {
            cerr << "Skipping unreadable file: " << fileName << endl;
        } else {
            if(frame.channels() == 3) {
                // Convert to RGB
                cvtColor(frame, frame, cv::COLOR_RGB2BGR);
            }
            return frame;
        }
    }

    return Mat();
}

void ImageReader::waitForFreeSlot() {
    unique_lock<mutex> lock(queueMutex);
    if(queue[writeIndex] != nullptr)
        pushCond.wait(lock);
}

void ImageReader::push(std::shared_ptr<StereoFrame> frame) {
    unique_lock<mutex> lock(queueMutex);
    if(done) {
        return; // everything has already finished
    }

    if(queue[writeIndex] != nullptr) {
        // Queue is full. We have to wait
        pushCond.wait(lock);
        if(done) {
            return;
        }
    }

    queue[writeIndex] = frame;

    // Advance Index
    writeIndex = (writeIndex + 1)%queueSize;
    // Notify waiting pop()
    popCond.notify_one();
}

void ImageReader::closeQueue() {
    done = true;
    // Notify waiting pop()
    popCond.notify_all();
}
