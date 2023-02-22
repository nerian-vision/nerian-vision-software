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

#if __GNUC__ == 4 && __GNUC_MINOR__ < 9
// This is a very ugly workaround for GCC bug 54562. If omitted,
// passing timeouts to collectReceivedImage() is broken.
#include <bits/c++config.h>
#undef _GLIBCXX_USE_CLOCK_MONOTONIC
#endif

#include <iostream>
#include <functional>
#include <stdexcept>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include <vector>
#include <cstring>
#include <algorithm>
#include "visiontransfer/asynctransfer.h"
#include "visiontransfer/alignedallocator.h"

using namespace std;
using namespace visiontransfer;
using namespace visiontransfer::internal;

namespace visiontransfer {

/*************** Pimpl class containing all private members ***********/

class AsyncTransfer::Pimpl {
public:
    Pimpl(const char* address, const char* service,
        ImageProtocol::ProtocolType protType, bool server,
        int bufferSize, int maxUdpPacketSize);
    ~Pimpl();

    // Redeclaration of public members
    void sendImageSetAsync(const ImageSet& imageSet, bool deleteData);
    bool collectReceivedImageSet(ImageSet& imageSet, double timeout);
    int getNumDroppedFrames() const;
    bool isConnected() const;
    void disconnect();
    std::string getRemoteAddress() const;
    bool tryAccept();

private:
    static constexpr int NUM_BUFFERS = ImageSet::MAX_SUPPORTED_IMAGES * 3;
    static constexpr int SEND_THREAD_SHORT_WAIT_MS = 1;
    static constexpr int SEND_THREAD_LONG_WAIT_MS = 10;

    // The encapsulated image transfer object
    ImageTransfer imgTrans;

    // Variable for controlling thread termination
    volatile bool terminate;

    // There are two threads, one for sending and one for receiving.
    // Each has a mutex and condition variable for synchronization.
    std::thread sendThread;
    std::mutex sendMutex;
    std::condition_variable sendCond;
    std::condition_variable sendWaitCond;

    std::thread receiveThread;
    std::timed_mutex receiveMutex;
    std::condition_variable_any receiveCond;
    std::condition_variable_any receiveWaitCond;

    // Objects for exchanging images with the send and receive threads
    ImageSet receivedSet;
    std::vector<unsigned char, AlignedAllocator<unsigned char> > receivedData[NUM_BUFFERS];
    volatile bool newDataReceived;

    ImageSet sendImageSet;
    bool sendSetValid;
    bool deleteSendData;

    // Exception occurred in one of the threads
    std::exception_ptr receiveException;
    std::exception_ptr sendException;

    bool sendThreadCreated;
    bool receiveThreadCreated;

    // Main loop for sending thread
    void sendLoop();

    // Main loop for receiving;
    void receiveLoop();

