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

#ifndef NERIAN_PORT_H
#define NERIAN_PORT_H

#include "misc/common.h"
#include "misc/handle.h"

#include <genicam/gentl.h>
#include <string>
#include <functional>

namespace GenTL {

class PortImplementation;

/*
 * Represents a port and encapsulates all port-related GenTL functions.
 */
class Port: public Handle {
public:
    Port(const char* id, const char* fileName, const char* portName,
        const char* module, PortImplementation* implementation);
	
	GC_ERROR setSelector(int value);

    // Methods that match functions form the GenTL interface
    GC_ERROR getPortInfo( PORT_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
        void* pBuffer, size_t* piSize);
    GC_ERROR getPortURL(char* sURL, size_t* piSize);
    GC_ERROR getNumPortURLs(uint32_t* piNumURLs);
    GC_ERROR getPortURLInfo(uint32_t iURLIndex, URL_INFO_CMD iInfoCmd,
        INFO_DATATYPE* piType, void* pBuffer, size_t* piSize);
    GC_ERROR readPort(uint64_t iAddress, void* pBuffer, size_t* piSize);
    GC_ERROR writePort(uint64_t iAddress, const void* pBuffer, size_t* piSize);
    GC_ERROR writePortStacked(PORT_REGISTER_STACK_ENTRY* pEntries, size_t* piNumEntries);
    GC_ERROR readPortStacked(PORT_REGISTER_STACK_ENTRY* pEntries, size_t* piNumEntries);

private:
    enum Addresses: uint64_t {
        FILE_ADDRESS = 0xF0000000,
        FEATURE_ADDRESS = 0xE0000000,
        SELECTOR_ADDRESS = 0xD0000000,
        CHILD_FEATURE_ADDRESS = 0xC0000000
    };

    std::string id;
    std::string fileName;
    std::string portName;
    std::string module;
    unsigned int selector;
    PortImplementation* implementation;

    GC_ERROR readXmlFromPort(uint64_t iAddress, void* pBuffer, size_t* piSize);
    GC_ERROR readFromFeature(uint64_t baseAddress, uint64_t iAddress, void* pBuffer, size_t* piSize,
        std::function<GC_ERROR(int32_t command, void* pBuffer, size_t* piSize)> readFunc);
};

}
#endif
