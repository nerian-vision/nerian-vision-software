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

#include <cstdio>
#include <iostream>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include "visiontransfer/imagetransfer.h"
#include "visiontransfer/exceptions.h"
#include "visiontransfer/internal/datablockprotocol.h"
#include "visiontransfer/internal/networking.h"

using namespace std;
using namespace visiontransfer;
using namespace visiontransfer::internal;

namespace visiontransfer {

/*************** Pimpl class containing all private members ***********/

class ImageTransfer::Pimpl {
public:
    Pimpl(const char* address, const char* service, ImageProtocol::ProtocolType protType,
        bool server, int bufferSize, int maxUdpPacketSize, int autoReconnectDelay,
        const std::vector<ExternalBufferSet>& externalBufferSets);
    ~Pimpl();

    // Redeclaration of public members
    void setRawTransferData(const ImageSet& metaData, const std::vector<unsigned char*>& rawData,
        int firstTileWidth = 0, int middleTileWidth = 0, int lastTileWidth = 0);
    void setRawValidBytes(const std::vector<int>& validBytes);
    void setTransferImageSet(const ImageSet& imageSet);
    TransferStatus transferData();
    bool receiveImageSet(ImageSet& imageSet);
    bool receivePartialImageSet(ImageSet& imageSet, int& validRows, bool& complete);
    int getNumDroppedFrames() const;
    bool isConnected() const;
    void disconnect();
    std::string getRemoteAddress() const;
    bool tryAccept();
    void setConnectionStateChangeCallback(std::function<void(visiontransfer::ConnectionState)> callback);
    void establishConnection();
    void setAutoReconnect(int secondsBetweenRetries);

    std::string statusReport();

    void assignExternalBuffer();
    void signalImageSetDone(ImageSet& imageSet);

private:
    // Configuration parameters
    ImageProtocol::ProtocolType protType;
    bool isServer;
    int bufferSize;
    int maxUdpPacketSize;

    // Thread synchronization
    std::recursive_mutex receiveMutex;
    std::recursive_mutex sendMutex;

    // Transfer related members
    SOCKET clientSocket;
    SOCKET tcpServerSocket;
    sockaddr_in remoteAddress;
    addrinfo* addressInfo;

    int tcpReconnectSecondsBetweenRetries;
    bool knownConnectedState; // see Pimpl::isConnected() for info
    bool gotAnyData; // to disambiguate 'connection refused'

    // Object for encoding and decoding the network protocol
    std::unique_ptr<ImageProtocol> protocol;

    // Outstanding network message that still has to be transferred
    int currentMsgLen;
    int currentMsgOffset;
    const unsigned char* currentMsg;

    // User callback for connection state changes
    std::function<void(visiontransfer::ConnectionState)> connectionStateChangeCallback;

    // The registered external sets of buffers
    std::map<ImageSet::ExternalBufferHandle, ExternalBufferSet> externalBufferPool;
    ImageSet::ExternalBufferHandle assignedBufferHandle;

    // Socket configuration
    void setSocketOptions();

    // Network socket initialization
    void initTcpServer();
    void initTcpClient();
    void initUdp();

    // Data reception
    bool receiveNetworkData(bool block);

    // Data transmission
    bool sendNetworkMessage(const unsigned char* msg, int length, sockaddr_in* destAddrUdp=nullptr);
    void sendPendingControlMessages();

