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

#ifndef IMAGEREADER_H
#define IMAGEREADER_H

#include <vector>
#include <string>
#include <utility>
#include <opencv2/opencv.hpp>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>

// Loads a sequence of stereo images through a backround thread
class ImageReader {
public:
    typedef std::pair<cv::Mat, cv::Mat> StereoFrame;

    ImageReader(const char* directory, int maxQueueSize, bool multiThreaded);
    virtual ~ImageReader();

    // Pops a new image from the queue
    std::shared_ptr<StereoFrame> pop(double timeout = -1);

    int getQueueSize() {return queueSize;}

private:
    std::thread imageThread; // Queue thread
    int queueSize;
    std::vector<std::shared_ptr<StereoFrame> > queue; // The image queue
    int readIndex, writeIndex; // Current index of the queue
    std::mutex queueMutex;
    std::condition_variable popCond, pushCond;
    volatile bool done; // True if we reached the end
    bool multiThreaded; // If enabled, images are prefetched asynchroneously
    bool threadCreated;
    unsigned fileIndex;
    std::vector<std::string> imageFiles;

    // Main method for the queue thread
    void threadMain();

    // Pushes a new frame to the queue
    void push(std::shared_ptr<StereoFrame> frame);
    // Signals that the queue has reached its end
    void closeQueue();
    // Waits until there's a free slot in the queue
    void waitForFreeSlot();

    // Method for loading the next frame
    virtual void queueFrame();

    // Stops the thread if it has been created. Should be called
    // in destructor.
    void stopThreads();

    cv::Mat readNextFile();
    const char* getNextFileName();
};


#endif
