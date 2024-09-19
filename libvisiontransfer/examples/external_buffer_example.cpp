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
#include <visiontransfer/externalbuffer.h>
#include <iostream>
#include <exception>
#include <stdio.h>

using namespace visiontransfer;

int main() {
    // This is an example using libvisiontransfer to receive images into externally allocated buffers with minimal copying.
    // For normal operation, refer to the asynctransfer.cpp example file, which uses automatic internal allocation.

    // The target receive buffers (in this example, `buffers` holds three 16 MB buffers).
    // They must be large enough to receive the entire configured image set, your hand is not held here.
    const size_t myBufSize = 16*1024*1024;
    unsigned char* buffers[3];
    for (int i=0; i<3; ++i) {
        buffers[i] = new unsigned char[myBufSize];
    }

    // One or more buffers are added to a buffer set; for each buffer in the set you select which image channels it accepts.
    // In this example, we generate sets with a single buffer each, which will accept several image channels (they will be packed consecutively).
    ExternalBufferSet bufferSets[3];
    for (int i=0; i<3; ++i) {
        // Wrap the raw buffer allocated above
        ExternalBuffer ebuf(buffers[i], myBufSize);
        // Define the mapping of ImageSet types and desired conversion rules. They will be packed in the defined order.
        ebuf.appendPartDefinition(ExternalBuffer::Part(ImageSet::IMAGE_LEFT, ExternalBuffer::CONVERSION_NONE));
        ebuf.appendPartDefinition(ExternalBuffer::Part(ImageSet::IMAGE_DISPARITY, ExternalBuffer::CONVERSION_MONO_12_TO_16)); // convert 12bit packed to 16, as usual
        // In this example, we leave out any other ImageSet::ImageTypes - this means they are not packed into
        // the resulting buffer layout, and will not be available as part of the ImageSets (even if enabled on-device).
        // Absent channels are omitted, unless CONVERSION_RESERVE_IF_NOT_PRESENT is requested (which would leave a gap
        // in the buffer data for more predictable, unchanging offsets inside the buffer).

        // Populate buffer set with one buffer
        bufferSets[i].addBuffer(ebuf);
    }
    // We now have three buffer sets with one buffer each (which are configured to accept the left and disparity channels).
    // -> We have the prerequisites for an external receive queue of three ImageSets.

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
        // Add the above buffer sets (also activates external buffering mode)
        for (int i=0; i<3; ++i) {
            cfg.addExternalBufferSet(bufferSets[i]);
        }
        // Create and launch an AsyncTransfer based on the config
        AsyncTransfer asyncTransfer(cfg);

        // Receive 100 images
        for(int imgNum=0; imgNum<100; imgNum++) {
            std::cout << "Receiving image set " << imgNum << std::endl;

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
            // on your allocated buffer (expect data being packed according
            // to the reported pixel formats and conversion settings).

            // ImageSet processing proper (here, just saving it)

            // Write all included images one after another
            for(int i = 0; i < imageSet.getNumberOfImages(); i++) {
                // Create PGM file
                char fileName[100];
                snprintf(fileName, sizeof(fileName), "image%03d_%d.pgm", imgNum, i);
                imageSet.writePgmFile(i, fileName);
            }

            // *IMPORTANT* - Cleanup phase for each processed ImageSet
            //
            // Signal the AsyncTransfer that we are done using the ImageSet
            // and the underlying buffer -> it can be reused in reception now.
            // If usable buffer sets are exhausted when reception is called,
            // frames would be discarded otherwise.
            asyncTransfer.signalImageSetDone(imageSet);

        }
    } catch(const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
    }

    for (int i=0; i<3; ++i) {
        delete[] buffers[i];
    }

    return 0;
}


