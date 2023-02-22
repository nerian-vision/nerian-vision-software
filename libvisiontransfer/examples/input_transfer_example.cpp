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

/* This example demonstrate the sending of input image data to SceneScan.
 * For this example, SceneScan must be configured to use the virtual
 * "Network" camera, and the correct image size and pixel format must be
 * set.
 *
 * It is highly recommended to use TCP instead of UDP in this configuration.
 */

#include <visiontransfer/deviceenumeration.h>
#include <visiontransfer/asynctransfer.h>
#include <visiontransfer/imageset.h>
#include <iostream>
#include <exception>
#include <vector>
#include <cstdio>
#include <cstring>
#include <thread>

#ifdef _WIN32
#   include <windows.h>
#else
#   include <unistd.h>
#endif

#ifdef _MSC_VER
// Visual studio does not come with snprintf
#define snprintf _snprintf_s
#endif

using namespace std;
using namespace visiontransfer;

static const int numImages = 10;
static const ImageSet::ImageFormat imageFormat = ImageSet::FORMAT_8_BIT_MONO;
static const int imageWidth = 640;
static const int imageHeight = 480;

int receivedFrames = 0;

void sendThread(AsyncTransfer* asyncTransfer) {
    try {
        // Initialize the image set meta data
        static ImageSet imageSet; // Data must remain valid while still sending
        imageSet.setWidth(imageWidth);
        imageSet.setHeight(imageHeight);
        // Define the set of contained image types, assigning indices
        imageSet.setNumberOfImages(2);
        imageSet.setIndexOf(ImageSet::IMAGE_LEFT, 0);
        imageSet.setIndexOf(ImageSet::IMAGE_RIGHT, 1);
        // Initialize data for all constituent images
        imageSet.setPixelFormat(0, imageFormat);
        imageSet.setPixelFormat(1, imageFormat);
        imageSet.setRowStride(0, imageSet.getBytesPerPixel(0)*imageSet.getWidth());
        imageSet.setRowStride(1, imageSet.getBytesPerPixel(0)*imageSet.getWidth());
        const int extraBufferSpace = 16; // Leave space some extra data in the buffer
        static std::vector<unsigned char> pixelData(imageSet.getRowStride(0) * imageSet.getHeight() + extraBufferSpace);
        imageSet.setPixelData(0, &pixelData[0]);
        imageSet.setPixelData(1, &pixelData[0]);

        for(int i=0; i<numImages; i++) {
            while(i-2 >= receivedFrames) { // Allow a maximum of 2 frames lead for sending
#ifdef _WIN32
            Sleep(10);
#else
            usleep(10000);
#endif
        }

            // Generate test image data
            for(int y=0; y<imageSet.getHeight(); y++) {
                for(int x=0; x<imageSet.getRowStride(0); x++) {
                    pixelData[y*imageSet.getRowStride(0) + x] = (y + x + i) & 0xff;
                }
            }

            // Send the image data
            std::cout << "Sending image set " << i << std::endl;
            asyncTransfer->sendImageSetAsync(imageSet);
        }
    } catch(const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
    }
}

void receiveThread(AsyncTransfer* asyncTransfer) {
    for(int i=0; i<numImages; i++) {
        std::cout << "Receiving image set " << i << std::endl;

        // Receive image
        ImageSet imageSet;
        while(!asyncTransfer->collectReceivedImageSet(imageSet,
            0.1 /*timeout*/)) {
            // Keep on trying until reception is successful
        }

        receivedFrames++;

        // Write all included images one after another
        for(int imageNumber = 0; imageNumber < imageSet.getNumberOfImages(); imageNumber++) {
            // Create PGM file
            char fileName[100];
            snprintf(fileName, sizeof(fileName), "image%03d_%d.pgm", i,
                imageNumber);

            imageSet.writePgmFile(imageNumber, fileName);
        }
    }
}

int main() {
    try {
        // Search for Nerian stereo devices
        DeviceEnumeration deviceEnum;
        DeviceEnumeration::DeviceList devices = deviceEnum.discoverDevices();
        if(devices.size() == 0) {
            std::cout << "No devices discovered!" << std::endl;
            return -1;
        }

        // Create an image transfer object that receives data from
        // the first detected device
        AsyncTransfer asyncTransfer(devices[0]);

        std::thread sndThread(sendThread, &asyncTransfer);
        std::thread rcvThread(receiveThread, &asyncTransfer);

        sndThread.join();
        rcvThread.join();
    } catch(const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
    }

    return 0;
}
