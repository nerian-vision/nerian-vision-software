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
#include <stdlib.h>
#include <thread>
#include <chrono>

using namespace visiontransfer;
using namespace visiontransfer::param;

int main(int argc, const char** argv) {
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
        bool uni = parameters.getParameter("uniqueness_check_enabled").getCurrent<bool>();
        bool tex = parameters.getParameter("texture_filter_enabled").getCurrent<bool>();
        std::cout << "Current values:" << std::endl;
        std::cout << " uniqueness_check_enabled == " << (uni?"true":"false") << std::endl;
        std::cout << " texture_filter_enabled   == " << (tex?"true":"false") << std::endl;

        // Use a Transaction to guard any batch update (although for these two independent
        //  parameters it could be fine without one). Only one active transaction per thread.
        std::cout << "Starting transaction" << std::endl;
        {
            auto transactionLock = parameters.transactionLock();
            parameters.setParameter("uniqueness_check_enabled", !uni);
            parameters.setParameter("texture_filter_enabled", !tex);
        } // -> transaction will be automatically committed at scope exit
        std::cout << "Transaction complete" << std::endl;

        // Output the updated parameterization
        uni = parameters.getParameter("uniqueness_check_enabled").getCurrent<bool>();
        tex = parameters.getParameter("texture_filter_enabled").getCurrent<bool>();
        std::cout << "New values:" << std::endl;
        std::cout << " uniqueness_check_enabled == " << (uni?"true":"false") << std::endl;
        std::cout << " texture_filter_enabled   == " << (tex?"true":"false") << std::endl;

        return 0;
    } catch(const std::exception& ex) {
        // Note: for setting parameters, there are two relevant exceptions that
        // should be handled. ParameterException indicates an invalid UID,
        // lack of access rights, or unacceptable value, while
        // TransferException likely means that the connection has been lost
        // and could not yet be reconnected automatically in the background.
        // You might want to retry the set operation later in the latter case.
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
    }

    return 0;
}

