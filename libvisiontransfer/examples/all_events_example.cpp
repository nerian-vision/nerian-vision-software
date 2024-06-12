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

//
// THis example subscribes to the available events in the AsyncTransfer
// and DeviceParameters, i.e. it showcases the use of a receiving image transfer
// alongside a parameter channel. This example can be used to log device<->client
// communication.
//

#include <visiontransfer/deviceenumeration.h>
#include <visiontransfer/asynctransfer.h>
#include <visiontransfer/imageset.h>
#include <visiontransfer/deviceparameters.h>
#include <visiontransfer/exceptions.h>
#include <iostream>
#include <exception>
#include <thread>
#include <chrono>
#include <stdio.h>

#ifdef _MSC_VER
// Visual studio does not come with snprintf
#define snprintf _snprintf_s
#endif

using namespace visiontransfer;

void showHelpAndExit() {
    std::cout << "Usage: all_events_example [--tcp <ipAddr> | --udp <ipAddr>]" << std::endl;
    std::cout << "  Logs incoming frames and parameter changes from the specified IP" << std::endl;
    std::cout << "  address, or else from the first device detected with a broadcast." << std::endl;
    std::cout << "  For manual IP address mode, you must choose between --tcp/--udp." << std::endl;
    exit(0);
}

int main(int argc, char** argv) {
    try {
        bool useUdp = true;
        std::string ipStr;
        const char* ipAddr = nullptr;
        if (argc > 1) {
            std::string arg = argv[1];
            if ((arg == "--help") || (argc < 3)) {
                showHelpAndExit();
            } else {
                if (argc < 3) {
                    showHelpAndExit();
                } else {
                    if (arg == "--udp") {
                        useUdp = true;
                    } else if (arg == "--tcp") {
                        useUdp = false;
                    } else {
                        showHelpAndExit();
                    }
                    ipAddr = argv[2];
                }
            }
        }

        if (ipAddr == nullptr) {
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

            // Choose first device to connect to
            ipStr = devices[0].getIpAddress();
            ipAddr = ipStr.c_str();
            useUdp = devices[0].getNetworkProtocol() == DeviceInfo::PROTOCOL_UDP;
        }

        std::cout << "Connecting to " << ipAddr << " for parameter connection"
                  << " and " << (useUdp?"UDP":"TCP") << " image transfer" << std::endl;

        // Create a DeviceParameters object that connects to the parameter
        // service on the device to set / receive current device parameters.
        // This uses a TCP connection with auto-reconnect enabled by default.
        DeviceParameters parameters(ipAddr);
        parameters.setConnectionStateChangeCallback([&parameters](visiontransfer::ConnectionState state){
            std::cout << "DeviceParameters connection state change: " << ((state==ConnectionState::CONNECTED)?"CONNECTED":"DISCONNECTED") << std::endl;
            // Debug / testing: toggle a parameter when connection comes back up
            if (state==ConnectionState::CONNECTED) {
                try {
                    auto transactionLock = parameters.transactionLock();
                    auto val = parameters.getParameter("texture_filter_enabled").getCurrent<int>();
                    parameters.setParameter("texture_filter_enabled", !val);
                    transactionLock->commitAndWait(2000);
                    std::cout << "Committed transaction" << std::endl;
                } catch(visiontransfer::TimeoutException& ex) {
                    (void) ex; // prevent build warning
                    std::cout << "(Timeout waiting for transaction effects - processing daemon may not be ready yet)" << std::endl;
                }
            }
        });
        // Show all remote parameter changes and device metadata updates as they come in
        parameters.setParameterUpdateCallback([&parameters](const std::string& uid) {
            // Gets the up-to-date value (not the one from the frozen ParameterSet we got earlier!)
            std::cout << "Parameter update: " << uid << ":=" << parameters.getParameter(uid).getCurrent<std::string>() << std::endl;
            // Debug / testing: toggle a parameter when another parameter is set.
            // (Needs threaded==true in setting the callback, see below; otherwise an exception is thrown here.)
            if (uid=="RT_temp_soc") {
                try {
                    std::cout << "Update: followup transaction ..." << std::endl;
                    auto transactionLock = parameters.transactionLock();
                    auto val = parameters.getParameter("texture_filter_enabled").getCurrent<int>();
                    parameters.setParameter("texture_filter_enabled", !val);
                    transactionLock->commitAndWait(2000);
                    std::cout << "Update: committed transaction." << std::endl;
                } catch(visiontransfer::TimeoutException& ex) {
                    (void) ex; // prevent build warning
                    std::cout << "(Timeout waiting for transaction effects - processing daemon may not be ready yet)" << std::endl;
                }
            }
        }, /* threaded = */ true);

        // Create an image transfer object that receives data from
        // the first detected device
        AsyncTransfer asyncTransfer(ipAddr, "7681", useUdp?ImageProtocol::PROTOCOL_UDP:ImageProtocol::PROTOCOL_TCP);

        // Connection state change handler: use this to be informed of any network
        // disconnection and subsequent automatic background reconnection.
        asyncTransfer.setConnectionStateChangeCallback([&asyncTransfer](visiontransfer::ConnectionState state){
                std::cout << "AsyncTransfer connection state change: " << ((state==ConnectionState::CONNECTED)?"CONNECTED":"DISCONNECTED") << std::endl;
                std::cout << "  -> AsyncTransfer.isConnected() = " << asyncTransfer.isConnected() << std::endl;
                });

        // Receive 100 images
        std::cout << "Receiving images, press Ctrl-C at any time to cancel ..." << std::endl;
        for (int imgNum=0;; ++imgNum) {

            // Receive image set
            ImageSet imageSet;
            while(!asyncTransfer.collectReceivedImageSet(imageSet,
                0.1 /*timeout*/)) {
                // Keep on trying until reception is successful
            }
            std::cout << "Received ImageSet #" << imgNum << " with image size " << imageSet.getWidth() << "x" << imageSet.getHeight() << ":" << std::endl;
            // Show contained image channels
            for (int idx=0; idx<imageSet.getNumberOfImages(); ++idx) {
                std::cout << "  Image " << idx << ": ";
                switch (imageSet.getImageType(idx)) {
                    case ImageSet::IMAGE_LEFT: std::cout << "left camera"; break;
                    case ImageSet::IMAGE_RIGHT: std::cout << "right camera"; break;
                    case ImageSet::IMAGE_DISPARITY: std::cout << "disparity"; break;
                    case ImageSet::IMAGE_COLOR: std::cout << "color camera"; break; // (Ruby)
                    default: std::cout << "UNDEFINED!"; break;
                }
                std::cout << ", " << imageSet.getBitsPerPixel(idx) << " bits" << std::endl;
            }

        }
    } catch(const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
    }

    return 0;
}