    void createSendThread();
};

/******************** Stubs for all public members ********************/

AsyncTransfer::AsyncTransfer(const char* address, const char* service,
        ImageProtocol::ProtocolType protType, bool server,
        int bufferSize, int maxUdpPacketSize)
    : pimpl(new Pimpl(address, service, protType, server, bufferSize, maxUdpPacketSize)) {
}

AsyncTransfer::AsyncTransfer(const DeviceInfo& device, int bufferSize, int maxUdpPacketSize)
    : pimpl(new Pimpl(device.getIpAddress().c_str(), "7681", static_cast<ImageProtocol::ProtocolType>(device.getNetworkProtocol()),
    false, bufferSize, maxUdpPacketSize)) {
}

AsyncTransfer::~AsyncTransfer() {
    delete pimpl;
}

void AsyncTransfer::sendImageSetAsync(const ImageSet& imageSet, bool deleteData) {
    pimpl->sendImageSetAsync(imageSet, deleteData);
}

bool AsyncTransfer::collectReceivedImageSet(ImageSet& imageSet, double timeout) {
    return pimpl->collectReceivedImageSet(imageSet, timeout);
}

int AsyncTransfer::getNumDroppedFrames() const {
    return pimpl->getNumDroppedFrames();
}

bool AsyncTransfer::isConnected() const {
    return pimpl->isConnected();
}

void AsyncTransfer::disconnect() {
    return pimpl->disconnect();
}

std::string AsyncTransfer::getRemoteAddress() const {
    return pimpl->getRemoteAddress();
}

bool AsyncTransfer::tryAccept() {
    return pimpl->tryAccept();
}

/******************** Implementation in pimpl class *******************/

AsyncTransfer::Pimpl::Pimpl(const char* address, const char* service,
        ImageProtocol::ProtocolType protType, bool server,
        int bufferSize, int maxUdpPacketSize)
    : imgTrans(address, service, protType, server, bufferSize, maxUdpPacketSize),
    terminate(false), newDataReceived(false), sendSetValid(false),
    deleteSendData(false), sendThreadCreated(false),
    receiveThreadCreated(false) {

    if(server) {
        createSendThread();
    }
}

AsyncTransfer::Pimpl::~Pimpl() {
    terminate = true;

    sendCond.notify_all();
    receiveCond.notify_all();
    sendWaitCond.notify_all();
    receiveWaitCond.notify_all();

    if(sendThreadCreated && sendThread.joinable()) {
        sendThread.join();
    }

    if(receiveThreadCreated && receiveThread.joinable()) {
        receiveThread.join();
    }

    if(sendSetValid && deleteSendData) {
        delete[] sendImageSet.getPixelData(0);
        delete[] sendImageSet.getPixelData(1);
    }
}

void AsyncTransfer::Pimpl::createSendThread() {
    if(!sendThreadCreated) {
        // Lazy initialization of the send thread as it is not always needed
        unique_lock<mutex> lock(sendMutex);
        sendThread = thread(bind(&AsyncTransfer::Pimpl::sendLoop, this));
        sendThreadCreated = true;
    }
}

void AsyncTransfer::Pimpl::sendImageSetAsync(const ImageSet& imageSet, bool deleteData) {
    createSendThread();

    while(true) {
        unique_lock<mutex> lock(sendMutex);

        // Test for errors
        if(sendException) {
            std::rethrow_exception(sendException);
        }

        if(!sendSetValid) {
            sendImageSet = imageSet;
            sendSetValid = true;
            deleteSendData = deleteData;

            // Wake up the sender thread
            sendCond.notify_one();

            return;
        } else {
            // Wait for old data to be processed first
            sendWaitCond.wait(lock);
        }
    }
}

bool AsyncTransfer::Pimpl::collectReceivedImageSet(ImageSet& imageSet, double timeout) {
    if(!receiveThreadCreated) {
        // Lazy initialization of receive thread
        unique_lock<timed_mutex> lock(receiveMutex);
        receiveThreadCreated = true;
        receiveThread = thread(bind(&AsyncTransfer::Pimpl::receiveLoop, this));
    }

    // Acquire mutex
    unique_lock<timed_mutex> lock(receiveMutex, std::defer_lock);
    if(timeout < 0) {
        lock.lock();
    } else {
        std::chrono::steady_clock::time_point lockStart =
            std::chrono::steady_clock::now();
        if(!lock.try_lock_for(std::chrono::microseconds(static_cast<unsigned int>(timeout*1e6)))) {
            // Timed out
            return false;
        }

        // Update timeout
        unsigned int lockDuration = static_cast<unsigned int>(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - lockStart).count());
        timeout = std::max(0.0, timeout - lockDuration*1e-6);
    }

    // Test for errors
    if(receiveException) {
        std::rethrow_exception(receiveException);
    }

    if(timeout == 0 && !newDataReceived) {
        // No image has been received and we are not blocking
        return false;
    }

    // If there is no data yet then keep on waiting
    if(!newDataReceived) {
        if(timeout < 0) {
            while(!terminate && !receiveException && !newDataReceived) {
                receiveCond.wait(lock);
            }
        } else {
            receiveCond.wait_for(lock, std::chrono::microseconds(static_cast<unsigned int>(timeout*1e6)));
        }
    }

    // Test for errors again
    if(receiveException) {
        std::rethrow_exception(receiveException);
    }

    if(newDataReceived) {
        // Get the received image
        imageSet = receivedSet;

        newDataReceived = false;
        receiveWaitCond.notify_one();

        return true;
    } else {
        return false;
    }
}