    bool selectSocket(bool read, bool wait);
    bool isTcpClientClosed(SOCKET sock);
};

class ImageTransfer::Config::Pimpl {
    public:
        Pimpl(const char* address);
        Pimpl(DeviceInfo& deviceInfo);
        //
        inline void setAddress(const char* address_) { address = address_; address_c = address.c_str(); }
        inline void setService(const char* service_) { service = service_; service_c = service.c_str(); }
        inline void setProtocolType(ImageProtocol::ProtocolType protType) { protocolType = protType; }
        inline void setServer(bool server) { isServer = server; }
        inline void setBufferSize(int bufferSize_) { bufferSize = bufferSize_; }
        inline void setMaxUdpPacketSize(int maxUdpPacketSize_) { maxUdpPacketSize = maxUdpPacketSize_; }
        inline void setAutoReconnectDelay(int autoReconnectDelay_) { autoReconnectDelay = autoReconnectDelay_; }
        inline void addExternalBufferSet(ExternalBufferSet bufset) { externalBufferSets.push_back(bufset); }
        //
        const char* getAddress() const { return address_c; }
        const char* getService() const { return service_c; }
        ImageProtocol::ProtocolType getProtocolType() const { return protocolType; }
        bool getServer() const { return isServer; }
        int getBufferSize() const { return bufferSize; }
        int getMaxUdpPacketSize() const { return maxUdpPacketSize; }
        int getAutoReconnectDelay() const { return autoReconnectDelay; }
        std::vector<ExternalBufferSet> getExternalBufferSets() const { return externalBufferSets; }
    private:
        std::string address;
        std::string service;
        const char* address_c;
        const char* service_c;
        ImageProtocol::ProtocolType protocolType;
        bool isServer;
        int bufferSize;
        int maxUdpPacketSize;
        int autoReconnectDelay;
        std::vector<ExternalBufferSet> externalBufferSets;
};

/******************** Stubs for all public members ********************/

ImageTransfer::ImageTransfer(const char* address, const char* service,
        ImageProtocol::ProtocolType protType, bool server, int bufferSize, int maxUdpPacketSize,
        int autoReconnectDelay):
        pimpl(new Pimpl(address, service, protType, server, bufferSize, maxUdpPacketSize,
                autoReconnectDelay, std::vector<ExternalBufferSet>{})) {
    // All initialization in the pimpl class
}

ImageTransfer::ImageTransfer(const DeviceInfo& device, int bufferSize, int maxUdpPacketSize,
        int autoReconnectDelay):
        pimpl(new Pimpl(device.getIpAddress().c_str(), "7681", static_cast<ImageProtocol::ProtocolType>(device.getNetworkProtocol()),
        false, bufferSize, maxUdpPacketSize, autoReconnectDelay, std::vector<ExternalBufferSet>{})) {
    // All initialization in the pimpl class
}

ImageTransfer::ImageTransfer(const ImageTransfer::Config& conf) {
    std::vector<ExternalBufferSet> bufferSets;
    for (int i=0; i<conf.getNumExternalBufferSets(); ++i) {
        bufferSets.push_back(conf.getExternalBufferSet(i));
    }
    // All initialization in the pimpl class
    pimpl = new Pimpl(conf.getAddress(), conf.getService(), conf.getProtocolType(), conf.getServer(),
            conf.getBufferSize(), conf.getMaxUdpPacketSize(), conf.getAutoReconnectDelay(),
            bufferSets);
}

ImageTransfer::~ImageTransfer() {
    delete pimpl;
}

void ImageTransfer::setRawTransferData(const ImageSet& metaData, const std::vector<unsigned char*>& rawData,
        int firstTileWidth, int middleTileWidth, int lastTileWidth) {
    pimpl->setRawTransferData(metaData, rawData, firstTileWidth, middleTileWidth, lastTileWidth);
}

void ImageTransfer::setRawValidBytes(const std::vector<int>& validBytes) {
    pimpl->setRawValidBytes(validBytes);
}

void ImageTransfer::setTransferImageSet(const ImageSet& imageSet) {
    pimpl->setTransferImageSet(imageSet);
}

ImageTransfer::TransferStatus ImageTransfer::transferData() {
    return pimpl->transferData();
}

bool ImageTransfer::receiveImageSet(ImageSet& imageSet) {
    return pimpl->receiveImageSet(imageSet);
}

bool ImageTransfer::receivePartialImageSet(ImageSet& imageSet, int& validRows, bool& complete) {
    return pimpl->receivePartialImageSet(imageSet, validRows, complete);
}

int ImageTransfer::getNumDroppedFrames() const {
    return pimpl->getNumDroppedFrames();
}

bool ImageTransfer::isConnected() const {
    return pimpl->isConnected();
}

void ImageTransfer::disconnect() {
    // User-requested disconnect: disable reconnection first
    pimpl->setAutoReconnect(0);
    pimpl->disconnect();
}

std::string ImageTransfer::getRemoteAddress() const {
    return pimpl->getRemoteAddress();
}

bool ImageTransfer::tryAccept() {
    return pimpl->tryAccept();
}

void ImageTransfer::setConnectionStateChangeCallback(std::function<void(visiontransfer::ConnectionState)> callback) {
    pimpl->setConnectionStateChangeCallback(callback);
}

void ImageTransfer::setAutoReconnect(int secondsBetweenRetries) {
    pimpl->setAutoReconnect(secondsBetweenRetries);
}

// ImageTransfer::Config

ImageTransfer::Config::Config(const char* address)
: pimpl( new ImageTransfer::Config::Pimpl(address)) {
}

ImageTransfer::Config::Config(DeviceInfo& deviceInfo)
: pimpl( new ImageTransfer::Config::Pimpl(deviceInfo)) {
}
ImageTransfer::Config::~Config() {
    delete pimpl;
}

ImageTransfer::Config& ImageTransfer::Config::setAddress(const char* address) {
    pimpl->setAddress(address);
    return *this;
}
ImageTransfer::Config& ImageTransfer::Config::setService(const char* service) {
    pimpl->setService(service);
    return *this;
}
ImageTransfer::Config& ImageTransfer::Config::setProtocolType(ImageProtocol::ProtocolType protType) {
    pimpl->setProtocolType(protType);
    return *this;
}
ImageTransfer::Config& ImageTransfer::Config::setServer(bool server) {
    pimpl->setServer(server);
    return *this;
}
ImageTransfer::Config& ImageTransfer::Config::setBufferSize(int bufferSize) {
    pimpl->setBufferSize(bufferSize);
    return *this;
}
ImageTransfer::Config& ImageTransfer::Config::setMaxUdpPacketSize(int maxUdpPacketSize) {
    pimpl->setMaxUdpPacketSize(maxUdpPacketSize);
    return *this;
}
ImageTransfer::Config& ImageTransfer::Config::setAutoReconnectDelay(int autoReconnectDelay) {
    pimpl->setAutoReconnectDelay(autoReconnectDelay);
    return *this;
}
ImageTransfer::Config& ImageTransfer::Config::addExternalBufferSet(ExternalBufferSet bufset) {
    pimpl->addExternalBufferSet(bufset);
    return *this;
}

const char* ImageTransfer::Config::getAddress() const {
    return pimpl->getAddress();
}
const char* ImageTransfer::Config::getService() const {
    return pimpl->getService();
}
ImageProtocol::ProtocolType ImageTransfer::Config::getProtocolType() const {
    return pimpl->getProtocolType();
}
bool ImageTransfer::Config::getServer() const {
    return pimpl->getServer();
}
int ImageTransfer::Config::getBufferSize() const {
    return pimpl->getBufferSize();
}
int ImageTransfer::Config::getMaxUdpPacketSize() const {
    return pimpl->getMaxUdpPacketSize();
}
int ImageTransfer::Config::getAutoReconnectDelay() const {
    return pimpl->getAutoReconnectDelay();
}
int ImageTransfer::Config::getNumExternalBufferSets() const {
    return (int) pimpl->getExternalBufferSets().size();
}
ExternalBufferSet ImageTransfer::Config::getExternalBufferSet(int idx) const {
    return pimpl->getExternalBufferSets().at(idx);
}

void ImageTransfer::signalImageSetDone(ImageSet& imageSet) {
    pimpl->signalImageSetDone(imageSet);
}

/******************** Implementation in pimpl classes *******************/

// ImageTransfer

ImageTransfer::Pimpl::Pimpl(const char* address, const char* service,
        ImageProtocol::ProtocolType protType, bool server, int
        bufferSize, int maxUdpPacketSize, int autoReconnectDelay,
        const std::vector<ExternalBufferSet>& externalBufferSets)
        : protType(protType), isServer(server), bufferSize(bufferSize),
        maxUdpPacketSize(maxUdpPacketSize),
        clientSocket(INVALID_SOCKET), tcpServerSocket(INVALID_SOCKET),
        tcpReconnectSecondsBetweenRetries(autoReconnectDelay),
        knownConnectedState(false), gotAnyData(false),
        currentMsgLen(0), currentMsgOffset(0), currentMsg(nullptr) {

    // Initialize the buffer store (and handle lookup table)
    for (auto& bufset: externalBufferSets) {
        std::cout << "DEBUG: Adding an ExternalBufferSet, handle " << bufset.getHandle() << ", consisting of:" << std::endl;
        for (int i=0; i<bufset.getNumBuffers(); ++i) {
            auto const& buf = bufset.getBuffer(i);
            std::cout << "DEBUG:     ExternalBuffer of size " << buf.getBufferSize() << " at address " << ((off_t) buf.getBufferPtr()) << " with target layout mapping: " << std::endl;
            for (int j=0; j<buf.getNumParts(); ++j) {
                auto const& part = buf.getPart(j);
                std::cout << "DEBUG:         ImageType " << part.imageType << " with conversion flags " << part.conversionFlags << " reserveBits " << part.reserveBits << std::endl;
            }
        }
        externalBufferPool[bufset.getHandle()] = bufset;
    }

    Networking::initNetworking();
#ifndef _WIN32
    // We don't want to be interrupted by the pipe signal
    signal(SIGPIPE, SIG_IGN);
#endif

    memset(&remoteAddress, 0, sizeof(remoteAddress));

    // If address is null we use the any address
    if(address == nullptr || string(address) == "") {
        address = "0.0.0.0";
    }

    addressInfo = Networking::resolveAddress(address, service);
    establishConnection();
}

void ImageTransfer::Pimpl::assignExternalBuffer() {
    for (auto& [handle, bufset]: externalBufferPool) {
        if (!bufset.getReady()) { // eligible for next buffer fill
            std::cout << "ImageProtocol gets buffer set #" << handle << std::endl;
            assignedBufferHandle = handle;
            protocol->setExternalBufferSet(bufset);
            return;
        }
    }
    throw TransferException("External buffer pool exhausted!");
}

void ImageTransfer::Pimpl::establishConnection() {

    try {
        if(protType == ImageProtocol::PROTOCOL_UDP) {
            initUdp();
        } else if(protType == ImageProtocol::PROTOCOL_TCP && isServer) {
            initTcpServer();
        } else {
            initTcpClient();
        }
    } catch(...) {
        throw;
    }

    knownConnectedState = true;
    if (connectionStateChangeCallback) {
        std::thread([&](){connectionStateChangeCallback(visiontransfer::ConnectionState::CONNECTED);}).detach();
    }
}

ImageTransfer::Pimpl::~Pimpl() {

    setAutoReconnect(0);
    if (isConnected()) disconnect();

    if(clientSocket != INVALID_SOCKET) {
        Networking::closeSocket(clientSocket);
    }
    if(tcpServerSocket != INVALID_SOCKET) {
        Networking::closeSocket(tcpServerSocket);
    }
    if(addressInfo != nullptr) {
        freeaddrinfo(addressInfo);
    }

}

void ImageTransfer::Pimpl::initTcpClient() {
    protocol.reset(new ImageProtocol(isServer, ImageProtocol::PROTOCOL_TCP));
    if (externalBufferPool.size() > 0) {
        assignExternalBuffer();
    }

    clientSocket = Networking::connectTcpSocket(addressInfo);
    memcpy(&remoteAddress, addressInfo->ai_addr, sizeof(remoteAddress));

    // Set special socket options
    setSocketOptions();
}

void ImageTransfer::Pimpl::initTcpServer() {
    protocol.reset(new ImageProtocol(isServer, ImageProtocol::PROTOCOL_TCP));
    if (externalBufferPool.size() > 0) {
        assignExternalBuffer();
    }

    // Create socket
    tcpServerSocket = ::socket(addressInfo->ai_family, addressInfo->ai_socktype,
        addressInfo->ai_protocol);
    if (tcpServerSocket == INVALID_SOCKET)  {
        TransferException ex("Error opening socket: " + Networking::getLastErrorString());
        throw ex;
    }

    // Enable reuse address
    Networking::enableReuseAddress(tcpServerSocket, true);

    // Open a server port
    Networking::bindSocket(tcpServerSocket, addressInfo);
    clientSocket = INVALID_SOCKET;

    // Make the server socket non-blocking
    Networking::setSocketBlocking(tcpServerSocket, false);

    // Listen on port
    listen(tcpServerSocket, 1);
}

void ImageTransfer::Pimpl::initUdp() {
    protocol.reset(new ImageProtocol(isServer, ImageProtocol::PROTOCOL_UDP, maxUdpPacketSize));
    if (externalBufferPool.size() > 0) {
        assignExternalBuffer();
    }

    // Create sockets
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(clientSocket == INVALID_SOCKET) {
        TransferException ex("Error creating receive socket: " + Networking::getLastErrorString());
        throw ex;
    }

    // Enable reuse address
    Networking::enableReuseAddress(clientSocket, true);

    // Bind socket to port
    if(isServer && addressInfo != nullptr) {
        Networking::bindSocket(clientSocket, addressInfo);
    }

    if(!isServer) {
        memcpy(&remoteAddress, addressInfo->ai_addr, sizeof(remoteAddress));
    }

    // Set special socket options
    setSocketOptions();
}

bool ImageTransfer::Pimpl::tryAccept() {
    if(protType != ImageProtocol::PROTOCOL_TCP || ! isServer) {
        throw TransferException("Connections can only be accepted in tcp server mode");
    }

    // Accept one connection
    sockaddr_in newRemoteAddress;
    memset(&newRemoteAddress, 0, sizeof(newRemoteAddress));
    SOCKET newSocket = Networking::acceptConnection(tcpServerSocket, newRemoteAddress);
    if(newSocket == INVALID_SOCKET) {
        // No connection
        return false;
    }

    // For a new connection we require locks
    unique_lock<recursive_mutex> recvLock(receiveMutex);
    unique_lock<recursive_mutex> sendLock(sendMutex);

    if(clientSocket != INVALID_SOCKET) {
        // More robust TCP behavior: reject new connection.
        // (We had to accept first so we can close now.)
        // Remote client will detect that we closed immediately without sending data.
        //std::cerr << "DEBUG- Rejecting new TCP connection, we are busy already" << std::endl;
        Networking::closeSocket(newSocket);
        return false;
    }
    memcpy(&remoteAddress, &newRemoteAddress, sizeof(remoteAddress));
    clientSocket = newSocket;

    // Set special socket options
    setSocketOptions();

    // Reset connection data
    protocol->resetTransfer();
    protocol->resetReception();
    currentMsg = nullptr;

    knownConnectedState = true;

    return true;
}

std::string ImageTransfer::Pimpl::getRemoteAddress() const {
    unique_lock<recursive_mutex> lock(const_cast<recursive_mutex&>(sendMutex)); // either mutex will work

    if(remoteAddress.sin_family != AF_INET) {
        return "";
    }

    char strPort[11];
    snprintf(strPort, sizeof(strPort), ":%d", remoteAddress.sin_port);

    return string(inet_ntoa(remoteAddress.sin_addr)) + strPort;
}

void ImageTransfer::Pimpl::setSocketOptions() {
    // Set the socket buffer sizes
    if(bufferSize > 0) {
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&bufferSize), sizeof(bufferSize));
        setsockopt(clientSocket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&bufferSize), sizeof(bufferSize));
    }

    Networking::setSocketTimeout(clientSocket, 500);
    Networking::setSocketBlocking(clientSocket, true);
}

