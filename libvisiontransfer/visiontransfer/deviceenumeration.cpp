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

#include <cstdint>
#include <cstring>
#include <sstream>
#include <chrono>
#include <thread>

#include "visiontransfer/deviceenumeration.h"
#include "visiontransfer/exceptions.h"
#include "visiontransfer/internal/networking.h"
#include "visiontransfer/internal/internalinformation.h"

using namespace std;
using namespace visiontransfer;
using namespace visiontransfer::internal;


// DEBUG OUTPUT
#ifdef _WIN32
#include <fstream>
    std::fstream debugStreamDeviceEnum("C:\\debug\\visiontransfer-device-enumeration-" + std::to_string(time(nullptr)) + ".txt", std::ios::out);
#else
#include <iostream>
    std::ostream& debugStreamDeviceEnum = std::cout;
#endif
std::chrono::system_clock::time_point debugStreamDeviceEnumInitTime = std::chrono::system_clock::now();
#define DEBUG_DEVICE_ENUM_THREAD_ID " (thread " << std::this_thread::get_id() << ") "
#define DEBUG_DEVICE_ENUM(x) debugStreamDeviceEnum << std::dec << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - debugStreamDeviceEnumInitTime).count() << ": " << DEBUG_DEVICE_ENUM_THREAD_ID << x << std::endl;

namespace visiontransfer {

/*************** Pimpl class containing all private members ***********/

class DeviceEnumeration::Pimpl {
public:
    Pimpl();
    ~Pimpl();
    DeviceInfo* getDevicesPointer(int* numDevices);

private:
    static constexpr int RESPONSE_WAIT_TIME_MS = 50;
    SOCKET sock;
    std::vector<DeviceInfo> deviceList;

    std::vector<sockaddr_in> findBroadcastAddresses();
    void sendDiscoverBroadcast();
    DeviceEnumeration::DeviceList collectDiscoverResponses();
};

/******************** Stubs for all public members ********************/

DeviceEnumeration::DeviceEnumeration():
        pimpl(new Pimpl()) {
    DEBUG_DEVICE_ENUM("DeviceEnumeration()");
    // All initialization in the pimpl class
}

DeviceEnumeration::~DeviceEnumeration() {
    DEBUG_DEVICE_ENUM("~DeviceEnumeration()");
    delete pimpl;
}

DeviceInfo* DeviceEnumeration::getDevicesPointer(int* numDevices) {
    return pimpl->getDevicesPointer(numDevices);
}

/******************** Implementation in pimpl class *******************/

DeviceEnumeration::Pimpl::Pimpl() {
    Networking::initNetworking();

    // Create socket
    if((sock = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        TransferException ex("Error creating broadcast socket: " + Networking::getLastErrorString());
        throw ex;
    }

    // Set broadcast flag
    int broadcastPermission = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&broadcastPermission),
            sizeof(broadcastPermission)) < 0) {
        TransferException ex("Error setting socket broadcast flag: " + Networking::getLastErrorString());
        throw ex;
    }

    // Set sending and receive timeouts
#ifdef _WIN32
    unsigned int timeout = RESPONSE_WAIT_TIME_MS;
#else
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = RESPONSE_WAIT_TIME_MS*1000;
#endif

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));
}

DeviceEnumeration::Pimpl::~Pimpl() {
    close(sock);
}

DeviceInfo* DeviceEnumeration::Pimpl::getDevicesPointer(int* numDevices) {
    DEBUG_DEVICE_ENUM("Started discovery");
    sendDiscoverBroadcast();
    deviceList = collectDiscoverResponses();

    // Convert vector to simple pointer
    *numDevices = (int) deviceList.size();
    return deviceList.data();
}

