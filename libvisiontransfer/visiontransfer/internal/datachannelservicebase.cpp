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

#include <sys/types.h>
#include <cstring>
#include <stdexcept>
#include <fcntl.h>
#include <fstream>

#include <visiontransfer/internal/internalinformation.h>
#include <visiontransfer/internal/networking.h>
#include <visiontransfer/internal/datachannelservicebase.h>

#include <iostream>

using namespace visiontransfer;
using namespace visiontransfer::internal;

namespace visiontransfer {
namespace internal {

DataChannelServiceBase::DataChannelServiceBase() {
    // Create socket
    if((dataChannelSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        throw std::runtime_error("Error creating data channel service socket!");
    }

    Networking::enableReuseAddress(dataChannelSocket, true);

    // Bind to port
    sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(InternalInformation::DATACHANNELSERVICE_PORT);
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(::bind(dataChannelSocket, (sockaddr *)&localAddr, sizeof(localAddr)) != 0) {
        throw std::runtime_error("Error binding dataChannel socket!");
    }

    Networking::setSocketBlocking(dataChannelSocket, false);
}

DataChannelServiceBase::~DataChannelServiceBase() {
    Networking::closeSocket(dataChannelSocket);
}

void DataChannelServiceBase::process() {
    static unsigned char buffer[100000];
    static sockaddr_in senderAddress;
    static socklen_t senderLength = (socklen_t) sizeof(senderAddress);

    int received;
    while (true) {
        // socket is non-blocking
        received = recvfrom(dataChannelSocket, (char*) buffer, sizeof(buffer), 0, (sockaddr *)&senderAddress, &senderLength);
        if ((received > 0) && ((unsigned)received >= sizeof(DataChannelMessageHeader))) {
            DataChannelMessageHeader* raw = reinterpret_cast<DataChannelMessageHeader*>(buffer);
            DataChannelMessage msg;
            msg.header.channelID = (DataChannel::ID) raw->channelID;
            msg.header.channelType = (DataChannel::Type) raw->channelType;
            msg.header.payloadSize = ntohl(raw->payloadSize);
            msg.payload = buffer + sizeof(DataChannelMessageHeader);
            if ((sizeof(DataChannelMessageHeader) + msg.header.payloadSize) != (unsigned) received) {
                std::cerr << "DataChannelServiceBase: Size mismatch in UDP message, type " << (int) msg.header.channelType << " ID " << (int) msg.header.channelID << " - discarded!" << std::endl;
            } else {
                if (!(msg.header.channelType)) {
                    handleChannel0Message(msg, &senderAddress);
                } else {
                    // Try to find a matching registered channel to handle the message
                    auto it = channels.find(msg.header.channelID);
                    if (it != channels.end()) {
                        it->second->handleMessage(msg, &senderAddress);
                    }
                }
            }
        } else {
            break;
        }

        // Call channel process() iterations
        for (auto& kv: channels) {
            kv.second->process();
        }
    }
}

// Actually send data, buffer must be stable
int DataChannelServiceBase::sendDataInternal(unsigned char* compiledMessage, unsigned int messageSize, sockaddr_in* recipient) {
    if (!recipient) throw std::runtime_error("Requested sendDataInternal without recipient address");
    if (messageSize < sizeof(DataChannelMessageHeader)) throw std::runtime_error("Message header too short");
    DataChannelMessageHeader* header = reinterpret_cast<DataChannelMessageHeader*>(compiledMessage);
    unsigned int reportedSize = sizeof(DataChannelMessageHeader) + ntohl(header->payloadSize);
    if (messageSize != reportedSize) throw std::runtime_error("Message size does not match");
    int result = 0;
    result = sendto(dataChannelSocket, (char*) compiledMessage, reportedSize, 0, (sockaddr*) recipient, sizeof(*recipient));
    if (result != (int) reportedSize) {
        std::cerr << "Error sending DataChannel message to " << inet_ntoa(recipient->sin_addr) << ": " << Networking::getLastErrorString() << std::endl;
        throw std::runtime_error("Error during sendto");
    }
    return result;
}

// Generate a new message and send it
int DataChannelServiceBase::sendDataIsolatedPacket(DataChannel::ID id, DataChannel::Type type, unsigned char* data, unsigned int dataSize, sockaddr_in* recipient) {
    unsigned int msgSize = sizeof(DataChannelMessageHeader) + dataSize;
    unsigned char* buf = new unsigned char[msgSize]();
    DataChannelMessageHeader* header = reinterpret_cast<DataChannelMessageHeader*>(buf);
    header->channelID = id;
    header->channelType = type;
    header->payloadSize = htonl(dataSize);
    std::memcpy(buf + sizeof(DataChannelMessageHeader), data, dataSize);

    int result = sendDataInternal(buf, msgSize, recipient);
    delete[] buf;
    return result;
}

DataChannel::ID DataChannelServiceBase::registerChannel(std::shared_ptr<DataChannel> channel) {
    // Preliminary implementation: set id:=type (should allocate dynamic IDs later)
    DataChannel::ID id = (DataChannel::ID) channel->getChannelType();
    if (channels.count(id)) {
        return 0; // already registered this ID
    }
    // Checking dynamic init, if this fails the service is not registered (and will be auto cleaned)
    if (!channel->initialize()) return 0;
    channel->setChannelID(id);
    channels[id] = channel;
    channel->setService(shared_from_this());
    return id;
}
int DataChannel::sendData(unsigned char* data, unsigned int dataLen, sockaddr_in* recipient) {
    if (auto srv = service.lock()) {
        return srv->sendDataIsolatedPacket(channelID, getChannelType(), data, dataLen, recipient);
    } else return 0;
}

}} // namespaces