void ImageTransfer::Pimpl::setRawTransferData(const ImageSet& metaData,
        const std::vector<unsigned char*>& rawDataVec, int firstTileWidth, int middleTileWidth, int lastTileWidth) {
    unique_lock<recursive_mutex> sendLock(sendMutex);
    protocol->setRawTransferData(metaData, rawDataVec, firstTileWidth, middleTileWidth, lastTileWidth);
    currentMsg = nullptr;
}

void ImageTransfer::Pimpl::setRawValidBytes(const std::vector<int>& validBytes) {
    unique_lock<recursive_mutex> sendLock(sendMutex);
    protocol->setRawValidBytes(validBytes);
}

void ImageTransfer::Pimpl::setTransferImageSet(const ImageSet& imageSet) {
    unique_lock<recursive_mutex> sendLock(sendMutex);
    protocol->setTransferImageSet(imageSet);
    currentMsg = nullptr;
}

ImageTransfer::TransferStatus ImageTransfer::Pimpl::transferData() {
    unique_lock<recursive_mutex> lock(sendMutex);

    // First receive data in case a control message arrives
    if(protType == ImageProtocol::PROTOCOL_UDP) {
        // This also handles the UDP 'disconnection' tracking
        receiveNetworkData(false);
    } else if (isServer) {
        if (isConnected()) {
            // Test if TCP pipe closed remotely (even when we have nothing to send)
            bool disconnected = isTcpClientClosed(clientSocket);
            if (disconnected) {
                // The connection has been closed
                disconnect();
            }
        }
    }

    if(!isConnected()) {
        // Cannot send while (temporarily) disconnected
        // Note: TCP server mode is currently not auto-reconnecting
        return NOT_CONNECTED;
    }

#ifndef _WIN32
    // Cork TCP to prevent sending of small packets
    if(protType == ImageProtocol::PROTOCOL_TCP) {
        int flag = 1;
        setsockopt(clientSocket, IPPROTO_TCP, TCP_CORK, (char *) &flag, sizeof(int));
    }
#endif

    // Get first message to transfer
    if(currentMsg == nullptr) {
        currentMsgOffset = 0;
        currentMsg = protocol->getTransferMessage(currentMsgLen);

        if(currentMsg == nullptr) {
            if(protocol->transferComplete()) {
                return ALL_TRANSFERRED;
            } else {
                return NO_VALID_DATA;
            }
        }
    }

    // Try transferring messages
    bool wouldBlock = false;
    bool dataTransferred = (currentMsg != nullptr);
    while(currentMsg != nullptr) {
        int writing = (int)(currentMsgLen - currentMsgOffset);

        if(sendNetworkMessage(&currentMsg[currentMsgOffset], writing)) {
            // Get next message
            currentMsgOffset = 0;
            currentMsg = protocol->getTransferMessage(currentMsgLen);
        } else {
            // The operation would block
            wouldBlock = true;
            break;
        }
    }

    if(dataTransferred && protType == ImageProtocol::PROTOCOL_TCP && protocol->transferComplete()) {
#ifndef _WIN32
        // Uncork - sends the assembled messages
        int flag = 0;
        setsockopt(clientSocket, IPPROTO_TCP, TCP_CORK, (char *) &flag, sizeof(int));
#else
        // Force a flush for TCP by turning the nagle algorithm off and on
        int flag = 1;
        setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
        flag = 0;
        setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
#endif
    }

    // Also check for control messages at the end
    if(protType == ImageProtocol::PROTOCOL_UDP) {
        receiveNetworkData(false);
    }

    if(protocol->transferComplete()) {
        return ALL_TRANSFERRED;
    } else if(wouldBlock) {
        return WOULD_BLOCK;
    } else {
        return PARTIAL_TRANSFER;
    }
}

