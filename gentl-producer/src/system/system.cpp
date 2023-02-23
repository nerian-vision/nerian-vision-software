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

#include "system/system.h"
#include "system/library.h"
#include "device/physicaldevice.h"
#include "device/logicaldevice.h"
#include "stream/datastream.h"
#include "misc/infoquery.h"
#include "misc/common.h"
#include <iostream>
#include <fstream>

namespace GenTL {

#ifdef ENABLE_DEBUGGING
#ifdef _WIN32
    std::fstream debugStreamSyst("C:\\debug\\gentl-debug-sys-" + std::to_string(time(nullptr)) + ".txt", std::ios::out);
#else
    std::ostream& debugStreamSyst = std::cout;
#endif
#define DEBUG_SYST(x) debugStreamSyst << x << std::endl;
#else
#define DEBUG_SYST(x) ;
#endif

using namespace std::placeholders;

System::System(): Handle(TYPE_SYSTEM), interface(this), updateCalled(false),
    portImpl(this), port("nerian-gentl", "system.xml", "TLPort", "TLSystem", &portImpl) {
}

GC_ERROR System::open(System** system) {
    *system = nullptr;

    if(system == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    } else {
        try {
            *system = new System;
            return GC_ERR_SUCCESS;
        } catch(...) {
            return GC_ERR_ACCESS_DENIED;
        }
    }
}

GC_ERROR System::close(System* system) {
    delete system;
    return GC_ERR_SUCCESS;
}

GC_ERROR System::updateInterfaceList(bool8_t* pbChanged, uint64_t iTimeout) {
    if (pbChanged != nullptr) {
        // We signal an update on the first call
        if (updateCalled) {
            *pbChanged = 0;
        }
        else {
            *pbChanged = 1;
        }
    }
    updateCalled = true;
    return GC_ERR_SUCCESS;
}

GC_ERROR System::getNumInterfaces(uint32_t* piNumIfaces) {
    if(piNumIfaces == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    } else {
        *piNumIfaces = 1;
        return GC_ERR_SUCCESS;
    }
}

GC_ERROR System::getInterfaceID(uint32_t iIndex, char* sIfaceID, size_t* piSize) {
    if(iIndex != 0) {
        return GC_ERR_INVALID_INDEX;
    }

    InfoQuery info(nullptr, sIfaceID, piSize);
    info.setString("eth");
    return info.query();
}

GC_ERROR System::getInterfaceInfo(const char* sIfaceID, INTERFACE_INFO_CMD iInfoCmd,
        INFO_DATATYPE* piType, void* pBuffer, size_t* piSize) {
    if(sIfaceID == nullptr || std::string(sIfaceID) != "eth") {
        return GC_ERR_INVALID_ID;
    }

    return interface.getInfo(iInfoCmd, piType, pBuffer, piSize);
}

GC_ERROR System::openInterface(const char* sIfaceID, IF_HANDLE* phIface) {
    if(sIfaceID == nullptr || std::string(sIfaceID) != "eth") {
        return GC_ERR_INVALID_ID;
    } else {
        *phIface = reinterpret_cast<IF_HANDLE>(&interface);
        return GC_ERR_SUCCESS;
    }
}

GC_ERROR System::getInfo(TL_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
        void* pBuffer, size_t* piSize) {
    return Library::getInfo(iInfoCmd, piType, pBuffer, piSize);
}

PhysicalDevice* System::findPhysicalDeviceByAddress(bool udp, const char* host) {
    for(unsigned int i=0; i<physicalDevices.size(); i++) {
        if(physicalDevices[i]->isUdp() == udp &&
                physicalDevices[i]->getHost() == host) {
            return physicalDevices[i].get();
        }
    }
    DEBUG_SYST("PhysicalDevice not found for " << host << " udp? " << (int) udp);
    return nullptr;
}

void System::addPhysicalDevice(std::shared_ptr<PhysicalDevice> device) {
    DEBUG_SYST("Adding a PhysicalDevice");
    physicalDevices.push_back(device);
}

void System::removePhysicalDevice(PhysicalDevice* device) {
    // Check if there are still more logicial devices for this device
    for(unsigned int i=0; i<physicalDevices.size(); i++) {
        if(physicalDevices[i].get() != device) {
            DEBUG_SYST("Removing a PhysicalDevice");
            physicalDevices.erase(physicalDevices.begin() + i);
            break;
        }
    }
}

void System::freeUnusedDevices() {
    for(int i=0; i<static_cast<int>(physicalDevices.size()); i++) {
        if(!physicalDevices[i]->inUse()) {
            DEBUG_SYST("Freeing an unused PhysicalDevice");
            physicalDevices.erase(physicalDevices.begin() + i);
            i--;
        }
    }
}

}