void DeviceEnumeration::Pimpl::sendDiscoverBroadcast() {
    bool hadError = false;
    std::stringstream ss;
    ss << "Error sending broadcast message:";
    std::vector<sockaddr_in> addresses = findBroadcastAddresses();
    for(sockaddr_in addr: addresses) {
        addr.sin_port = htons(InternalInformation::DISCOVERY_BROADCAST_PORT);

        char* ipStr = inet_ntoa(addr.sin_addr);

        int sendResult = (int) sendto(sock, InternalInformation::DISCOVERY_BROADCAST_MSG,
                sizeof(InternalInformation::DISCOVERY_BROADCAST_MSG)-1, 0,
                (struct sockaddr *) &addr, sizeof(addr));
        if (sendResult != sizeof(InternalInformation::DISCOVERY_BROADCAST_MSG)-1) {
            hadError = true;
            DEBUG_DEVICE_ENUM("Broadcast FAIL on " << ipStr);
        } else {
            DEBUG_DEVICE_ENUM("Broadcast OK on " << ipStr);
        }
        ss << " " << ipStr << "(" << Networking::getLastErrorString() << "/" << sendResult << ")";
    }
    if (hadError) {
        throw std::runtime_error(ss.str());
    }
}

DeviceEnumeration::DeviceList DeviceEnumeration::Pimpl::collectDiscoverResponses() {
    DeviceList ret;

    constexpr long MAX_MS_WAIT_FOR_REPLIES = 500;

    DEBUG_DEVICE_ENUM("Collecting responses for " << MAX_MS_WAIT_FOR_REPLIES << " msec");
    std::chrono::steady_clock::time_point tStart = std::chrono::steady_clock::now();
    while(true) {
        InternalInformation::DiscoveryMessage msg;
        sockaddr_in senderAddress;
        socklen_t senderLength = sizeof(senderAddress);

        int received = recvfrom(sock, reinterpret_cast<char*>(&msg), sizeof(msg),
            0, (sockaddr *)&senderAddress, &senderLength);

        long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tStart).count();

        if(received < 0) {
            // There are no more replies
            if (elapsed > MAX_MS_WAIT_FOR_REPLIES) break; // Maximum collection time exceeded
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        bool isLegacy = received == (int) sizeof(InternalInformation::DiscoveryMessageBasic);

        char* ip_addr = inet_ntoa(senderAddress.sin_addr);

        DEBUG_DEVICE_ENUM("Received reply from " << ip_addr << " at " << elapsed << " msec");

        bool isLegacyWithStatusInfo = received == (int) sizeof(InternalInformation::DiscoveryMessageWithStatus);
        if(!(isLegacy||isLegacyWithStatusInfo)) {
            if  ( ((received < (int) sizeof(InternalInformation::DiscoveryMessageExtensibleV0)))
               || ((received < (int) sizeof(InternalInformation::DiscoveryMessageExtensibleV1)) && (msg.discoveryExtensionVersion >= 0x01))
                ) {
                // Malformed message, truncated relative to reported format
                DEBUG_DEVICE_ENUM("  (rejected malformed reply)");
                continue;
            }
        }

        // Zero terminate version string
        char fwVersion[sizeof(msg.firmwareVersion)+1];
        memcpy(fwVersion, msg.firmwareVersion, sizeof(msg.firmwareVersion));
        fwVersion[sizeof(msg.firmwareVersion)] = '\0';

        DeviceStatus status;
        if (!isLegacy) {
            // Construct health status report
            status = DeviceStatus(msg.lastFps, msg.jumboSize, msg.currentCaptureSource);
        }

        // Fallback for undefined fields
        std::string serial = "N/A";

        if (!(isLegacy||isLegacyWithStatusInfo)) {
            // Parse extension fields up to maximum supported reported version
            if (msg.discoveryExtensionVersion >= 0x01) {
                serial = std::string(msg.serialNumber);
            }
            // [Append subsequent extension levels here]

            if (msg.discoveryExtensionVersion > InternalInformation::CURRENT_DISCOVERY_EXTENSION_VERSION) {
                // Device is sending more fields than we know of (library not up-to-date)
                //
                // Could warn here (but should continue)
            }
        }

        // Add to result list
        DeviceInfo info(
            ip_addr,
            msg.useTcp ? DeviceInfo::PROTOCOL_TCP : DeviceInfo::PROTOCOL_UDP,
            fwVersion,
            (DeviceInfo::DeviceModel)msg.model,
            msg.protocolVersion == InternalInformation::CURRENT_PROTOCOL_VERSION,
            serial,
            status
        );
        ret.push_back(info);
    }

    DEBUG_DEVICE_ENUM("# of received responses: " << ret.size());
    DEBUG_DEVICE_ENUM("-----------------------------");
    return ret;
}

