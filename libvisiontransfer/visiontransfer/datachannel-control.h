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

#ifndef VISIONTRANSFER_DATACHANNEL_CONTROL_H
#define VISIONTRANSFER_DATACHANNEL_CONTROL_H

#include <cstring>
#include <memory>
#include <map>
#include <set>
#include <vector>

#include <visiontransfer/datachannelservicebase.h>

namespace visiontransfer {
namespace internal {

/**
 * \brief Commands understood on DataChannelService channel 0 (control)
 */
class DataChannelControlCommands {
public:
    enum Command {
        CTLReserved,
        CTLRequestAdvertisement,
        CTLProvideAdvertisement,
        CTLRequestSubscriptions,
        CTLProvideSubscriptions,
        CTLRequestUnsubscriptions,
        CTLProvideUnsubscriptions
    };
};

/**
 * \brief Internal helpers for packing and unpacking channel 0 service messages
 */
class DataChannelControlUtil {
public:
    static DataChannelControlCommands::Command getCommand(unsigned char* data, int datalen) {
        if (datalen < 2) throw std::runtime_error("Buffer too small");
        return (DataChannelControlCommands::Command) ntohs(*((uint16_t*) data));
    }
    // Advertisements (available services)
    static int packAdvertisementMessage(unsigned char* data, int datalen, DataChannelControlCommands::Command cmd, const std::map<DataChannel::ID, std::shared_ptr<DataChannel> >& channels) {
        int origDataLen = datalen;
        if (datalen < 3) throw std::runtime_error("Buffer too small");
        *((uint16_t*)data) = htons(cmd);
        uint8_t num = (uint8_t) std::min(255, (int) channels.size()); // pack 255 items max
        data[2] = num;
        // payload
        data += 3; datalen -= 3;
        int i = 0;
        for (auto kv: channels) {
            i++; if (i>num) break;
            if (datalen < 3) throw std::runtime_error("Buffer too small");
            auto p = kv.second;
            const std::string& infoString = p->getInfoString();
            uint8_t strSize = (uint8_t) std::min(255, (int) infoString.size());
            int elemLen = 1 + 1 + 1 + strSize;
            if (datalen < elemLen) throw std::runtime_error("Buffer too small");
            data[0] = p->getChannelID();
            data[1] = p->getChannelType();
            data[2] = strSize;
            std::memcpy(data + 3, infoString.c_str(), strSize);
            data += elemLen; datalen -= elemLen;
        }
        return (origDataLen - datalen);
    }
    static std::vector<DataChannelInfo> unpackAdvertisementMessage(unsigned char* data, int datalen) {
        std::vector<DataChannelInfo> result;
        if (datalen < 3) throw std::runtime_error("Buffer too small");
        uint8_t num = data[2];
        data += 3; datalen -= 3;
        for (int i=0; i<num; ++i) {
            if (datalen < 3) throw std::runtime_error("Buffer too small");
            uint8_t id = data[0];
            uint8_t type = data[1];
            uint8_t strSize = data[2];
            int elemLen = 1 + 1 + 1 + strSize;
            if (datalen < elemLen) throw std::runtime_error("Buffer too small");
            result.emplace_back(DataChannelInfo((DataChannel::ID) id, (DataChannel::Type) type, std::string(data[3], strSize)));
            data += elemLen; datalen -= elemLen;
        }
        return result;
    }

    // Subscriptions (connected services)
    static int packSubscriptionMessage(unsigned char* data, int datalen, DataChannelControlCommands::Command cmd, const std::vector<DataChannel::ID>& subscriptions) {
        if (datalen < 4) throw std::runtime_error("Buffer too small");
        *((uint16_t*)data) = htons(cmd);
        uint8_t num = (uint8_t) std::min(255, (int) subscriptions.size());
        data[2] = num; // pack 255 items max
        data += 3; datalen -= 3;
        if (datalen < (1*num)) throw std::runtime_error("Buffer too small");
        for (int i=0; i<num; ++i) {
            auto p = subscriptions[i];
            data[0] = p;
            data += 1; datalen -= 1;
        }
        return (2+1+1*num);
    }
    static std::vector<DataChannel::ID> unpackSubscriptionMessage(unsigned char* data, int datalen) {
        std::vector<DataChannel::ID> result;
        if (datalen < 3) throw std::runtime_error("Buffer too small");
        uint8_t num = data[2];
        data += 3; datalen -= 3;
        if (datalen < (1*num)) throw std::runtime_error("Buffer too small");
        for (int i=0; i<num; ++i) {
            result.emplace_back(static_cast<DataChannel::ID>(data[0]));
            data += 1; datalen -= 1;
        }
        return result;
    }
};

}} // namespaces

#endif