bool ImageTransfer::Pimpl::receiveImageSet(ImageSet& imageSet) {
    int validRows = 0;
    bool complete = false;

    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    while(!complete) {
        if(!receivePartialImageSet(imageSet, validRows, complete)) {
            return false;
        }

        unsigned int time = static_cast<unsigned int>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count());
        if(time > 100 && !complete) {
            return false;
        }
    }

    return true;
}

bool ImageTransfer::Pimpl::receivePartialImageSet(ImageSet& imageSet,
        int& validRows, bool& complete) {
    unique_lock<recursive_mutex> lock(receiveMutex);

    // Try to receive further image data if needed
    bool block = true;
    while(!protocol->imagesReceived() && receiveNetworkData(block)) {
        block = false;
    }

    // Get received image
    bool result = protocol->getPartiallyReceivedImageSet(imageSet, validRows, complete);

    // If the image set was completed now (and the transfer has hence reset),
    // make sure that the next external available buffer set is rotated in (if enabled)
    if (complete) {
        if (externalBufferPool.size() > 0) {
            imageSet.setExternalBufferHandle(assignedBufferHandle);
            externalBufferPool[assignedBufferHandle].setReady(true);
            assignExternalBuffer();
            // May have returned empty bufset if all buffer sets have not returned from external control!
            // The protocol will then discard any incoming data until a new buffer set is provided.
        }
    }

    return result;
}

