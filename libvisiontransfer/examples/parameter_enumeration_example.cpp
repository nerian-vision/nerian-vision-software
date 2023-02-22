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

        const int colW = 40;
        const int valueW = 8;
        std::cout << std::boolalpha << std::left;
        std::cout << "Server-side Parameter Enumeration" << std::endl;
        std::cout << "=================================" << std::endl << std::endl;
        ParameterSet allParams = parameters.getParameterSet();
        std::cout << "All " << allParams.size() << " parameters reported by server:" << std::endl;
        for (ParameterSet::iterator it = allParams.begin(); it != allParams.end(); ++it) {
            Parameter& param = it->second;
            switch (param.getType()) {
                case ParameterValue::TYPE_INT: {
                    std::cout << std::setw(colW) << (param.getUid()+" (int)") << " = " << std::setw(valueW) << param.getCurrent<int>();
                    if (param.hasRange()) {
                        std::cout << "  range " << param.getMin<int>() << "-" << param.getMax<int>();
                    }
                    if (param.getIncrement<int>() != 1) {
                        std::cout << "  increment " << param.getIncrement<int>();
                    }
                    std::cout << std::endl;
                    break;
                }
                case ParameterValue::TYPE_BOOL: {
                    std::cout << std::setw(colW) << (param.getUid() + " (bool)") << " = " << (param.getCurrent<bool>()?"true":"false") << std::endl;
                    break;
                }
                case ParameterValue::TYPE_DOUBLE: {
                    std::cout << std::setw(colW) << (param.getUid()+" (double)") << " = " << std::setw(valueW) << param.getCurrent<double>();
                    if (param.hasRange()) {
                        std::cout << "  range " << param.getMin<double>() << "-" << param.getMax<double>();
                    }
                    if (param.hasIncrement()) {
                        std::cout << "  increment " << param.getIncrement<double>();
                    }
                    std::cout << std::endl;
                    break;
                }
                case ParameterValue::TYPE_STRING: {
                    std::cout << std::setw(colW) << (param.getUid() + " (string)") << " = \"" << param.getCurrent<std::string>() << "\"" << std::endl;
                    break;
                }
                case ParameterValue::TYPE_SAFESTRING: {
                    std::cout << std::setw(colW) << (param.getUid() + " (safestring)") << " = \"" << param.getCurrent<std::string>() << "\"" << std::endl;
                    break;
                }
                case ParameterValue::TYPE_TENSOR: {
                    std::cout << std::setw(colW) << (param.getUid() + " (tensor) - shape: ");
                    for (unsigned int i=0; i<param.getTensorDimension(); ++i) {
                        if (i) std::cout << "x";
                        std::cout << param.getTensorShape()[i];
                    }
                    std::cout << std::endl;

                    std::vector<double> data = param.getTensorData();
                    std::cout << std::setw(colW) << (" ");
                    std::cout << " - data: ";
                    int perline = (param.getTensorDimension()==2) ? param.getTensorShape()[1] : param.getTensorNumElements();
                    for (unsigned int i=0; i<param.getTensorNumElements(); ++i) {
                        std::cout << data[i] << " ";
                        if (((i+1)%perline)==0) {
                            std::cout << std::endl << std::setw(colW) << (" ") << "         ";
                        }
                    }
                    std::cout << std::endl;
                    break;
                }
                case ParameterValue::TYPE_COMMAND: {
                    std::cout << std::setw(colW) << (param.getUid() + " (-> command trigger)") << std::endl;
                    break;
                default:
                    break;
                }
            }
            /*
            // Show description texts, where defined
            auto descr = param.getDescription();
            if (descr != "") {
                std::cout << "\tDescription: " << descr << std::endl;
            }
            */
        }
        std::cout << std::endl;

        // Setting an enumerated parameter
        if (argc > 1) {
            if (std::string(argv[1]) == "LOG") {
                std::cout << "(Showing incoming parameter events - terminate with Ctrl-C ...)" << std::endl;
                parameters.setParameterUpdateCallback([&parameters](const std::string& uid) {
                    // Gets the up-to-date value (not the one from the frozen ParameterSet we got earlier!)
                    std::cout << uid << " := " << parameters.getParameter(uid).getCurrent<std::string>() << std::endl;
                });
                while (true) std::this_thread::sleep_for(std::chrono::seconds(1));
            } else {
                std::string argname(argv[1]);
                if (argc > 2) {
                    std::string val(argv[2]);
                    std::cout << "Sending request to set " << argname << " to " << val << std::endl;
                    parameters.setParameter(argname, val);
                } else {
                    std::cout << "Requesting single parameter " << argname << std::endl;
                    std::cout << "-> cast as a string: " << parameters.getParameter(argname).getCurrent<std::string>() << std::endl;
                }
            }
        } else {
            std::cout << "You can launch this with a parameter name to get (and a value to set it)" << std::endl;
            std::cout << "  e.g. " << argv[0] << " operation_mode [2]" << std::endl;
            std::cout << "or with 'LOG' as first argument to see continuous parameter updates" << std::endl;
            std::cout << std::endl;
        }

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

