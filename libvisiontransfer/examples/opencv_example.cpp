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

// OpenCV headers must be included first!
#include <opencv2/opencv.hpp>

#include <visiontransfer/deviceenumeration.h>
#include <visiontransfer/imagetransfer.h>
#include <visiontransfer/imageset.h>
#include <visiontransfer/reconstruct3d.h>
#include <iostream>
#include <exception>
#include <stdio.h>

#ifdef _MSC_VER
// Visual studio does not come with snprintf
#define snprintf _snprintf_s
#endif

using namespace visiontransfer;

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
        ImageTransfer imageTransfer(devices[0]);

        // Receive one image
        std::cout << "Receiving image set..." << std::endl;
        ImageSet imageSet;
        while(!imageTransfer.receiveImageSet(imageSet)) {
        }

        // Convert to OpenCV images
        for (int imageNumber=0; imageNumber < imageSet.getNumberOfImages(); imageNumber++) {
            char fileName[100];
            cv::Mat convertedImage;
            snprintf(fileName, sizeof(fileName), "image%d.pgm", imageNumber);
            imageSet.toOpenCVImage(imageNumber, convertedImage);
            cv::imwrite(fileName, convertedImage);
            std::cout << "Written " << fileName << " with OpenCV" << std::endl;
        }

    } catch(const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
    }

    return 0;
}
