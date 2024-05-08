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
#include <visiontransfer/imagetransfer.h>
#include <visiontransfer/imageset.h>
#include <visiontransfer/deviceparameters.h>
#include <iostream>
#include <exception>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <chrono>
#include <string>
#include <map>

using namespace visiontransfer;
using namespace visiontransfer::param;

int main(int argc, const char** argv) {
    try {
        std::map<std::string, std::string> additionalParams;
        // Add optional extra key-value pairs from the arguments to the transaction, for testing
        // Usage: parameter_set_batch_write_example [key1 "value1" [key2 "value2" ...]]
        if (argc < 2) {
            std::cerr << "Must provide a parameter UID argument to poll (list with parameter_enumeration_example)." << std::endl;
            return 1;
        }
        std::string pollUid = argv[1];

        // Search for Nerian stereo devices
        DeviceEnumeration deviceEnum;

        DeviceEnumeration::DeviceList devices = deviceEnum.discoverDevices();
        if(devices.size() == 0) {
            std::cout << "No devices discovered!" << std::endl;
            return -1;
        }

        // Print devices (the first device is automatically used in this example)
        std::cout << "Discovered devices:" << std::endl;
        for(unsigned int i = 0; i< devices.size(); i++) {
            std::cout << devices[i].toString() << std::endl;
        }
        std::cout << std::endl;

        // Create a DeviceParameters object that connects to the parameter
        // service on the device to set / receive current device parameters.
        // This uses a TCP connection with auto-reconnect enabled by default.
        DeviceParameters parameters(devices[0]);

        auto param = parameters.getParameter(pollUid); // does not contain up-to-date value for polled parameters!
        if (! param.getIsPolled()) {
            std::cerr << "Polling of UID " << pollUid << " is not possible (updated automatically)." << std::endl;
            std::cout << "Current value for " << pollUid << ": " << param.getCurrent<std::string>() << std::endl;
            return 1;
        }

        try {
            auto polledParam = parameters.pollParameter(pollUid);
            std::cout << "Polled value for " << pollUid << ": " << polledParam.getCurrent<std::string>() << std::endl;
            // value will remain for this UID until the next active poll operation
        } catch(const std::exception& ex) {
            std::cerr << "Polling of UID " << pollUid << " failed: " << ex.what() << std::endl;
        }
    } catch(const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
    }

    return 0;
}


