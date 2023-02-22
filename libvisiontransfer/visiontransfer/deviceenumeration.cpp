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

#include <cstring>

#include "visiontransfer/deviceenumeration.h"
#include "visiontransfer/exceptions.h"
#include "visiontransfer/networking.h"
#include "visiontransfer/internalinformation.h"

using namespace std;
using namespace visiontransfer;
using namespace visiontransfer::internal;

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
    // All initialization in the pimpl class
}

DeviceEnumeration::~DeviceEnumeration() {
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
    sendDiscoverBroadcast();
    deviceList = collectDiscoverResponses();

    // Convert vector to simple pointer
    *numDevices = deviceList.size();
    return deviceList.data();
}

void DeviceEnumeration::Pimpl::sendDiscoverBroadcast() {
    std::vector<sockaddr_in> addresses = findBroadcastAddresses();
    for(sockaddr_in addr: addresses) {
        addr.sin_port = htons(InternalInformation::DISCOVERY_BROADCAST_PORT);

        if (sendto(sock, InternalInformation::DISCOVERY_BROADCAST_MSG,
                sizeof(InternalInformation::DISCOVERY_BROADCAST_MSG)-1, 0,
                (struct sockaddr *) &addr, sizeof(addr))
                != sizeof(InternalInformation::DISCOVERY_BROADCAST_MSG)-1) {
            throw std::runtime_error("Error sending broadcast message");
        }
    }
}

DeviceEnumeration::DeviceList DeviceEnumeration::Pimpl::collectDiscoverResponses() {
    DeviceList ret;

    while(true) {
        InternalInformation::DiscoveryMessage msg;
        sockaddr_in senderAddress;
        socklen_t senderLength = sizeof(senderAddress);

        int received = recvfrom(sock, reinterpret_cast<char*>(&msg), sizeof(msg),
            0, (sockaddr *)&senderAddress, &senderLength);

        if(received < 0) {
            // There are no more replies
            break;
        }
        bool isLegacy = received == sizeof(InternalInformation::DiscoveryMessageBasic);
        if((received != sizeof(msg)) && !isLegacy ) {
            // Invalid message
            continue;
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

        // Add to result list
        DeviceInfo info(
            inet_ntoa(senderAddress.sin_addr),
            msg.useTcp ? DeviceInfo::PROTOCOL_TCP : DeviceInfo::PROTOCOL_UDP,
            fwVersion,
            (DeviceInfo::DeviceModel)msg.model,
            msg.protocolVersion == InternalInformation::CURRENT_PROTOCOL_VERSION,
            status
        );
        ret.push_back(info);
    }

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
                ret.push_back(*reinterpret_cast<sockaddr_in*>(p->ifa_dstaddr));
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