bool ImageTransfer::Pimpl::receiveNetworkData(bool block) {
    unique_lock<recursive_mutex> lock = block ?
        unique_lock<recursive_mutex>(receiveMutex) : unique_lock<recursive_mutex>(receiveMutex, std::try_to_lock);

    if(clientSocket == INVALID_SOCKET) {
        return false; // Not connected
    }

    if (protType == ImageProtocol::PROTOCOL_UDP) {
        // UDP-only: Track and signal connection state by checking protocol
        // (TCP uses socket-level disconnect/reconnect events instead)
        bool newConnectedState = protocol->isConnected();
        if (newConnectedState != knownConnectedState) {
            knownConnectedState = newConnectedState;
            if (connectionStateChangeCallback) {
                std::thread([&, newConnectedState](){ connectionStateChangeCallback(newConnectedState ? visiontransfer::ConnectionState::CONNECTED : visiontransfer::ConnectionState::DISCONNECTED); }).detach();
            }
            if (!newConnectedState) {
                // Newly disconnected, abort
                return false;
            }
        }
    }

    // First send control messages if necessary
    sendPendingControlMessages();

    if(!lock.owns_lock()) {
        // Waiting for the lock would block this call
        return false;
    }

    // Test if the socket has data available
    if(!block && !selectSocket(true, false)) {
        return false;
    }

    int maxLength = 0;
    char* buffer = reinterpret_cast<char*>(protocol->getNextReceiveBuffer(maxLength));

    // Receive data
    sockaddr_in fromAddress;
    socklen_t fromSize = sizeof(fromAddress);

    int bytesReceived = recvfrom(clientSocket, buffer, maxLength,
        0, reinterpret_cast<sockaddr*>(&fromAddress), &fromSize);

    auto err = Networking::getErrno();
    if(bytesReceived == 0 || (protType == ImageProtocol::PROTOCOL_TCP && bytesReceived < 0 && err == WSAECONNRESET)) {
        // Connection closed
        disconnect();
        if ((!isServer) && (!gotAnyData)) {
            // TCP client connection was refused by the device because it had another connected client
            setAutoReconnect(0);
            throw ConnectionClosedException("Device is already connected to another client");
        }
    } else if(bytesReceived < 0 && err != EWOULDBLOCK && err != EINTR &&
            err != ETIMEDOUT && err != WSA_IO_PENDING && err != WSAECONNRESET) {
        TransferException ex("Error reading from socket: " + Networking::getErrorString(err));
        throw ex;
    } else if(bytesReceived > 0) {
        // Check whether this reception is from an unexpected new sender (for UDP server)
        bool newSender = (
                protType == ImageProtocol::PROTOCOL_UDP &&
                ((fromAddress.sin_addr.s_addr!=remoteAddress.sin_addr.s_addr) || (fromAddress.sin_port!=remoteAddress.sin_port)) &&
                (remoteAddress.sin_port != 0)
            );
        if (isServer && newSender) {
            //std::cout << "New connection" << std::endl;
            if (protocol->isConnected()) {
                // Reject interfering client
                // Note: this has no bearing on the receive buffer obtained above; we will overwrite in place
                //std::cerr << "DEBUG- Rejecting interfering UDP client" << std::endl;
                const unsigned char* disconnectionMsg;
                int disconnectionMsgLen;
                DataBlockProtocol::getDisconnectionMessage(disconnectionMsg, disconnectionMsgLen);
                if (disconnectionMsgLen > 0) {
                    sendNetworkMessage(disconnectionMsg, disconnectionMsgLen, &fromAddress);
                }
            }
        } else {
            gotAnyData = true;
            protocol->processReceivedMessage(bytesReceived);
            if(protocol->newClientConnected()) {
                // We have just established a new connection
                memcpy(&remoteAddress, &fromAddress, sizeof(remoteAddress));

                if (isServer && (protType == ImageProtocol::PROTOCOL_UDP)) {
                    // Welcome client with the knock sequence. Older clients just ignore this,
                    // new clients know they can expect, and may also use, the extended protocol.
                    const unsigned char* heartbeatMsg;
                    int heartbeatMsgLen;
                    DataBlockProtocol::getHeartbeatMessage(heartbeatMsg, heartbeatMsgLen);
                    if (heartbeatMsgLen > 0) {
                        //std::cout << "Sending five knocks" << std::endl;
                        for (int i=0; i<5; ++i) {
                            // Send 5 UDP knocks for good measure, the client looks for at least 3 within 0.5 s
                            sendNetworkMessage(heartbeatMsg, heartbeatMsgLen, &fromAddress);
                        }
                    }
                }
            }
        }
        if (isServer && protType == ImageProtocol::PROTOCOL_UDP) {
            if (!protocol->isConnected() && (remoteAddress.sin_port != 0)) {
                //std::cout << "Invalidating remote address" << std::endl;
                // Existing UDP client has disconnected, invalidate the remote address
                memset(&remoteAddress, 0, sizeof(remoteAddress));
            }
        }
    }

    return bytesReceived > 0;
}

