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

#ifndef NERIAN_SYSTEM_H
#define NERIAN_SYSTEM_H

#include "misc/common.h"
#include "misc/handle.h"
#include "misc/port.h"
#include "interface/interface.h"
#include "system/systemportimpl.h"

#include <genicam/gentl.h>
#include <memory>
#include <vector>

namespace GenTL {

// Some neccessary predeclarations
class DataStream;
class PhysicalDevice;
class LogicalDevice;

/*
 * This class represents a TL_HANDLE handle and it encapsulates
 * all GenTL system functions.
 */
class System: public Handle {
public:
    // Methods that match functions form the GenTL interface
    static GC_ERROR open(System** system);
    static GC_ERROR close(System* system);

    GC_ERROR updateInterfaceList(bool8_t* pbChanged, uint64_t iTimeout);
    GC_ERROR getNumInterfaces(uint32_t* piNumIfaces);
    GC_ERROR getInterfaceID(uint32_t iIndex, char* sIfaceID, size_t* piSize);
    GC_ERROR getInterfaceInfo(const char* sIfaceID, INTERFACE_INFO_CMD iInfoCmd,
            INFO_DATATYPE* piType, void* pBuffer, size_t* piSize);
    GC_ERROR openInterface(const char* sIfaceID, IF_HANDLE* phIface);
    GC_ERROR getInfo(TL_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
        void* pBuffer, size_t* piSize);

    // Methods that are used internally
    void addPhysicalDevice(std::shared_ptr<PhysicalDevice> device);
    PhysicalDevice* findPhysicalDeviceByAddress(bool udp, const char* host);
    void removePhysicalDevice(PhysicalDevice* device);
    void freeUnusedDevices();

    Port* getPort() {return &port;}

private:
    Interface interface; // There is only one interface
    bool updateCalled;

    // Two lists of registered physical and logical devices
    std::vector<std::shared_ptr<PhysicalDevice> > physicalDevices;

    SystemPortImpl portImpl;
    Port port;

    // Constructor is private because instances are created through open()
    System();
};

}

#endif
