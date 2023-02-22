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
        AsyncTransfer asyncTransfer(devices[0]);

        // Receive 100 images
        for(int imgNum=0; imgNum<100; imgNum++) {
            std::cout << "Receiving image set " << imgNum << std::endl;

            // Receive image
            ImageSet imageSet;
            while(!asyncTransfer.collectReceivedImageSet(imageSet,
                0.1 /*timeout*/)) {
                // Keep on trying until reception is successful
            }
            
            // Project to 3D pointcloud
            Reconstruct3D recon3d;
            float* pointcloud = recon3d.createPointMap(imageSet, 0 /*minimum disparity*/);
            
            // ... insert additional code to process pointcloud ...
            
            // Write PLY file
            char fileName[100];
            snprintf(fileName, sizeof(fileName), "pointcloud%03d.ply", imgNum);
            recon3d.writePlyFile(fileName, imageSet,
                2.0, /* max distance in meters*/
                true /* binary format*/);
        }
    } catch(const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
    }

    return 0;
}
