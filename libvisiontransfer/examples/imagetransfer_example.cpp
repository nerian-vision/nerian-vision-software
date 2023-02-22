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
#include <visiontransfer/imagetransfer.h>
#include <visiontransfer/imageset.h>
#include <iostream>
#include <exception>
#include <stdio.h>

#ifdef _MSC_VER
// Visual studio does not come with snprintf
#define snprintf _snprintf_s
#endif

using namespace visiontransfer;

/*
 * NOTE: The use of ImageTransfer is only advised when operation
 *  without background threads is strictly required. For typical
 *  application use, we recommend to use AsyncTransfer instead
 *  (see asynctransfer_example.cpp).
 *
 *  When using ImageTransfer directly, make sure to regularly poll
 *  using receiveImageSet(); i.e. limit the frame rate through
 *  the device settings, and not by reducing call frequency.
 */

int main() {
        // Search for Nerian stereo devices
        DeviceEnumeration deviceEnum;
        DeviceEnumeration::DeviceList devices =
            deviceEnum.discoverDevices();
        if(devices.size() == 0) {
            std::cout << "No devices discovered!" << std::endl;
            return -1;
        }

        // Print devices
        std::cout << "Discovered devices:" << std::endl;
        for(unsigned int i = 0; i< devices.size(); i++) {
            std::cout << devices[i].toString() << std::endl;
        }
        std::cout << std::endl;

        // Create an image transfer object that receives data from
        // the first detected device
        ImageTransfer imageTransfer(devices[0]);

        // Receive 100 images
        for(int imgNum=0; imgNum<100; imgNum++) {
            std::cout << "Receiving image set " << imgNum << std::endl;

            // Receive image
            ImageSet imageSet;
            while(!imageTransfer.receiveImageSet(imageSet)) {
                // Keep on trying until reception is successful
            }

            // Write all included images one after another
            for(int i = 0; i < imageSet.getNumberOfImages(); i++) {
                // Create PGM file
                char fileName[100];
                snprintf(fileName, sizeof(fileName), "image%03d_%d.pgm", i,
                    imgNum);

                imageSet.writePgmFile(i, fileName);
            }
        }

    return 0;
}