void AsyncTransfer::Pimpl::sendLoop() {
    {
        // Delay the thread start
        unique_lock<mutex> lock(sendMutex);
    }

    ImageSet imgSet;
    bool deleteSet = false;

    try {
        while(!terminate) {
            // Wait for next image
            {
                unique_lock<mutex> lock(sendMutex);
                // Wait for next frame to be queued
                bool firstWait = true;
                while(!terminate && !sendSetValid) {
                    imgTrans.transferData();
                    sendCond.wait_for(lock, std::chrono::milliseconds(
                        firstWait ? SEND_THREAD_SHORT_WAIT_MS : SEND_THREAD_LONG_WAIT_MS));
                    firstWait = false;
                }
                if(!sendSetValid) {
                    continue;
                }

                imgSet = sendImageSet;
                deleteSet = deleteSendData;
                sendSetValid = false;

                sendWaitCond.notify_one();
            }

            imgTrans.setTransferImageSet(imgSet);
            while(!terminate) {
                ImageTransfer::TransferStatus status = imgTrans.transferData();
                if(status != ImageTransfer::PARTIAL_TRANSFER && status != ImageTransfer::WOULD_BLOCK) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(SEND_THREAD_LONG_WAIT_MS));
            }

            if(deleteSet) {
                for (int i=0; i<imgSet.getNumberOfImages(); ++i) {
                    delete[] imgSet.getPixelData(i);
                }
                deleteSet = false;
            }
        }
    } catch(...) {
        // Store the exception for later
        if(!sendException) {
            sendException = std::current_exception();
        }
        sendWaitCond.notify_all();

        // Don't forget to free the memory
        if(deleteSet) {
            for (int i=0; i<imgSet.getNumberOfImages(); ++i) {
                delete[] imgSet.getPixelData(i);
            }
            deleteSet = false;
        }
    }
}

void AsyncTransfer::Pimpl::receiveLoop() {
    {
        // Delay the thread start
        unique_lock<timed_mutex> lock(receiveMutex);
    }

    try {
        ImageSet currentSet;
        int bufferIndex = 0;

        while(!terminate) {
            // Receive new image
            if(!imgTrans.receiveImageSet(currentSet)) {
                // No image available
                continue;
            }

            // Copy the pixel data
            for(int i=0;i<currentSet.getNumberOfImages();i++) {
                int bytesPerPixel = currentSet.getBytesPerPixel(i);
                int newStride = currentSet.getWidth() * bytesPerPixel;
                int totalSize = currentSet.getHeight() * newStride;
                int bufIdxHere = (i + bufferIndex) % NUM_BUFFERS;
                if(static_cast<int>(receivedData[bufIdxHere].size()) < totalSize) {
                    receivedData[bufIdxHere].resize(totalSize);
                }
                if(newStride == currentSet.getRowStride(i)) {
                    memcpy(&receivedData[bufIdxHere][0], currentSet.getPixelData(i),
                        newStride*currentSet.getHeight());
                } else {
                    for(int y = 0; y<currentSet.getHeight(); y++) {
                        memcpy(&receivedData[bufIdxHere][y*newStride],
                            &currentSet.getPixelData(i)[y*currentSet.getRowStride(i)],
                            newStride);
                    }
                    currentSet.setRowStride(i, newStride);
                }
                currentSet.setPixelData(i, &receivedData[bufIdxHere][0]);
            }

            {
                unique_lock<timed_mutex> lock(receiveMutex);

                // Wait for previously received data to be processed
                while(newDataReceived) {
                    receiveWaitCond.wait_for(lock, std::chrono::milliseconds(100));
                    if(terminate) {
                        return;
                    }
                }

                // Notify that a new image set has been received
                newDataReceived = true;
                receivedSet = currentSet;
                receiveCond.notify_one();
            }

            // Increment index for data buffers
            bufferIndex = (bufferIndex + currentSet.getNumberOfImages()) % NUM_BUFFERS;
        }
    } catch(...) {
        // Store the exception for later
        if(!receiveException) {
            receiveException = std::current_exception();
        }
        receiveCond.notify_all();
    }
}

bool AsyncTransfer::Pimpl::isConnected() const {
    return imgTrans.isConnected();
}

void AsyncTransfer::Pimpl::disconnect() {
    imgTrans.disconnect();
}

std::string AsyncTransfer::Pimpl::getRemoteAddress() const {
    return imgTrans.getRemoteAddress();
}

int AsyncTransfer::Pimpl::getNumDroppedFrames() const {
    return imgTrans.getNumDroppedFrames();
}

bool AsyncTransfer::Pimpl::tryAccept() {
    return imgTrans.tryAccept();
}

constexpr int AsyncTransfer::Pimpl::NUM_BUFFERS;
constexpr int AsyncTransfer::Pimpl::SEND_THREAD_SHORT_WAIT_MS;
constexpr int AsyncTransfer::Pimpl::SEND_THREAD_LONG_WAIT_MS;

} // namespace

