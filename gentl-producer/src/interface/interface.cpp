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

#include "interface/interface.h"
#include "system/system.h"
#include "device/physicaldevice.h"
#include "device/logicaldevice.h"
#include "misc/infoquery.h"
#include <cstring>
#include <cstdio>

using namespace visiontransfer;

namespace GenTL {

Interface::Interface(System* system): Handle(TYPE_INTERFACE), system(system),
    portImpl(this), port("eth", "interface.xml", "InterfacePort", "TLInterface", &portImpl),
    deviceSuffixes{"/", "/left", "/right", "/third_color", "/disparity", "/pointcloud"} {

    updateDeviceList(nullptr, 0);
}

GC_ERROR Interface::close() {
    return GC_ERR_SUCCESS;
}

GC_ERROR Interface::updateDeviceList(bool8_t* pbChanged, uint64_t iTimeout) {
    std::vector<DeviceInfo> oldList = deviceList;
#ifdef DELIVER_TEST_DATA
    deviceList.clear();
    deviceList.push_back(DeviceInfo("123.123.123.123", DeviceInfo::PROTOCOL_UDP, "1.0",
        DeviceInfo::SCENESCAN, true));
#else
    DeviceEnumeration devEnum;
    deviceList = devEnum.discoverDevices();
#endif
    updateDeviceMetadataCache();

    if (pbChanged != nullptr) {
        if(oldList != deviceList) {
            *pbChanged = 1;
        } else {
            *pbChanged = 0;
        }
    }
    return GC_ERR_SUCCESS;
}

// Updated device-related metadata like reported model name etc.,
// for each enumerated device, and for all [pseudo]device suffixes
void Interface::updateDeviceMetadataCache() {
    char deviceID[1024];
    size_t sz = 1024;
    deviceIDToModelName.clear();
    for (int iIndex=0; iIndex<(int) deviceList.size(); ++iIndex) {
        DeviceInfo& devInfo = deviceList[iIndex / deviceSuffixes.size()];
        // For querying hardware parameters by abstract name
        DeviceInfo::DeviceModel model = devInfo.getModel();
        std::string modelName;
        switch(model) {
            case DeviceInfo::DeviceModel::SCENESCAN_PRO:
                modelName = "SceneScan Pro";
                break;
            case DeviceInfo::DeviceModel::SCARLET:
                modelName = "Scarlet";
                break;
            case DeviceInfo::DeviceModel::RUBY:
                modelName = "Ruby";
                break;
            default:
                break;
        }
        for (int i=0; i<deviceSuffixes.size(); ++i) {
            sz = 1024;
            (void) getDeviceID(iIndex*deviceSuffixes.size() + i, deviceID, &sz);
            std::string subName = modelName;
            if (i != 0) {
                // Not the multipart device: add channel description to disambiguate name
                subName += std::string(" (") + deviceSuffixes[i].substr(1) + " only)";
            }
            deviceIDToModelName[deviceID] = subName;
        }

    }
}

GC_ERROR Interface::getNumDevices(uint32_t* piNumDevices) {
    if(piNumDevices == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    } else {
        *piNumDevices = static_cast<unsigned int>(deviceList.size()*deviceSuffixes.size());
        return GC_ERR_SUCCESS;
    }
}

GC_ERROR Interface::getDeviceID(uint32_t iIndex, char* sDeviceID, size_t* piSize) {
    if(iIndex >= deviceList.size()*deviceSuffixes.size()) {
        return GC_ERR_INVALID_INDEX;
    }

    InfoQuery info(nullptr, sDeviceID, piSize);
    DeviceInfo& devInfo = deviceList[iIndex / deviceSuffixes.size()];

    std::string deviceID = (devInfo.getNetworkProtocol() == DeviceInfo::PROTOCOL_TCP ? "tcp://" : "udp://")
                + devInfo.getIpAddress() + deviceSuffixes[iIndex % deviceSuffixes.size()];
    info.setString(deviceID);

    return info.query();
}

GC_ERROR Interface::getDeviceInfo(const char* sDeviceID, DEVICE_INFO_CMD iInfoCmd,
        INFO_DATATYPE* piType, void* pBuffer, size_t* piSize) {
    if(sDeviceID == nullptr) {
        return GC_ERR_INVALID_ID;
    }

    InfoQuery info(piType, pBuffer, piSize);
    switch(iInfoCmd) {
        case DEVICE_INFO_ID:
            info.setString(sDeviceID);
            break;
        case DEVICE_INFO_VENDOR:
            info.setString("Nerian Vision GmbH");
            break;
        case DEVICE_INFO_MODEL: {
                std::map<std::string, std::string>::const_iterator it = deviceIDToModelName.find(sDeviceID);
                if (it != deviceIDToModelName.end()) {
                    info.setString(it->second);
                } else {
                    info.setString("Nerian device"); // should not happen
                }
                break;
            }
        case DEVICE_INFO_TLTYPE:
            info.setString("Ethernet");
            break;
        case DEVICE_INFO_DISPLAYNAME:
            info.setString(sDeviceID);
            break;
        case DEVICE_INFO_ACCESS_STATUS:
            if(!checkDeviceInUse(sDeviceID)) {
                info.setInt(DEVICE_ACCESS_STATUS_READWRITE);
            } else {
                info.setInt(DEVICE_ACCESS_STATUS_BUSY);
            }
            break;
        //case DEVICE_INFO_SERIAL_NUMBER:
        //    info.setString(sDeviceID);
        //    break;
        case DEVICE_INFO_USER_DEFINED_NAME:
        case DEVICE_INFO_VERSION:
            return GC_ERR_NOT_AVAILABLE;
        case DEVICE_INFO_TIMESTAMP_FREQUENCY:
            info.setUInt64(1000000);
            break;
        default:
            return GC_ERR_NOT_IMPLEMENTED;
    }

    return info.query();
}


GC_ERROR Interface::getParentTL(TL_HANDLE* phSystem) {
    if(phSystem == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    } else {
        *phSystem = reinterpret_cast<TL_HANDLE>(system);
        return GC_ERR_SUCCESS;
    }
}

GC_ERROR Interface::getInfo(INTERFACE_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
        void* pBuffer, size_t* piSize) {

    InfoQuery info(piType, pBuffer, piSize);
    switch(iInfoCmd) {
        case INTERFACE_INFO_ID:
            info.setString("eth");
            break;
        case INTERFACE_INFO_DISPLAYNAME:
            info.setString("Ethernet");
            break;
        case INTERFACE_INFO_TLTYPE:
            info.setString("Ethernet");
            break;
        case INTERFACE_INFO_CUSTOM_ID + 0: {
            // Maximum device selector ID
            info.setInt(static_cast<int>(deviceList.size()*deviceSuffixes.size()) - 1);
            break;
        }
        default:
            ; // Nothing to do
    }

    return info.query();
}

bool Interface::parseDeviceUrl(const char* sDeviceID, bool& udp, std::string& host, std::string& stream) {
    // Device URL have the format:
    // [protocol]://[host]:[port]/[stream]

    std::string id(sDeviceID);
    std::string protocol = id.substr(0, 6);
    host = id.substr(6, id.rfind('/') - 6);
    stream = id.substr(id.rfind('/') + 1);

    if(protocol == "udp://") {
        udp = true;
    } else if(protocol == "tcp://") {
        udp = false;
    } else {
        return false;
    }

    if(host == "") {
        return false;
    } else {
        return true;
    }
}

GC_ERROR Interface::openDevice(const char* sDeviceID, DEVICE_ACCESS_FLAGS iOpenFlags,
        DEV_HANDLE* phDevice) {
    if(sDeviceID == nullptr || strlen(sDeviceID) <= 6) {
        return GC_ERR_INVALID_ID;
    }

    if(checkDeviceInUse(sDeviceID)) {
        return GC_ERR_RESOURCE_IN_USE;
    }

    // Parse the URL
    bool udp = true;
    std::string host, stream;
    if(!parseDeviceUrl(sDeviceID, udp, host, stream)) {
        return GC_ERR_INVALID_ID;
    }

    // Get index for logical device
    int index = PhysicalDevice::logicalIdToIndex(stream.c_str());
    if(index < 0) {
        return GC_ERR_INVALID_ID;
    }

    PhysicalDevice* physicalDevice = system->findPhysicalDeviceByAddress(
        udp, host.c_str());

    if(physicalDevice == nullptr) {
        // We could not find an existing physical device. We have to open one
        std::shared_ptr<PhysicalDevice> device(new PhysicalDevice(this));
        if(device->open(udp, host.c_str()) == GC_ERR_SUCCESS) {
            system->addPhysicalDevice(device);
            physicalDevice = device.get();
        } else {
            return GC_ERR_IO;
        }
    }

    physicalDevice->getLogicalDevice(index)->open();
    *phDevice = reinterpret_cast<DEV_HANDLE>(physicalDevice->getLogicalDevice(index));

    return GC_ERR_SUCCESS;
}

bool Interface::checkDeviceInUse(const char* sDeviceID) {
    bool udp = true;
    std::string host, stream;
    if(!parseDeviceUrl(sDeviceID, udp, host, stream)) {
        return false;
    }

    PhysicalDevice* physicalDevice = system->findPhysicalDeviceByAddress(
        udp, host.c_str());

    if(physicalDevice != nullptr) {
        int logicalIndex = PhysicalDevice::logicalIdToIndex(stream.c_str());
        if(logicalIndex < 0) {
            return false; // Invalid id;
        }

        return physicalDevice->getLogicalDevice(logicalIndex)->isOpen();
    } else {
        return false;
    }
}

}