void ImageTransfer::Pimpl::disconnect() {
    // disconnect
    unique_lock<recursive_mutex> recvLock(receiveMutex);
    unique_lock<recursive_mutex> sendLock(sendMutex);

    if(clientSocket != INVALID_SOCKET) {
        if ((!isServer) && isConnected() && protType == ImageProtocol::PROTOCOL_UDP) {
            if (protocol->supportsExtendedConnectionStateProtocol()) {
                // Send a final client-side disconnection request instead of
                // needing to wait for UDP heartbeat timeout on the device
                try {
                    const unsigned char* disconnectionMsg;
                    int disconnectionMsgLen;
                    DataBlockProtocol::getDisconnectionMessage(disconnectionMsg, disconnectionMsgLen);
                    if (disconnectionMsgLen > 0) {
                        sendNetworkMessage(disconnectionMsg, disconnectionMsgLen, &remoteAddress);
                    }
                } catch(...) {
                    // Server will see a disconnection (through heartbeat timeout) anyway
                }
            }
        }
    }

    knownConnectedState = false;
    if (connectionStateChangeCallback) connectionStateChangeCallback(visiontransfer::ConnectionState::DISCONNECTED);

    if(clientSocket != INVALID_SOCKET && protType == ImageProtocol::PROTOCOL_TCP) {
        Networking::closeSocket(clientSocket);
        memset(&remoteAddress, 0, sizeof(remoteAddress));

        // Attempt reconnection, if configured
        if ((!isServer) && (tcpReconnectSecondsBetweenRetries > 0)) {
            for (;;) {
                try {
                    establishConnection();
                    // Successful reconnection (state change has been signaled inside establishConnection)
                    return;
                } catch(...) {
                    // An exception has occurred during reconnection. Since the connection
                    // had suceeded originally during construction, we just keep trying.
                    std::this_thread::sleep_for(std::chrono::seconds(tcpReconnectSecondsBetweenRetries));
                }
            }
        }
    }
    memset(&remoteAddress, 0, sizeof(remoteAddress));
}

