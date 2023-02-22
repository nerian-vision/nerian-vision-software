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

#include <memory>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#ifndef VISIONTRANSFER_DATACHANNELSERVICEBASE_H
#define VISIONTRANSFER_DATACHANNELSERVICEBASE_H

namespace visiontransfer {
namespace internal {

/*
 * This is a header file for internal use, please use the wrappers from datachannelservice.h
 */

#pragma pack(push,1)
/**
 * \brief Transport-level DataChannel header
 */
struct DataChannelMessageHeader {
    uint8_t channelID;
    uint8_t channelType;
    uint32_t payloadSize;
};
struct DataChannelMessage {
    DataChannelMessageHeader header;
    unsigned char* payload;
};
#pragma pack(pop)



class DataChannelServiceBase;
/**
 * \brief Base class all data channel services derive from (once on the server side, once on the API side)
 */
class DataChannel {
public:
    /**
     * \brief Known data channel service types, not all may be active on a specific device
     */
    struct Types {
        enum DataChannelTypesEnum {
            CONTROL             = 0x00,
            BNO080              = 0x01,
            UNDEFINED           = 0xff
        };
    };

    typedef unsigned char Type;
    typedef unsigned char ID;

    inline DataChannel(): infoString("RESERVED") { }
    inline virtual ~DataChannel() {}
    inline ID getChannelID() const { return channelID; }
    inline std::string getInfoString() const { return infoString; }
    inline void setService(std::weak_ptr<DataChannelServiceBase> serv) { service = serv; }
    inline void setChannelID(ID id) { channelID = id; }

    virtual Type getChannelType() const = 0;
    /// \brief Channel-dependent message handlers in respective channel implementations
    virtual int handleMessage(DataChannelMessage& message, sockaddr_in* sender) = 0;
    /// \brief When initialize() implementations return false, the service will be deactivated
    virtual bool initialize() = 0;
    /// \brief startService() implementations can start devices, launch an IO-blocked worker thread etc.
    virtual int startService() = 0;
    /** \brief A single processing iteration; should be short and must not block.
     *  Actual frequency determined by the thread calling DataChannelServiceBase::process()
     */
    virtual bool process() { return true; }
    virtual int stopService() = 0;
protected:
    std::string infoString;
    int sendData(unsigned char* data, unsigned int dataSize, sockaddr_in* recipient=nullptr);
private:
    ID channelID;
    std::weak_ptr<DataChannelServiceBase> service;
};

/**
 * \brief API-level data channel info for advertisements and subscription accounting
 */
class DataChannelInfo {
public:
    inline DataChannelInfo(DataChannel::ID id, DataChannel::Type type, const std::string& info): channelID(id), channelType(type), infoString(info) { }
    inline DataChannel::ID   getChannelID() const { return channelID; }
    inline DataChannel::Type getChannelType() const { return channelType; }
    inline std::string     getInfoString() const { return infoString; }
private:
    DataChannel::ID channelID;
    DataChannel::Type channelType;
    std::string infoString;
};



/**
 * \brief Base class for the data service (background sending and receiving, dispatching to channels)
 */
class DataChannelServiceBase: public std::enable_shared_from_this<DataChannelServiceBase> {
public:
    DataChannelServiceBase();
    ~DataChannelServiceBase();
    void process();
    DataChannel::ID registerChannel(std::shared_ptr<DataChannel> channel);
    virtual int sendDataInternal(unsigned char* compiledMessage, unsigned int messageSize, sockaddr_in* recipient);
    int sendDataIsolatedPacket(DataChannel::ID id, DataChannel::Type type, unsigned char* data, unsigned int dataSize, sockaddr_in* recipient);
    virtual int handleChannel0Message(DataChannelMessage& message, sockaddr_in* sender) = 0;
protected:
    std::map<DataChannel::ID, std::shared_ptr<DataChannel> > channels;
#ifdef _WIN32
    SOCKET dataChannelSocket;
#else
    int dataChannelSocket;
#endif
    DataChannelMessage message;
};


}} // namespaces

#endif

