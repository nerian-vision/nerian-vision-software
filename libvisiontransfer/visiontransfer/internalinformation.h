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

#ifndef VISIONTRANSFER_INTERNALINFORMATION_H
#define VISIONTRANSFER_INTERNALINFORMATION_H

namespace visiontransfer {
namespace internal {

/**
 * \brief Information that is required internally by different program
 * parts.
 */
struct InternalInformation {
#pragma pack(push,1)
    struct DiscoveryMessageBasic {
        unsigned char protocolVersion;
        unsigned char model;
        unsigned char useTcp;
        char firmwareVersion[14];
    };
    struct DiscoveryMessage: public DiscoveryMessageBasic {
        // Extended device status / health info
        double lastFps; // Most recent FPS report, or 0.0 if N/A
        unsigned int jumboSize; // Jumbo MTU or 0 if disabled
        char currentCaptureSource[8]; // For targeted debug instructions
    };
#pragma pack(pop)

    static const char DISCOVERY_BROADCAST_MSG[16];

    static constexpr int DISCOVERY_BROADCAST_PORT = 7680;
    static constexpr int IMAGEDATA_PORT = 7681;
    static constexpr int WEBSOCKET_PORT = 7682;
    static constexpr int PARAMETER_PORT = 7683;
    static constexpr int DATACHANNELSERVICE_PORT = 7684;
    static constexpr int PARAMETER_WEBSOCKET_PORT = 7685;

    static constexpr unsigned char CURRENT_PROTOCOL_VERSION = 0x06;
    static constexpr unsigned char CURRENT_PARAMETER_PROTOCOL_VERSION_MAJOR = 0x07;
    static constexpr unsigned char CURRENT_PARAMETER_PROTOCOL_VERSION_MINOR = 0x01;

};

}} // namespace

#endif