bool ImageTransfer::Pimpl::isConnected() const {
    unique_lock<recursive_mutex> lock(const_cast<recursive_mutex&>(sendMutex)); //either mutex will work

    // This tracks the most up-to-date connection state. For TCP, this simply means socket-level
    // disconnects or [re]connects. For UDP, which is connectionless, this means tracking whether
    // the heartbeat replies (and/or payload data) currently arrive (this is established internally
    // on the level of the DataBlockProtocol).
    return knownConnectedState;
}

bool ImageTransfer::Pimpl::sendNetworkMessage(const unsigned char* msg, int length, sockaddr_in* destAddrUdp) {
    int written = 0;
    if(protType == ImageProtocol::PROTOCOL_UDP) {
        sockaddr_in* destAddr;
        SOCKET destSocket;
        {
            unique_lock<recursive_mutex> lock(sendMutex);
            if (destAddrUdp) {
                // An overridden UDP destination (i.e. an interfering client)
                destAddr = destAddrUdp;
            } else {
                // The correctly connected client
                destAddr = &remoteAddress;
            }
            destSocket = clientSocket;
        }

        if(destAddr->sin_family != AF_INET) {
            return false; // Not connected
        }

        written = sendto(destSocket, reinterpret_cast<const char*>(msg), length, 0,
            reinterpret_cast<sockaddr*>(destAddr), sizeof(*destAddr));
    } else {
        SOCKET destSocket;
        {
            unique_lock<recursive_mutex> lock(sendMutex);
            destSocket = clientSocket;
        }
        written = send(destSocket, reinterpret_cast<const char*>(msg), length, 0);
    }

    auto sendError = Networking::getErrno();

    if(written < 0) {
        if(sendError == EAGAIN || sendError == EWOULDBLOCK || sendError == ETIMEDOUT) {
            // The socket is not yet ready for a new transfer
            return false;
        } else if(sendError == EPIPE) {
            // The connection has been closed
            disconnect();
            return false;
        } else {
            TransferException ex("Error sending network packet: " + Networking::getErrorString(sendError));
            throw ex;
        }
    } else if(written != length) {
        if(protType == ImageProtocol::PROTOCOL_UDP) {
            // The message has been transmitted partially
            throw TransferException("Unable to transmit complete UDP message");
        } else {
            // For TCP we can transmit the remaining data later
            currentMsgOffset += written;
            return false;
        }
    } else {
        return true;
    }
}