std::vector<sockaddr_in> DeviceEnumeration::Pimpl::findBroadcastAddresses() {
    std::vector<sockaddr_in> ret;

#ifndef _WIN32
    // BSD-style implementation
    struct ifaddrs * ifap;
    if (getifaddrs(&ifap) == 0) {
        struct ifaddrs * p = ifap;
        while(p) {
            if(p->ifa_dstaddr != nullptr && p->ifa_dstaddr->sa_family == AF_INET) {
                sockaddr_in* sinp = reinterpret_cast<sockaddr_in*>(p->ifa_dstaddr);
                const unsigned char* ipParts = reinterpret_cast<const unsigned char*>(&(sinp->sin_addr.s_addr));
                if (!(  (ipParts[0]==127 && ipParts[1]==0 && ipParts[2]==0 && ipParts[3]==1)  // exclude loopback 127.0.0.1
                     || (ipParts[0]==169 && ipParts[1]==254)                                  // exclude link-local 169.254.x.x
                    )) {
                    ret.push_back(*sinp);
                }
            }
            p = p->ifa_next;
        }
        freeifaddrs(ifap);
    }
#else
    // Windows XP style implementation

    // Adapted from example code at http://msdn2.microsoft.com/en-us/library/aa365917.aspx
    // Now get Windows' IPv4 addresses table.  We gotta call GetIpAddrTable()
    // multiple times in order to deal with potential race conditions properly.
    MIB_IPADDRTABLE*  ipTable = nullptr;
    ULONG bufLen = 0;
    for (int i=0; i<5; i++) {
        DWORD ipRet = GetIpAddrTable(ipTable, &bufLen, false);
        if (ipRet == ERROR_INSUFFICIENT_BUFFER) {
            if(ipTable != nullptr) {
                delete []reinterpret_cast<unsigned char*>(ipTable);  // in case we had previously allocated it
            }
            ipTable = reinterpret_cast<MIB_IPADDRTABLE *>(new unsigned char[bufLen]);
            memset(ipTable, 0, bufLen);
        } else if (ipRet == NO_ERROR) {
            break;
        } else {
            if(ipTable != nullptr) {
                delete []reinterpret_cast<unsigned char*>(ipTable);
            }
            break;
        }
    }

    if (ipTable != nullptr) {
        for (DWORD i=0; i<ipTable->dwNumEntries; i++) {
            const MIB_IPADDRROW & row = ipTable->table[i];

            uint32_t ipAddr  = row.dwAddr;
            const unsigned char* ipParts = reinterpret_cast<const unsigned char*>(&ipAddr);
            if (    (ipParts[0]==127 && ipParts[1]==0 && ipParts[2]==0 && ipParts[3]==1)  // exclude loopback 127.0.0.1
                 || (ipParts[0]==169 && ipParts[1]==254)                                  // exclude link-local 169.254.x.x
                ) {
                continue;
            }
            uint32_t netmask = row.dwMask;
            uint32_t baddr   = ipAddr & netmask;
            if (row.dwBCastAddr) {
                baddr |= ~netmask;
            }

            sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = baddr;
            ret.push_back(addr);
        }

        delete []reinterpret_cast<unsigned char*>(ipTable);
    }
#endif

    return ret;
}

} // namespace

