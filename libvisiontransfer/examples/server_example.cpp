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

#include <visiontransfer/deviceenumeration.h>
#include <visiontransfer/asynctransfer.h>
#include <visiontransfer/imageset.h>
#include <iostream>
#include <exception>
#include <vector>
#include <cstdio>
#include <cstring>

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

int main() {
    try {
        // Create an image transfer object that will serve as server
        AsyncTransfer asyncTransfer("0.0.0.0", "7681", ImageProtocol::PROTOCOL_UDP, true);

        // Initialize the image set meta data
        ImageSet imageSet;
        imageSet.setWidth(640);
        imageSet.setHeight(480);
        // Define the set of contained image types, assigning indices
        imageSet.setNumberOfImages(2);
        imageSet.setIndexOf(ImageSet::IMAGE_LEFT, 0);
        imageSet.setIndexOf(ImageSet::IMAGE_RIGHT, 1);
        // Initialize data for all constituent images
        imageSet.setPixelFormat(0, ImageSet::FORMAT_8_BIT_MONO);
        imageSet.setPixelFormat(1, ImageSet::FORMAT_8_BIT_MONO);
        imageSet.setRowStride(0, imageSet.getBytesPerPixel(0)*imageSet.getWidth());
        imageSet.setRowStride(1, imageSet.getBytesPerPixel(0)*imageSet.getWidth());
        std::vector<unsigned char> pixelData(imageSet.getRowStride(0) * imageSet.getHeight());
        imageSet.setPixelData(0, &pixelData[0]);
        imageSet.setPixelData(1, &pixelData[0]);

        int transfer = 0;
        while(true) {
#ifdef _WIN32
            Sleep(500);
#else
            usleep(50000);
#endif
            if(!asyncTransfer.isConnected()) {
                // Continue waiting until a client connects
                continue;
            }

            if(transfer == 0) {
                cout << "Client IP: " << asyncTransfer.getRemoteAddress() << endl;
            }

            // Generate test image data
            for(int y=0; y<imageSet.getHeight(); y++) {
                for(int x=0; x<imageSet.getWidth(); x++) {
                    pixelData[y*imageSet.getRowStride(0) + x] = (y + x + transfer) & 0xff;
                }
            }
            transfer++;

            // Send the image data
            asyncTransfer.sendImageSetAsync(imageSet);
        }
    } catch(const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
    }

    return 0;
}