void ImageTransfer::Pimpl::sendPendingControlMessages() {
    const unsigned char* controlMsgData = nullptr;
    int controlMsgLen = 0;

    while(true) {
        unique_lock<recursive_mutex> lock(sendMutex);
        if(remoteAddress.sin_family != AF_INET) {
            return;
        }

        controlMsgData = protocol->getNextControlMessage(controlMsgLen);

        if(controlMsgData != nullptr) {
            sendNetworkMessage(controlMsgData, controlMsgLen);
        } else {
            break;
        }
    }
}

int ImageTransfer::Pimpl::getNumDroppedFrames() const {
    return protocol->getNumDroppedFrames();
}

bool ImageTransfer::Pimpl::isTcpClientClosed(SOCKET sock) {
    char x;
    auto ret = recv(sock, &x, 1, MSG_DONTWAIT | MSG_PEEK);
    return ret == 0;
}

bool ImageTransfer::Pimpl::selectSocket(bool read, bool wait) {
    (void) read; // used in Windows
    SOCKET sock;
    {
        unique_lock<recursive_mutex> lock(sendMutex); // Either mutex will do
        sock = clientSocket;
    }
#ifdef _WIN32
    fd_set fds;
    struct timeval tv;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    tv.tv_sec = 0;
    if(wait) {
        tv.tv_usec = 100000;
    } else {
        tv.tv_usec = 0;
    }

    if(select(((int)sock)+1, (read ? &fds : nullptr), (!read ? &fds : nullptr), nullptr, &tv) <= 0) {
        // The socket is currently not ready
        return false;
    }
#else
    // use poll() on non-Windows platform (glibc select() limitations)
    constexpr int timeoutMillisec = 100;
    pollfd pfd;
    pfd.fd = sock;
    pfd.events = POLLIN;
    if (poll(&pfd, 1, wait ? timeoutMillisec: 0) <= 0) {
        // The socket is currently not ready
        return false;
    }
#endif
    // select (or poll) reported an event
    return true;
}

std::string ImageTransfer::statusReport() {
    return pimpl->statusReport();
}
std::string ImageTransfer::Pimpl::statusReport() {
    return protocol->statusReport();
}

void ImageTransfer::Pimpl::setConnectionStateChangeCallback(std::function<void(visiontransfer::ConnectionState)> callback) {
    connectionStateChangeCallback = callback;
}

void ImageTransfer::Pimpl::setAutoReconnect(int secondsBetweenRetries) {
    tcpReconnectSecondsBetweenRetries = secondsBetweenRetries;
}

void ImageTransfer::Pimpl::signalImageSetDone(ImageSet& imageSet) {
    auto handle = imageSet.getExternalBufferHandle();
    std::cout << "handleImageSetDone for handle #" << handle << std::endl;
    if (handle == 0) return; // No-op, not an image set with external buffering
    if (!externalBufferPool.count(handle)) {
        throw ProtocolException("Invalid external buffer handle");
    }
    // Allow the buffers to be filled again
    externalBufferPool[handle].setReady(false);
}


// ImageTransfer::Config

ImageTransfer::Config::Pimpl::Pimpl(const char* address_)
: address(address_), service("7681"), protocolType(ImageProtocol::PROTOCOL_UDP),
  isServer(false), bufferSize(16*1048576), maxUdpPacketSize(1472),
  autoReconnectDelay(1) {
      address_c = address.c_str();
      service_c = service.c_str();
}

ImageTransfer::Config::Pimpl::Pimpl(DeviceInfo& device)
: address(device.getIpAddress().c_str()), service("7681"), protocolType(static_cast<ImageProtocol::ProtocolType>(device.getNetworkProtocol())),
  isServer(false), bufferSize(16*1048576), maxUdpPacketSize(1472),
  autoReconnectDelay(1) {
      address_c = address.c_str();
      service_c = service.c_str();
}

} // namespace

