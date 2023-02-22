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
#include <iostream>
#include <exception>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>

#include <visiontransfer/datachannelservice.h>

#ifdef _WIN32
#include <windows.h>
#define usleep(X) Sleep(X/1000)
#else
#include <unistd.h>
#endif


using namespace visiontransfer;

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

        std::cout << "Connecting to " << devices[0].toString() << std::endl;
        // DataChannelService will run asynchronously in the background
        DataChannelService service(devices[0]);

        // Allow some grace time for UDP host<->device handshake
        usleep(250000);

        std::cout << std::endl <<"Device reports " << (service.imuAvailable()?"an inertial measurement unit.":"inertial measurement unit is not available.") << std::endl << std::endl;

        if (!service.imuAvailable()) {
            return 1;
        }

        std::vector<TimestampedQuaternion> quaternionBufferData;
        std::vector<TimestampedVector> vectorBufferData;
        while(true) {
            usleep(20000);
            TimestampedQuaternion quat = service.imuGetRotationQuaternion();
            TimestampedVector lin = service.imuGetLinearAcceleration();
            quaternionBufferData = service.imuGetRotationQuaternionSeries();
            vectorBufferData = service.imuGetLinearAccelerationSeries();

            // Quaternion->Euler
            double roll, pitch, yaw;
            quat.getRollPitchYaw(roll, pitch, yaw);

            roll *= 180.0/M_PI;
            pitch *= 180.0/M_PI;
            yaw *= 180.0/M_PI;

            std::cout << std::setprecision(2) << std::fixed;
            std::cout << "Device orientation: " << quat.x() << " " << quat.y() << " " << quat.z() << " " << quat.w() << " accuracy " << quat.accuracy() << std::endl;
            std::cout << "  -> Roll " << roll << " Pitch " << pitch << " Yaw " << yaw << std::endl;
            std::cout << "  and read orientation time series of size  " << quaternionBufferData.size() << std::endl;
            std::cout << "Linear acceleration:    " << lin.x() << " " << lin.y() << " " << lin.z() << std::endl;
            std::cout << "  and read linear accel time series of size " << vectorBufferData.size() << std::endl;
            std::cout << std::endl;
        }

        return 0;
    } catch(const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
    }

    return 0;
}

