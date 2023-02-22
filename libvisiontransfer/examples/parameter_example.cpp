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
#include <visiontransfer/deviceparameters.h>
#include <iostream>
#include <exception>
#include <iomanip>
#include <stdio.h>

#ifdef _MSC_VER
    #include <windows.h>
    // Visual studio does not come with sleep
    #define sleep(x) Sleep(x*1000)
#else
    #include <unistd.h>
#endif

using namespace visiontransfer;

int main(int, char**) {
    try {
        // Search for Nerian stereo devices
        DeviceEnumeration deviceEnum;

        DeviceEnumeration::DeviceList devices = deviceEnum.discoverDevices();
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
        // the first detected Nerian stereo device
        DeviceParameters parameters(devices[0]);

        // Output the current parameterization

        const int colW = 35;
        std::cout << std::boolalpha << std::left;
        std::cout << "Current Parameters" << std::endl;
        std::cout << "==================" << std::endl << std::endl;
        std::cout << std::setw(colW) << "Operation mode: " << parameters.getOperationMode() << std::endl;
        std::cout << std::setw(colW) << "Disparity offset: " << parameters.getDisparityOffset() << std::endl;
        std::cout << std::setw(colW) << "Stereo P1 edge: " << parameters.getStereoMatchingP1Edge() << std::endl;
        std::cout << std::setw(colW) << "Stereo P2 edge: " << parameters.getStereoMatchingP2Edge() << std::endl;
        std::cout << std::setw(colW) << "Stereo P1 no edge: " << parameters.getStereoMatchingP1NoEdge() << std::endl;
        std::cout << std::setw(colW) << "Stereo P2 no edge: " << parameters.getStereoMatchingP2NoEdge() << std::endl;
        std::cout << std::setw(colW) << "Stereo edge sensitivity: " << parameters.getStereoMatchingEdgeSensitivity() << std::endl;
        std::cout << std::setw(colW) << "Mask border pixels: " << parameters.getMaskBorderPixelsEnabled() << std::endl;
        std::cout << std::setw(colW) << "Consistency check enabled: " << parameters.getConsistencyCheckEnabled() << std::endl;
        std::cout << std::setw(colW) << "Consistency check sensitivity: " << parameters.getConsistencyCheckSensitivity() << std::endl;
        std::cout << std::setw(colW) << "Uniqueness check enabled: " << parameters.getUniquenessCheckEnabled() << std::endl;
        std::cout << std::setw(colW) << "Uniqueness check sensitivity: " << parameters.getUniquenessCheckSensitivity() << std::endl;
        std::cout << std::setw(colW) << "Texture filter enabled: " << parameters.getTextureFilterEnabled() << std::endl;
        std::cout << std::setw(colW) << "Texture filter sensitivity: " << parameters.getTextureFilterSensitivity() << std::endl;
        std::cout << std::setw(colW) << "Gap interpolation enabled: " << parameters.getGapInterpolationEnabled() << std::endl;
        std::cout << std::setw(colW) << "Noise reduction enabled: " << parameters.getNoiseReductionEnabled() << std::endl;
        std::cout << std::setw(colW) << "Speckle filter itarations: " << parameters.getSpeckleFilterIterations() << std::endl;
        std::cout << std::setw(colW) << "Auto exposure/gain mode: " << parameters.getAutoMode() << std::endl;
        std::cout << std::setw(colW) << "Auto target intensity: " << parameters.getAutoTargetIntensity() << std::endl;
        std::cout << std::setw(colW) << "Auto intensity delta : " << parameters.getAutoIntensityDelta() << std::endl;
        std::cout << std::setw(colW) << "Auto target frame: " << parameters.getAutoTargetFrame() << std::endl;
        std::cout << std::setw(colW) << "Auto skipped frames: " << parameters.getAutoSkippedFrames() << std::endl;
        std::cout << std::setw(colW) << "Maximum auto exposure time: " << parameters.getAutoMaxExposureTime() << " us" << std::endl;
        std::cout << std::setw(colW) << "Maximum auto gain: " << parameters.getAutoMaxGain() << " dB" << std::endl;
        std::cout << std::setw(colW) << "Manual exposure time: " << parameters.getManualExposureTime() << " us" << std::endl;
        std::cout << std::setw(colW) << "Manual gain: " << parameters.getManualGain() << " dB" << std::endl;
        std::cout << std::setw(colW) << "Auto ROI enabled: " << parameters.getAutoROIEnabled() << std::endl;

        int x = 0, y = 0, width = 0, height = 0;
        parameters.getAutoROI(x, y, width, height);
        std::cout << std::setw(colW) << "Auto ROI: " << "(" << x << ", " << y << ") (" << width << " x " << height << ")" << std::endl;

        std::cout << std::setw(colW) << "Maximum frame time difference: " << parameters.getMaxFrameTimeDifference() << " ms" << std::endl;
        std::cout << std::setw(colW) << "Trigger frequency: " << parameters.getTriggerFrequency() << " Hz" << std::endl;
        std::cout << std::setw(colW) << "Trigger 0 enabled: " << parameters.getTrigger0Enabled() << std::endl;
        std::cout << std::setw(colW) << "Trigger 1 enabled: " << parameters.getTrigger1Enabled() << std::endl;
        std::cout << std::setw(colW) << "Trigger 0 pulse width: " << parameters.getTrigger0PulseWidth() << " ms"<< std::endl;
        std::cout << std::setw(colW) << "Trigger 1 pulse width: " << parameters.getTrigger1PulseWidth() << " ms"<< std::endl;
        std::cout << std::setw(colW) << "Trigger 1 offset: " << parameters.getTrigger1Offset() << " ms"<< std::endl;
        std::cout << std::setw(colW) << "Auto re-calibration enabled: " << parameters.getAutoRecalibrationEnabled() << std::endl;
        std::cout << std::setw(colW) << "Save auto re-calibration: " << parameters.getSaveAutoRecalibration() << std::endl;


        // Change a few selected parameters

        std::cout << std::endl
                  << "Changing Parameters" << std::endl
                  << "==================" << std::endl << std::endl;

        std::cout << "Operation mode..." << std:: endl;
        parameters.setOperationMode(DeviceParameters::PASS_THROUGH);
        sleep(1);
        parameters.setOperationMode(DeviceParameters::STEREO_MATCHING);
        sleep(1);

        std::cout << "Disparity offset..." << std::endl;
        try {
            // This will get refused if the full disparity range is active
            parameters.setDisparityOffset(10);
            sleep(1);
            parameters.setDisparityOffset(0);
            sleep(1);
        } catch (...) {
            std::cout << "... failed (reduce number of disparities to test this)." << std::endl;
        }

        std::cout << "Auto exposure target brightness..." << std:: endl;
        parameters.setAutoTargetIntensity(0.7);
        sleep(1);
        parameters.setAutoTargetIntensity(0.33);
        sleep(1);

        std::cout << "Trigger frequency..." << std:: endl;
        parameters.setTriggerFrequency(5);
        sleep(1);
        parameters.setTriggerFrequency(30);
        sleep(1);

        return 0;
    } catch(const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
    }

    return 0;
}

