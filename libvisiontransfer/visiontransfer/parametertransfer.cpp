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

#include <iostream>

#include "visiontransfer/parametertransfer.h"
#include "visiontransfer/exceptions.h"
#include "visiontransfer/internalinformation.h"
#include "visiontransfer/standardparameterids.h"
#include "visiontransfer/parametertransferdata.h"
#include "visiontransfer/parameterserialization.h"

#include <cstring>
#include <string>
#include <functional>
#include <atomic>
#include <chrono>

using namespace std;
using namespace visiontransfer;
using namespace visiontransfer::internal;
using namespace visiontransfer::param;

namespace visiontransfer {
namespace internal {

constexpr int ParameterTransfer::SOCKET_TIMEOUT_MS;

ParameterTransfer::ParameterTransfer(const char* address, const char* service)
    : socket(INVALID_SOCKET), address(address), service(service), networkReady(false) {

    tabTokenizer.collapse(false).separators({"\t"});

    Networking::initNetworking();
    attemptConnection();
}

ParameterTransfer::~ParameterTransfer() {
    threadRunning = false;
    if (receiverThread->joinable()) {
        receiverThread->join();
    }

    if(socket != INVALID_SOCKET) {
        Networking::closeSocket(socket);
    }
}

// Will attempt initial connection or reconnection after error
void ParameterTransfer::attemptConnection() {
    std::unique_lock<std::mutex> localLock(socketModificationMutex);

    addrinfo* addressInfo = Networking::resolveAddress(address.c_str(), service.c_str());

    socket = Networking::connectTcpSocket(addressInfo);
    Networking::setSocketTimeout(socket, SOCKET_TIMEOUT_MS);

    if (!receiverThread) {
        receiverThread = std::make_shared<std::thread>(std::bind(&ParameterTransfer::receiverRoutine, this));
    }
    networkError = false;
    pollDelay = 1000;

    // Initial 'GetAll' command
    size_t written = send(socket, "A\n", 2, 0);
    if(written != 2) {
        Networking::closeSocket(socket);
        socket = INVALID_SOCKET;
        networkError = true;
        TransferException ex("Error sending GetAllParameter request: " + Networking::getLastErrorString());
        throw ex;
    }

    freeaddrinfo(addressInfo);
}

void ParameterTransfer::waitNetworkReady() {
    if (!networkReady) {
        // Block for network to become ready
        std::unique_lock<std::mutex> readyLock(readyMutex);
        auto status = readyCond.wait_for(readyLock, std::chrono::milliseconds(2000));
        if (status == std::cv_status::timeout) {
            throw TransferException("Timeout waiting for parameter server ready state");
        }
    }
}

void ParameterTransfer::readParameter(unsigned char messageType, const char* id, unsigned char* dest, int length) {
    waitNetworkReady();
    if (networkError) {
        // collecting deferred error from background thread
        throw TransferException("ParameterTransfer currently not operational: " + networkErrorString);
    }

    for (int i=0; i<length; ++i) { dest[i] = '\0'; } // PLACEHOLDER
}

template<typename T>
void ParameterTransfer::writeParameter(const char* id, const T& value) {
    waitNetworkReady();
    if (networkError) {
        // collecting deferred error from background thread
        throw TransferException("ParameterTransfer currently not operational: " + networkErrorString);
    }
    if (!paramSet.count(id)) {
        throw ParameterException("Invalid parameter: " + std::string(id));
    }
    blockingCallThisThread([this, &id, &value](){
        // Emit a set request with our thread id - the receiver thread will unblock us based on that ID
        std::stringstream ss;
        ss << "S" << "\t" << getThreadId() << "\t" << id << "\t" << value << "\n";
        {
            std::unique_lock<std::mutex> localLock(socketModificationMutex);
            if (socket == INVALID_SOCKET) {
                throw TransferException("Connection has been closed and not reconnected so far");
            }
            size_t written = send(socket, ss.str().c_str(), (int) ss.str().size(), 0);
            if(written != ss.str().size()) {
                throw TransferException("Error sending parameter set request: " + Networking::getLastErrorString());
            }
        }
    });
    auto result = lastSetRequestResult[getThreadId()];
    if (result.first == false) {
        // There was a remote error, append its info to the exception
        throw ParameterException("Remote parameter error: " + result.second);
    } else {
        // Local preliminary value update - the (successful!) async remote update may need additional time.
        // The actual value MIGHT have been revised by the server, but in the vast majority of cases this allows
        // reading back the successfully written parameter. The safest way is via setParameterUpdateCallback.
        paramSet[id].setCurrent<T>(value);
    }
}

// Explicit instantiation for std::string
template<>
void ParameterTransfer::writeParameter(const char* id, const std::string& value) {
    waitNetworkReady();
    if (networkError) {
        // collecting deferred error from background thread
        throw TransferException("ParameterTransfer currently not operational: " + networkErrorString);
    }
    if (!paramSet.count(id)) {
        throw ParameterException("Invalid parameter: " + std::string(id));
    }
    blockingCallThisThread([this, &id, &value](){
        // Emit a set request with our thread id - the receiver thread will unblock us based on that ID
        std::stringstream ss;
        ss << "S" << "\t" << getThreadId() << "\t" << id << "\t" << value << "\n";
        {
            std::unique_lock<std::mutex> localLock(socketModificationMutex);
            if (socket == INVALID_SOCKET) {
                throw TransferException("Connection has been closed and not reconnected so far");
            }
            size_t written = send(socket, ss.str().c_str(), (int) ss.str().size(), 0);
            if(written != ss.str().size()) {
                throw TransferException("Error sending parameter set request: " + Networking::getLastErrorString());
            }
        }
    });
    auto result = lastSetRequestResult[getThreadId()];
    if (result.first == false) {
        // There was a remote error, append its info to the exception
        throw ParameterException("Remote parameter error: " + result.second);
    } else {
        // Local preliminary value update - the (successful!) async remote update may need additional time.
        // The actual value MIGHT have been revised by the server, but in the vast majority of cases this allows
        // reading back the successfully written parameter. The safest way is via setParameterUpdateCallback.
        paramSet[id].setCurrent<std::string>(value);
    }
}

int ParameterTransfer::readIntParameter(const char* id) {
    waitNetworkReady();
    if (!paramSet.count(id)) {
        throw ParameterException("Invalid parameter: " + std::string(id));
    }
    return paramSet[id].getCurrent<int>();
}

double ParameterTransfer::readDoubleParameter(const char* id) {
    waitNetworkReady();
    if (!paramSet.count(id)) {
        throw ParameterException("Invalid parameter: " + std::string(id));
    }
    return paramSet[id].getCurrent<double>();
}

bool ParameterTransfer::readBoolParameter(const char* id) {
    waitNetworkReady();
    if (!paramSet.count(id)) {
        throw ParameterException("Invalid parameter: " + std::string(id));
    }
    return paramSet[id].getCurrent<bool>();
}

void ParameterTransfer::writeIntParameter(const char* id, int value) {
    writeParameter(id, value);
}

void ParameterTransfer::writeDoubleParameter(const char* id, double value) {
    writeParameter(id, value);
}

void ParameterTransfer::writeBoolParameter(const char* id, bool value) {
    writeParameter(id, value);
}

std::map<std::string, ParameterInfo> ParameterTransfer::getAllParameters() {
    waitNetworkReady();
    std::map<std::string, ParameterInfo> compatMap;
    {
        std::unique_lock<std::mutex> globalLock(mapMutex);
        for (auto kv: paramSet) {
            auto& name = kv.first;
            auto& param = kv.second;
            bool writeable = param.getAccessForApi() == param::Parameter::ACCESS_READWRITE;
            switch(param.getType()) {
                case param::ParameterValue::TYPE_INT: {
                        int min = -1, max = -1, increment = -1;
                        if (param.hasRange()) {
                            min = param.getMin<int>();
                            max = param.getMax<int>();
                        }
                        if (param.hasIncrement()) {
                            increment = param.getIncrement<int>();
                        }
                        compatMap[name] = ParameterInfo::fromInt(name, writeable, param.getCurrent<int>(), min, max, increment);
                        break;
                    }
                case param::ParameterValue::TYPE_DOUBLE: {
                        double min = -1, max = -1, increment = -1;
                        if (param.hasRange()) {
                            min = param.getMin<double>();
                            max = param.getMax<double>();
                        }
                        if (param.hasIncrement()) {
                            increment = param.getIncrement<double>();
                        }
                        compatMap[name] = ParameterInfo::fromDouble(name, writeable, param.getCurrent<double>(), min, max, increment);
                        break;
                    }
                case param::ParameterValue::TYPE_BOOL: {
                        compatMap[name] = ParameterInfo::fromBool(name, writeable, param.getCurrent<bool>());
                        break;
                    }
                default:
                    // Omit parameters with other types from legacy compatibility API
                    break;
            }
        }
    }
    return compatMap;
}

void ParameterTransfer::receiverRoutine() {
    auto refTime = std::chrono::steady_clock::now();
    recvBufBytes = 0;
    threadRunning = true;
    [[maybe_unused]] int internalThreadId = getThreadId(); // we just reserve ID 0 for the receiver
    while (threadRunning) {
        if (socket == INVALID_SOCKET) {
            // Error that is recoverable by reconnection (otherwise this thread would have been terminated)
            try {
                attemptConnection();
            } catch(...) {
                std::cerr << "Failed to reconnect to parameter server." << std::endl;
                // Sleep receiver thread and retry reconnection in next iteration
                std::this_thread::sleep_for(std::chrono::milliseconds(SOCKET_RECONNECT_INTERVAL_MS));
            }
        } else {
            // Regular connection state - handle incoming events and replies
            int bytesReceived = recv(socket, recvBuf+recvBufBytes, (RECV_BUF_SIZE - recvBufBytes), 0);
            if (bytesReceived < 0) {
                auto err = Networking::getErrno();
                if(err == EAGAIN || err == EWOULDBLOCK || err == ETIMEDOUT) {
                    //std::cout << err << std::endl;
                    // No event or reply - no problem
                    //std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                } else {
                    std::cerr << "Network error (will periodically attempt reconnection)." << std::endl;
                    std::unique_lock<std::mutex> localLock(socketModificationMutex);
                    Networking::closeSocket(socket);
                    socket = INVALID_SOCKET;
                    refTime = std::chrono::steady_clock::now();
                    networkError = true;
                    networkErrorString = std::string("Error receiving network packet: ") + Networking::getLastErrorString();
                    continue;
                }
            } else if (bytesReceived == 0) {
                std::cerr << "Connection closed by remote side (will periodically attempt reconnection)." << std::endl;
                std::unique_lock<std::mutex> localLock(socketModificationMutex);
                Networking::closeSocket(socket);
                socket = INVALID_SOCKET;
                refTime = std::chrono::steady_clock::now();
                networkError = true;
                networkErrorString = "Connection closed";
                continue;
            } else {
                recvBufBytes += bytesReceived;
            }
            unsigned int start=0;
            for (unsigned int i=0; i<recvBufBytes; ++i) {
                unsigned char c = recvBuf[i];
                if (c=='\n') {
                    std::string currentLine((const char*) recvBuf+start, i-start);
                    auto toks = tabTokenizer.tokenize(currentLine);
                    if (toks.size()>0) {
                        const std::string& cmd = toks[0];
                        if (cmd=="P") {
                            if (toks.size()>1) {
                                // Check of protocol version - old firmwares do not send newline-terminated version, which will just time out waitNetworkReady()
                                if(atol(toks[1].c_str()) != static_cast<unsigned int>(InternalInformation::CURRENT_PARAMETER_PROTOCOL_VERSION)) {
                                    networkError = true;
                                    networkErrorString = std::string("Protocol version mismatch, expected ") + std::to_string(InternalInformation::CURRENT_PARAMETER_PROTOCOL_VERSION) + " but got " + toks[1];
                                    threadRunning = false;
                                    break;
                                }
                            } else {
                                networkError = true;
                                networkErrorString = "Incomplete transfer of protocol version";
                                threadRunning = false;
                                break;
                            }
                        } else if (cmd=="I") {
                            // Add or overwrite local parameter
                            Parameter param = ParameterSerialization::deserializeParameterFullUpdate(toks);
                            auto uid = param.getUid();
                            bool alreadyPresent = paramSet.count(uid);
                            paramSet[uid] = param;
                            if (alreadyPresent && parameterUpdateCallback) {
                                // Only call the user callback for metadata updates, but not for the initial enumeration
                                parameterUpdateCallback(uid);
                            }
                        } else if (cmd=="V") {
                            if (toks.size() < 3) {
                                throw TransferException("Received malformed parameter value update");
                            }
                            if (paramSet.count(toks[1])) {
                                // In-place update
                                ParameterSerialization::deserializeParameterValueChange(toks, paramSet[toks[1]]);
                                if (parameterUpdateCallback) {
                                    parameterUpdateCallback(toks[1]);
                                }
                            } else {
                                std::cerr << "Parameter not received yet - not updating value of: " << toks[1] << std::endl;;
                            }
                        } else if (cmd=="R") {
                            if (toks.size() < 4) {
                                throw TransferException("Received malformed reply for parameter set request");
                            }
                            std::unique_lock<std::mutex> globalLock(mapMutex);
                            int replyThreadId = atol(toks[1].c_str());
                            if (waitConds.count(replyThreadId)) {
                                // Reanimating the waiting thread - it will clean up after itself
                                std::lock_guard<std::mutex> localLock(waitCondMutexes[replyThreadId]);
                                lastSetRequestResult[replyThreadId] = {toks[2] == "1", toks[3]};
                                waitConds[replyThreadId].notify_all();
                            } else {
                                std::cerr << "Ignoring unexpected request result for thread " << replyThreadId << std::endl;
                            }
                        } else if (cmd=="E") {
                            // 'End of Transmission' - at least one full enumeration has arrived - we are ready
                            networkReady = true;
                            // Wake any sleeping threads that were blocked until network became ready
                            std::lock_guard<std::mutex> readyLock(readyMutex);
                            readyCond.notify_all();
                        } else if (cmd=="HB") {
                            // heartbeat - ignore
                        } else {
                            networkError = true;
                            networkErrorString = std::string("Unknown update command received: ") + cmd;
                            threadRunning = false;
                            break;
                        }
                    }
                    start = i+1;
                }
            }
            // Move any incomplete line to front of recv buffer
            if (start>=recvBufBytes) {
                recvBufBytes = 0;
            } else {
                std::memmove(recvBuf, recvBuf+start, recvBufBytes-start);
                recvBufBytes = recvBufBytes-start;
            }
        }
    }
}

int ParameterTransfer::getThreadId() {
    // Always returns an int type (which may not be the case for std::thread::id)
    static std::atomic_int threadCount{0};
    thread_local int threadId = threadCount.fetch_add(1);
    return threadId;
}

void ParameterTransfer::blockingCallThisThread(std::function<void()> fn, int waitMaxMilliseconds) {
    auto tid = getThreadId();
    std::unique_lock<std::mutex> globalLock(mapMutex);
    // Populate maps
    auto& localWaitCond = waitConds[tid];
    auto& localWaitCondMutex = waitCondMutexes[tid];
    std::unique_lock<std::mutex> localLock(localWaitCondMutex);
    // First do the actual handshake setup, like emitting the network message
    // (The current thread is protected against a reply race at this point)
    fn();
    // Allow receiver thread to access its checks (it is still blocked by our specific localLock)
    globalLock.unlock();
    // Wait for receiver thread to notify us with the reply
    auto status = localWaitCond.wait_for(localLock, std::chrono::milliseconds(waitMaxMilliseconds));
    // Cleanup, so that any spurious network replies can get detected and discarded
    globalLock.lock();
    waitConds.erase(tid);
    waitCondMutexes.erase(tid);
    globalLock.unlock();
    // Outcome
    if (status == std::cv_status::timeout) {
        TransferException ex("Timeout waiting for request reply from parameter server");
        throw ex;
    }
}

ParameterSet& ParameterTransfer::getParameterSet() {
    waitNetworkReady();
    return paramSet;
}

void ParameterTransfer::setParameterUpdateCallback(std::function<void(const std::string& uid)> callback) {
    parameterUpdateCallback = callback;
}

}} // namespace

