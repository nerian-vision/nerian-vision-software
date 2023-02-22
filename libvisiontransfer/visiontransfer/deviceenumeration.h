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

#ifndef VISIONTRANSFER_DEVICEENUMERATION_H
#define VISIONTRANSFER_DEVICEENUMERATION_H

#include <vector>
#include "visiontransfer/common.h"
#include "visiontransfer/deviceinfo.h"

namespace visiontransfer {

/**
 * \brief Allows for the discovery of devices in the network.
 *
 * Devices are discovered by transmitting a broad-cast message to all
 * network interfaces and then waiting for replies.
 */
class VT_EXPORT DeviceEnumeration {
public:
    typedef std::vector<DeviceInfo> DeviceList;

    DeviceEnumeration();
    ~DeviceEnumeration();

    /**
     * \brief Discovers new devices and returns the list of all devices
     * that have been found
     * \return List of devices found
     */
    DeviceList discoverDevices() {
        // This code is inlined in order to provide binary compatibility with
        // different STL implementations
        int numDevices = 0;
        DeviceInfo* devices = getDevicesPointer(&numDevices);
        std::vector<DeviceInfo> ret(devices, &devices[numDevices]);
        return ret;
    }

private:
    // We follow the pimpl idiom
    class Pimpl;
    Pimpl* pimpl;

    // This class cannot be copied
    DeviceEnumeration(const DeviceEnumeration& other);
    DeviceEnumeration& operator=(const DeviceEnumeration&);

    DeviceInfo* getDevicesPointer(int* numDevices);
};

} // namespace

#endif
