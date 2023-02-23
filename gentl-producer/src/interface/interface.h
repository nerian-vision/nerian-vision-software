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

#ifndef NERIAN_INTERFACE_H
#define NERIAN_INTERFACE_H

#include "misc/common.h"
#include "misc/handle.h"
#include "misc/port.h"
#include "interface/interfaceportimpl.h"

#include <genicam/gentl.h>
#include <string>
#include <vector>
#include <map>
#include <visiontransfer/deviceenumeration.h>

namespace GenTL {

class System;

/*
 * Represents an IF_HANDLE and encapsulates all interface related
 * GenTL functions
 */
class Interface: public Handle {
public:
    Interface(System* system);

    // Methods that match functions form the GenTL interface
    GC_ERROR close();
    GC_ERROR updateDeviceList(bool8_t* pbChanged, uint64_t iTimeout);
    GC_ERROR getNumDevices(uint32_t* piNumDevices);
    GC_ERROR getDeviceID(uint32_t iIndex, char* sDeviceID, size_t* piSize);
    GC_ERROR getDeviceInfo(const char* sDeviceID, DEVICE_INFO_CMD iInfoCmd,
            INFO_DATATYPE* piType, void* pBuffer, size_t* piSize);
    GC_ERROR getParentTL(TL_HANDLE* phSystem);
    GC_ERROR getInfo(INTERFACE_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
            void* pBuffer, size_t* piSize);
    GC_ERROR openDevice(const char* sDeviceID, DEVICE_ACCESS_FLAGS iOpenFlags,
            DEV_HANDLE* phDevice);

    Port* getPort() {return &port;}
    System* getSystem() {return system;}

private:
    System* system;
    bool updateCalled;

    InterfacePortImpl portImpl;
    Port port;
    std::vector<visiontransfer::DeviceInfo> deviceList;
    std::vector<std::string> deviceSuffixes;
    std::map<std::string, std::string> deviceIDToModelName;

    bool parseDeviceUrl(const char* sDeviceID, bool& udp, std::string& host,
        std::string& stream);
    bool checkDeviceInUse(const char* sDeviceID);

    void updateDeviceMetadataCache();
};

}

#endif
