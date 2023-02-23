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

#include "misc/port.h"
#include "misc/infoquery.h"
#include "misc/xmlfiles.h"
#include "misc/portimplementation.h"

#include "event/eventqueue.h" // for FeatureStringType

#include <genicam/gentl.h>
#include <cstring>
#include <algorithm>

#include <fstream>
#include <iostream>

using namespace std;

namespace GenTL {

using namespace std::placeholders;

#ifdef ENABLE_DEBUGGING
#ifdef _WIN32
    std::fstream debugStreamPort("C:\\debug\\gentl-debug-port-" + std::to_string(time(nullptr)) + ".txt", std::ios::out);
#else
    std::ostream& debugStreamPort = std::cout;
#endif
#define DEBUG_PORT(x) debugStreamPort << x << std::endl;
#else
#define DEBUG_PORT(x) ;
#endif

Port::Port(const char* id, const char* fileName, const char* portName, const char* moduleName,
        PortImplementation* implementation)
    :Handle(TYPE_PORT), id(id), fileName(fileName), portName(portName), moduleName(moduleName),
        selector(0), implementation(implementation), featureInvalidateEvent(nullptr) {
}
Port::~Port() {
    freeFeatureInvalidateEvent();
}

Event* Port::allocFeatureInvalidateEvent() {
    if(featureInvalidateEvent != nullptr) {
        return nullptr;
    } else {
        featureInvalidateEvent = new Event(EVENT_FEATURE_INVALIDATE, sizeof(EventQueue::FeatureStringType), nullptr);
        return featureInvalidateEvent;
    }
}

void Port::freeFeatureInvalidateEvent() {
    if(featureInvalidateEvent != nullptr) {
        delete featureInvalidateEvent;
        featureInvalidateEvent = nullptr;
    }
}

void Port::emitFeatureInvalidateEvent(const std::string& featureName) {
    if(featureInvalidateEvent != nullptr) {
        EventQueue::FeatureStringType buf; //MAX_LEN_FEATURE_NAME
        int len = featureName.length();
        if (len >= sizeof(buf)) len = sizeof(buf)-1;
        std::strncpy(buf.str, featureName.c_str(), len+1);
        buf.str[sizeof(buf)-1] = 0;
        DEBUG_PORT("Emitting (to queue) FEATURE_INVALIDATE for " << featureName << " -> DATA_ID=\"" << buf.str << "\"");
        featureInvalidateEvent->emitEvent(buf);
    }
}


GC_ERROR Port::setSelector(int value) {
    if(implementation != nullptr) {
        GC_ERROR err = implementation->writeSelector(value);
        if(err == GC_ERR_SUCCESS) {
            selector = value;
        }
        return err;
    } else {
        return GC_ERR_INVALID_ADDRESS;
    }
}

GC_ERROR Port::getPortInfo(PORT_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
        void* pBuffer, size_t* piSize) {
    InfoQuery info(piType, pBuffer, piSize);
    switch(iInfoCmd) {
        case PORT_INFO_ID:
            info.setString(id);
            break;
        case PORT_INFO_VENDOR:
            info.setString("Nerian_Vision_Technologies");
            break;
        case PORT_INFO_MODEL:
            info.setString("nerian");
            break;
        case PORT_INFO_TLTYPE:
            info.setString("Ethernet");
            break;
        case PORT_INFO_MODULE:
            info.setString(moduleName);
            break;
        case PORT_INFO_LITTLE_ENDIAN:
            info.setBool(true);
            break;
        case PORT_INFO_BIG_ENDIAN:
            info.setBool(false);
            break;
        case PORT_INFO_ACCESS_READ:
            info.setBool(true);
            break;
        case PORT_INFO_ACCESS_WRITE:
            info.setBool(true);
            break;
        case PORT_INFO_ACCESS_NA:
            info.setBool(false);
            break;
        case PORT_INFO_ACCESS_NI:
            info.setBool(false);
            break;
        case PORT_INFO_VERSION:
            info.setString("1.0");
            break;
        case PORT_INFO_PORTNAME:
            info.setString(portName);
            break;
        default:
            ; // Nothing to do
    }

    return info.query();
}

GC_ERROR Port::getPortURL(char* sURL, size_t* piSize) {
    if (piSize == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    char fileLen[9];
    snprintf(fileLen, sizeof(fileLen), "%x", static_cast<unsigned int>(XmlFiles::getFileContent(fileName).length()));

    char address[9];
    snprintf(address, sizeof(address), "%x", static_cast<unsigned int>(FILE_ADDRESS));

    std::string url = std::string("local:") + fileName + ";" + address + ";" + fileLen + "?SchemaVersion=1.0.0";
    size_t size = url.length() + 2;

    if (*piSize < size && sURL != nullptr) {
        return GC_ERR_BUFFER_TOO_SMALL;
    }

    if(sURL != nullptr) {
        memcpy(sURL, url.c_str(), url.length() + 1);
        sURL[url.length() + 1] = '\0'; // Extra '\0' for list termination.
    }

    *piSize = size;
    return GC_ERR_SUCCESS;
}

GC_ERROR Port::getNumPortURLs(uint32_t* piNumURLs) {
    if(piNumURLs == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    } else {
        *piNumURLs = 1;
        return GC_ERR_SUCCESS;
    }
}

GC_ERROR Port::getPortURLInfo(uint32_t iURLIndex, URL_INFO_CMD iInfoCmd,
        INFO_DATATYPE* piType, void* pBuffer, size_t* piSize) {

    int major = 0, minor = 0, subminor = 0;
    XmlFiles::getFileVersion(fileName, major, minor, subminor);

    InfoQuery info(piType, pBuffer, piSize);
    switch(iInfoCmd) {
        case URL_INFO_URL:
            *piType = INFO_DATATYPE_STRING;
            return getPortURL(reinterpret_cast<char*>(pBuffer), piSize);
        case URL_INFO_SCHEMA_VER_MAJOR:
            info.setInt(1);
            break;
        case URL_INFO_SCHEMA_VER_MINOR:
            info.setInt(1);
            break;
        case URL_INFO_FILE_VER_MAJOR:
            info.setInt(major);
            break;
        case URL_INFO_FILE_VER_MINOR:
            info.setInt(minor);
            break;
        case URL_INFO_FILE_VER_SUBMINOR:
            info.setInt(subminor);
            break;
        //case URL_INFO_FILE_SHA1_HASH: TODO
        case URL_INFO_FILE_REGISTER_ADDRESS:
            info.setUInt64(FILE_ADDRESS);
            break;
        case URL_INFO_FILE_SIZE:
            info.setUInt64(XmlFiles::getFileContent(fileName).length());
            break;
        case URL_INFO_SCHEME:
            info.setInt(URL_SCHEME_LOCAL);
            break;
        case URL_INFO_FILENAME:
            info.setString(fileName);
            break;
        default:
            ; // Nothing to do
    }

    return info.query();
}

GC_ERROR Port::readPort(uint64_t iAddress, void* pBuffer, size_t* piSize) {
    if(pBuffer == nullptr || piSize == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    };

    if(iAddress >= FILE_ADDRESS) {
        return readXmlFromPort(iAddress, pBuffer, piSize);
    } else if(iAddress >= FEATURE_ADDRESS && iAddress < FILE_ADDRESS) {
        return readFromFeature(FEATURE_ADDRESS, iAddress, pBuffer, piSize,
            std::bind(&PortImplementation::readFeature, implementation, _1, _2, _3));
    } else if(iAddress == SELECTOR_ADDRESS) {
        InfoQuery info(nullptr, pBuffer, piSize);
        info.setUInt(selector);
        return info.query();
    } else if(iAddress >= CHILD_FEATURE_ADDRESS && iAddress < SELECTOR_ADDRESS) {
        return readFromFeature(CHILD_FEATURE_ADDRESS, iAddress, pBuffer, piSize,
            std::bind(&PortImplementation::readChildFeature, implementation, selector, _1, _2, _3));
    } else {
        return GC_ERR_INVALID_ADDRESS;
    }
}

GC_ERROR Port::readFromFeature(uint64_t baseAddress, uint64_t iAddress, void* pBuffer, size_t* piSize,
        std::function<GC_ERROR(int32_t command, void* pBuffer, size_t* piSize)> readFunc) {
    memset(pBuffer, 0, *piSize);
    uint32_t featureId = static_cast<uint32_t>((iAddress - baseAddress) / 0x1000);
    uint32_t offset = static_cast<uint32_t>(iAddress % 0x1000);
    size_t readSize = *piSize;

    if(implementation != nullptr) {
        GC_ERROR err;
        if(offset == 0) {
            // Read full value
            err =   readFunc(featureId, pBuffer, &readSize);
        } else {
            // Partial read
            std::vector<unsigned char> buffer(*piSize + offset);
            readSize += offset;
            err =  readFunc(featureId, &buffer[0], &readSize);
            memcpy(pBuffer, &buffer[offset], *piSize);
        }

        if(err == GC_ERR_BUFFER_TOO_SMALL) {
            // Ignore this error
            return GC_ERR_SUCCESS;
        } else {
            return err;
        }
    } else {
        return GC_ERR_INVALID_ADDRESS;
    }
}

GC_ERROR Port::writeToFeature(uint64_t baseAddress, uint64_t iAddress, const void* pBuffer, size_t* piSize,
        std::function<GC_ERROR(int32_t command, const void* pBuffer, size_t* piSize)> writeFunc) {
    uint32_t featureId = static_cast<uint32_t>((iAddress - baseAddress) / 0x1000);
    uint32_t offset = static_cast<uint32_t>(iAddress % 0x1000);
    size_t writeSize = *piSize;

    if(implementation != nullptr) {
        GC_ERROR err;
        if(offset == 0) {
            // Relay to implementation
            err = writeFunc(featureId, pBuffer, &writeSize);
            return err; //return GC_ERR_SUCCESS;
        } else {
            // Partial write (would this even be needed for our purely register-like features?)
            DEBUG_PORT("Partial feature write not implemented (port.c)");
            return GC_ERR_INVALID_ADDRESS;
        }
    } else {
        return GC_ERR_INVALID_ADDRESS;
    }
}

GC_ERROR Port::readXmlFromPort(uint64_t iAddress, void* pBuffer, size_t* piSize) {
    int offset = static_cast<int>(iAddress - FILE_ADDRESS);
    int fileSize = static_cast<int>(XmlFiles::getFileContent(fileName).length());
    int toRead = std::min(static_cast<int>(*piSize), fileSize - offset);

    if(toRead < 0) {
        return GC_ERR_INVALID_ADDRESS;
    }

    std::memcpy(pBuffer, &XmlFiles::getFileContent(fileName).c_str()[offset], toRead);
    *piSize = toRead;

    return GC_ERR_SUCCESS;
}

GC_ERROR Port::writePort(uint64_t iAddress, const void* pBuffer, size_t* piSize) {
    if(iAddress == SELECTOR_ADDRESS) {
        // Write the selector
        if(piSize == nullptr || *piSize < sizeof(selector) || pBuffer == nullptr) {
            return GC_ERR_INVALID_PARAMETER;
        } else {
            unsigned int newSelector = *reinterpret_cast<const unsigned int*>(pBuffer);
            DEBUG_PORT("Write selector, ID " << id << ": " << newSelector);
            return setSelector(newSelector);
        }
    } else if(iAddress >= CHILD_FEATURE_ADDRESS && iAddress < SELECTOR_ADDRESS) {
        return writeToFeature(CHILD_FEATURE_ADDRESS, iAddress, pBuffer, piSize,
            std::bind(&PortImplementation::writeChildFeature, implementation, selector, _1, _2, _3));
    } else {
        return GC_ERR_INVALID_ADDRESS;
    }
}

GC_ERROR Port::writePortStacked(PORT_REGISTER_STACK_ENTRY* pEntries, size_t* piNumEntries) {
    if(pEntries == nullptr || piNumEntries == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    int successfulWrites = 0;
    for(unsigned int i=0; i<*piNumEntries; i++) {
        GC_ERROR result = writePort(pEntries[i].Address, pEntries[i].pBuffer, &pEntries[i].Size);
        if(result != GC_ERR_SUCCESS) {
            *piNumEntries = successfulWrites;
            return result;
        } else {
            successfulWrites++;
        }
    }
    *piNumEntries = successfulWrites;
    return GC_ERR_SUCCESS;
}

GC_ERROR Port::readPortStacked(PORT_REGISTER_STACK_ENTRY* pEntries, size_t* piNumEntries) {
    if(pEntries == nullptr || piNumEntries == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    int successfulReads = 0;
    for(unsigned int i=0; i<*piNumEntries; i++) {
        GC_ERROR result = readPort(pEntries[i].Address, pEntries[i].pBuffer, &pEntries[i].Size);
        if(result != GC_ERR_SUCCESS) {
            *piNumEntries = successfulReads;
            return result;
        } else {
            successfulReads++;
        }
    }
    *piNumEntries = successfulReads;
    return GC_ERR_SUCCESS;
}

}
