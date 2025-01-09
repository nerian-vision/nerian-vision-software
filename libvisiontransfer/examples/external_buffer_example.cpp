/*******************************************************************************
 * Copyright (c) 2024 Allied Vision Technologies GmbH
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
#include <visiontransfer/externalbufferset.h>
#include <iostream>
#include <exception>
#include <thread>
#include <chrono>
#include <algorithm>
#include <stdio.h>

// This is an example using libvisiontransfer to receive images into
// pre-allocated external buffers without extra copying.
//
// This is *not* required or recommended for normal operation
// -> Please refer to asynctransfer_example instead!


// Size of the pool; must be at least 2 (one buffer with the user, one for current reception).
//  If you set this to just one, every other frame will report an exhausted buffer pool.
const int numBufferSets = 3;

using namespace visiontransfer;

int main() {

    // The target receive buffers (in this example, `buffers` holds three 16 MB buffers).
    // They must be large enough to receive the entire configured image set, your hand is not held here.
    const size_t myBufSize = 16*1024*1024;
    unsigned char* buffers[numBufferSets];
    for (int i=0; i<numBufferSets; ++i) {
        buffers[i] = new unsigned char[myBufSize];
    }

    // In this example, we generate sets with a single buffer each, which will accept several image channels (they will be packed consecutively).
    ExternalBufferSet bufferSets[numBufferSets]; // Handles can be either provided to the constructors or auto-generated internally (do not mix)
    for (int i=0; i<numBufferSets; ++i) {
        // Wrap the raw buffer allocated above
        ExternalBuffer ebuf(buffers[i], myBufSize);
        // Define the mapping of ImageSet types and desired conversion rules. They will be packed in the defined order.
        ebuf.appendPartDefinition(ExternalBuffer::Part(ImageSet::IMAGE_COLOR, ExternalBuffer::CONVERSION_NONE));
        ebuf.appendPartDefinition(ExternalBuffer::Part(ImageSet::IMAGE_LEFT, ExternalBuffer::CONVERSION_NONE));
        ebuf.appendPartDefinition(ExternalBuffer::Part(ImageSet::IMAGE_DISPARITY, ExternalBuffer::CONVERSION_MONO_12_TO_16)); // convert 12bit packed to 16 (currently hardwired internally)
        ebuf.appendPartDefinition(ExternalBuffer::Part(ImageSet::IMAGE_RIGHT, ExternalBuffer::CONVERSION_NONE));
        // Absent channels are omitted, unless CONVERSION_RESERVE_IF_NOT_PRESENT is requested (which would leave a gap
        // in the buffer data for more predictable, unchanging offsets inside the buffer).

        // Populate buffer set with one buffer
        bufferSets[i].addBuffer(ebuf);
    }
    // We now have three buffer sets with one multi-part buffer each (configured to accept all four possible channels).

    // -> We have the prerequisites for an external receive queue of three ImageSets
    //  (of which one must always remain available to the receiver thread to avoid frame loss).

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

        // Create an image transfer configuration object to
        // receive data from the first detected device
        AsyncTransfer::Config cfg = AsyncTransfer::Config(devices[0]);
        // Add the above buffer sets
        for (int i=0; i<numBufferSets; ++i) {
            cfg.addExternalBufferSet(bufferSets[i]);
        }
        // Activate external buffering mode
        cfg.setExternalBufferingActive(true);
        // Create and launch an AsyncTransfer based on the config
        AsyncTransfer asyncTransfer(cfg);

        // Receive images in a loop
        for(int imgNum=0; ; imgNum++) {
            std::cout << "---------- Receiving image set " << imgNum << std::endl;

            // Receive image
            ImageSet imageSet;
            while(!asyncTransfer.collectReceivedImageSet(imageSet,
                0.1 /*timeout*/)) {
                // Keep on trying until reception is successful
            }
            // This ImageSet is backed by one of the ExternalBufferSets,
            // (assignment apparent using imageSet.getExternalBufferHandle()
            // and the unique values of ExternalBufferSet::getHandle()).
            // You may use getPixelData() as usual, or operate directly
            // on your known data buffer (data was packed according
            // to the reported pixel formats and conversion settings).

            // imageSet.getExternalBufferHandle() will return 0 for image channels
            // that were not configured to use an external buffer (their data
            // refers to a library buffer), and -1 to signal pool exhaustion
            // (data also pointing to internal buffer and NOT an external one).
            //
            // You must check this for each image channel when using external buffering mode.

            // ImageSet processing proper goes here

            std::cout << "Processing image set." << std::endl;

            // Example: we validate that we get all image channels in external buffers
            std::vector<std::pair<visiontransfer::ImageSet::ImageType, std::string> > channelsToProcess = {
                {visiontransfer::ImageSet::IMAGE_LEFT, "left"},
                {visiontransfer::ImageSet::IMAGE_DISPARITY, "disparity"},
                {visiontransfer::ImageSet::IMAGE_RIGHT, "right"},
                {visiontransfer::ImageSet::IMAGE_COLOR, "color"},
            };
            for (auto const& typeAndName: channelsToProcess) {
                auto const& name = typeAndName.second;
                int idx = imageSet.getIndexOf(typeAndName.first);
                if (idx == -1) {
                    std::cout << " Channel " << name << " not present in ImageSet" << std::endl;
                } else {
                    auto handle = imageSet.getExternalBufferHandle(idx);
                    if (handle==0) {
                        // Should not happen, we defined all image types in the buffer layout
                        std::cerr << "ERROR: Unexpected handle 0 for channel " << name << std::endl;
                        return 1;
                    } else if (handle==-1) {
                        std::cerr << "Buffer pool currently exhausted for channel " << name << std::endl;
                    } else {
                        std::cout << " Channel " << name << " with external buffer handle " << handle << std::endl;

                        //
                        // Normal image processing for the channel goes here
                        //
                    }
                }
            }

            std::cout << "Unlocking the processed image set." << std::endl;

            // *IMPORTANT* - Cleanup phase for each processed ImageSet
            //               (in external buffer mode only.)

            // Signal the AsyncTransfer that we are done using the ImageSet
            // and the underlying buffer -> it can be reused in reception now.
            // Otherwise channels would be marked as exhausted by the receiver thread
            // instead if no buffer sets are available when the next image set arrives.
            // (See above on how to detect this.)
            asyncTransfer.requeueExternalBuffersForImageSet(imageSet);

        }
    } catch(const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
    }

    for (int i=0; i<numBufferSets; ++i) {
        delete[] buffers[i];
    }

    return 0;
}


