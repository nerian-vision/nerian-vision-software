/*******************************************************************************
 * Copyright (c) 2023 Allied Vision Technologies GmbH
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

thread_local bool ParameterTransfer::transactionInProgress = false;
thread_local std::vector<std::pair<std::string, std::string> > ParameterTransfer::transactionQueuedWrites = {};
thread_local bool ParameterTransfer::writingProhibited = false;

ParameterTransfer::ParameterTransfer(const char* address, const char* service)
    : socket(INVALID_SOCKET), address(address), service(service), networkReady(false), featureDisabledTransactions(false) {

    tabTokenizer.collapse(false).separators({"\t"});
    spaceTokenizer.collapse(true).separators({" "});
    slashTokenizer.collapse(false).separators({"/"});

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

bool ParameterTransfer::isConnected() const {
    // This is technically 'more' than just being connected raw;
    // when true, this connection has passed the full handshake and is operational.
    return networkReady;
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
        networkReady = false;
        networkError = true;
        TransferException ex("Error sending GetAllParameter request: " + Networking::getLastErrorString());
        throw ex;
    }

    freeaddrinfo(addressInfo);
}

void ParameterTransfer::waitNetworkReady() const {
    if (!networkReady) {
        // Block for network to become ready
        std::unique_lock<std::mutex> readyLock(readyMutex);
        auto status = readyCond.wait_for(readyLock, std::chrono::milliseconds(3000));
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

void ParameterTransfer::sendNetworkCommand(const std::string& cmdline, const std::string& diagStr) {
    std::unique_lock<std::mutex> localLock(socketModificationMutex);
    if (socket == INVALID_SOCKET) {
        throw TransferException("Connection has been closed and not reconnected so far");
    }
    size_t written = send(socket, cmdline.c_str(), (int) cmdline.size(), 0);
    if(written != cmdline.size()) {
        throw TransferException("Error sending "+diagStr+" request: " + Networking::getLastErrorString());
    }
}

template<typename T>
void ParameterTransfer::writeParameter(const char* id, const T& value, bool synchronous) {
    waitNetworkReady();
    if (networkError) {
        // collecting deferred error from background thread
        throw TransferException("ParameterTransfer currently not operational: " + networkErrorString);
    }
    if (!paramSet.count(id)) {
        throw ParameterException("Invalid parameter: " + std::string(id));
    }

    // Assemble a set request with our thread id - the receiver thread can unblock us based on that ID
    //  For 'fire-and-forget' commands without reply, we use -1
    std::stringstream ss;
    ss << "S" << "\t" << (synchronous ? getThreadId() : -1) << "\t" << id << "\t" << value << "\n";

    if (synchronous) {
        blockingCallThisThread([this, &ss](){
            sendNetworkCommand(ss.str(), "parameter set");
        });
        auto result = lastSetRequestResult[getThreadId()];
        if (result.first == false) {
            // There was a remote error, append its info to the exception
            throw ParameterException("Remote parameter error: " + result.second);
        } else {
            // Local preliminary value update - the (successful!) async remote update may need additional time.
            // The actual value MIGHT have been revised by the server, but in the vast majority of cases this allows
            // reading back the successfully written parameter. The safest way is via setParameterUpdateCallback.
            auto& param = paramSet[id];
            if (param.isScalar()) {
                param.setCurrent<T>(value);
            } else {
                // Should not be required here (only for the std::string specialization below)
                auto toks = spaceTokenizer.tokenize(std::to_string(value));
                std::vector<double> vs;
                for (auto& t: toks) vs.push_back(atof(t.c_str()));
                param.setTensorData(vs);
            }
        }
    } else {
        // 'Fire and forget' immediate-return mode, e.g. for sending a trigger
        sendNetworkCommand(ss.str(), "parameter set");
    }
}

// Explicit instantiation for std::string
template<>
void ParameterTransfer::writeParameter(const char* id, const std::string& value, bool synchronous) {
    waitNetworkReady();
    if (networkError) {
        // collecting deferred error from background thread
        throw TransferException("ParameterTransfer currently not operational: " + networkErrorString);
    }
    if (!paramSet.count(id)) {
        throw ParameterException("Invalid parameter: " + std::string(id));
    }

    // Assemble a set request with our thread id - the receiver thread can unblock us based on that ID
    //  For 'fire-and-forget' commands without reply, we use -1
    std::stringstream ss;
    ss << "S" << "\t" << (synchronous ? getThreadId() : -1) << "\t" << id << "\t" << value << "\n";

    if (synchronous) {
        blockingCallThisThread([this, &id, &value, &ss](){
            sendNetworkCommand(ss.str(), "parameter set");
        });
        auto result = lastSetRequestResult[getThreadId()];
        if (result.first == false) {
            // There was a remote error, append its info to the exception
            throw ParameterException("Remote parameter error: " + result.second);
        } else {
            // Local preliminary value update - the (successful!) async remote update may need additional time.
            // The actual value MIGHT have been revised by the server, but in the vast majority of cases this allows
            // reading back the successfully written parameter. The safest way is via setParameterUpdateCallback.
            auto& param = paramSet[id];
            if (param.isScalar()) {
                param.setCurrent<std::string>(value);
            } else {
                auto toks = spaceTokenizer.tokenize(value);
                std::vector<double> vs;
                for (auto& t: toks) vs.push_back(atof(t.c_str()));
                param.setTensorData(vs);
            }
        }
    } else {
        // 'Fire and forget' immediate-return mode, e.g. for sending a trigger
        sendNetworkCommand(ss.str(), "parameter set");
    }
}

template<typename T>
void ParameterTransfer::writeParameterTransactionGuardedImpl(const char* id, const T& value) {
    if (writingProhibited) {
        throw ParameterException("Writing parameters is not valid inside an unthreaded event handler");
    }
    if (transactionInProgress) {
        if (!paramSet.count(id)) {
            throw ParameterException("Invalid parameter: " + std::string(id));
        }
        // Queue is thread_local
        transactionQueuedWrites.push_back({std::string(id), std::to_string(value)});
    } else {
        // No transaction, immediate dispatch
        writeParameter(id, value);
    }
}

template<typename T>
void ParameterTransfer::writeParameterTransactionUnguardedImpl(const char* id, const T& value) {
    // No transaction, immediate dispatch
    writeParameter(id, value, false);
}

// Explicit instantiation for std::string
template<>
void ParameterTransfer::writeParameterTransactionGuarded(const char* id, const std::string& value) {
    if (writingProhibited) {
        throw ParameterException("Writing parameters is not valid inside an unthreaded event handler");
    }
    if (transactionInProgress) {
        if (!paramSet.count(id)) {
            throw ParameterException("Invalid parameter: " + std::string(id));
        }
        // Queue is thread_local
        transactionQueuedWrites.push_back({std::string(id), value});
    } else {
        // No transaction, immediate dispatch
        writeParameter(id, value);
    }
}

template<>
void ParameterTransfer::writeParameterTransactionGuarded(const char* id, const double& value) {
    writeParameterTransactionGuardedImpl<double>(id, value);
}

template<>
void ParameterTransfer::writeParameterTransactionGuarded(const char* id, const int& value) {
    writeParameterTransactionGuardedImpl<int>(id, value);
}

template<>
void ParameterTransfer::writeParameterTransactionGuarded(const char* id, const bool& value) {
    writeParameterTransactionGuardedImpl<bool>(id, value);
}

template<>
void ParameterTransfer::writeParameterTransactionUnguarded(const char* id, const bool& value) {
    writeParameterTransactionUnguardedImpl<bool>(id, value);
}

int ParameterTransfer::readIntParameter(const char* id) {
    waitNetworkReady();
    if (networkError) {
        // collecting deferred error from background thread
        throw TransferException("ParameterTransfer currently not operational: " + networkErrorString);
    }
    if (!paramSet.count(id)) {
        throw ParameterException("Invalid parameter: " + std::string(id));
    }
    return paramSet[id].getCurrent<int>();
}

double ParameterTransfer::readDoubleParameter(const char* id) {
    waitNetworkReady();
    if (networkError) {
        // collecting deferred error from background thread
        throw TransferException("ParameterTransfer currently not operational: " + networkErrorString);
    }
    if (!paramSet.count(id)) {
        throw ParameterException("Invalid parameter: " + std::string(id));
    }
    return paramSet[id].getCurrent<double>();
}

bool ParameterTransfer::readBoolParameter(const char* id) {
    waitNetworkReady();
    if (networkError) {
        // collecting deferred error from background thread
        throw TransferException("ParameterTransfer currently not operational: " + networkErrorString);
    }
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
    if (networkError) {
        // collecting deferred error from background thread
        throw TransferException("ParameterTransfer currently not operational: " + networkErrorString);
    }
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
                //std::cerr << "Failed to reconnect to parameter server." << std::endl;
                // Sleep receiver thread and retry reconnection in next iteration
                std::this_thread::sleep_for(std::chrono::milliseconds(SOCKET_RECONNECT_INTERVAL_MS));
            }
        } else {
            // Regular connection state - handle incoming events and replies
            int bytesReceived = recv(socket, recvBuf+recvBufBytes, (RECV_BUF_SIZE - recvBufBytes), 0);
            if (bytesReceived < 0) {
                auto err = Networking::getErrno();
                if(err == EAGAIN || err == EWOULDBLOCK || err == ETIMEDOUT) {
                    // No event or reply - no problem
                    //std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                } else {
                    //std::cerr << "Network error (will periodically attempt reconnection)." << std::endl;
                    std::unique_lock<std::mutex> localLock(socketModificationMutex);
                    Networking::closeSocket(socket);
                    socket = INVALID_SOCKET;
                    refTime = std::chrono::steady_clock::now();
                    networkReady = false;
                    recvBufBytes = 0;
                    networkError = true;
                    networkErrorString = std::string("Error receiving network packet: ") + Networking::getLastErrorString();
                    if (connectionStateChangeCallback) {
                        std::thread([&](){connectionStateChangeCallback(ConnectionState::DISCONNECTED);}).detach();
                    }
                    continue;
                }
            } else if (bytesReceived == 0) {
                //std::cerr << "Connection closed by remote side (will periodically attempt reconnection)." << std::endl;
                std::unique_lock<std::mutex> localLock(socketModificationMutex);
                Networking::closeSocket(socket);
                socket = INVALID_SOCKET;
                refTime = std::chrono::steady_clock::now();
                networkReady = false;
                recvBufBytes = 0;
                networkError = true;
                networkErrorString = "Connection closed";
                if (connectionStateChangeCallback) {
                    std::thread([&](){connectionStateChangeCallback(ConnectionState::DISCONNECTED);}).detach();
                }
                continue;
            } else {
                recvBufBytes += bytesReceived;
            }
            unsigned int start=0;
            for (unsigned int i=0; i<recvBufBytes; ++i) {
                unsigned char c = recvBuf[i];
                if (c=='\n') {
                    std::string currentLine((const char*) recvBuf+start, i-start);
                    //std::cout << "PARAM RECV: " << currentLine << std::endl;
                    auto toks = tabTokenizer.tokenize(currentLine);
                    if (toks.size()>0) {
                        const std::string& cmd = toks[0];
                        if (cmd=="P") {
                            if (toks.size()>1) {
                                // Check of protocol version - old (non-nvparam) firmwares do not send a newline-terminated version, which will just time out waitNetworkReady()
                                long reportedVersionMajor = atol(toks[1].c_str());
                                if(reportedVersionMajor != static_cast<unsigned int>(InternalInformation::CURRENT_PARAMETER_PROTOCOL_VERSION_MAJOR)) {
                                    // Unhandled / incompatible version
                                    networkError = true;
                                    networkErrorString = std::string("Protocol major version mismatch, expected ") + std::to_string(InternalInformation::CURRENT_PARAMETER_PROTOCOL_VERSION_MAJOR) + " but got " + toks[1];
                                    threadRunning = false;
                                    // Wake up the network wait (to propagate error quickly)
                                    std::lock_guard<std::mutex> readyLock(readyMutex);
                                    readyCond.notify_all();
                                    break;
                                }
                                long reportedVersionMinor = -1; // = unreported, legacy version
                                if (toks.size()>2) {
                                    // Minor version reported
                                    reportedVersionMinor = atol(toks[2].c_str());
                                    if (reportedVersionMinor > static_cast<unsigned int>(InternalInformation::CURRENT_PARAMETER_PROTOCOL_VERSION_MINOR)) {
                                        std::cerr << "Caution: remote parameter protocol version " << reportedVersionMajor << "." << reportedVersionMinor
                                            << " is newer than our version " <<static_cast<unsigned int>(InternalInformation::CURRENT_PARAMETER_PROTOCOL_VERSION_MAJOR)
                                            << "." << static_cast<unsigned int>(InternalInformation::CURRENT_PARAMETER_PROTOCOL_VERSION_MINOR) << std::endl;
                                        std::cerr << "Consider a library upgrade for maximum compatibility." << std::endl;
                                    }
                                    // Further toks fields are reserved for future extensions
                                }
                                if (reportedVersionMinor == -1) {
                                    // Device is protocol 7.0, batch transactions added in 7.1 --> fallback: write parameters one-by-one
                                    std::cerr << "Warning: remote firmware is out of date - parameter batch transaction support disabled." << std::endl;
                                    featureDisabledTransactions = true;
                                } else {
                                    // Device accepts full version description handshake, report ours
                                    std::stringstream ss;
                                    ss << "P" << "\t" << (unsigned int) visiontransfer::internal::InternalInformation::CURRENT_PARAMETER_PROTOCOL_VERSION_MAJOR
                                        << "\t" << (unsigned int) visiontransfer::internal::InternalInformation::CURRENT_PARAMETER_PROTOCOL_VERSION_MINOR << "\n";
                                    {
                                        std::unique_lock<std::mutex> localLock(socketModificationMutex);
                                        if (socket == INVALID_SOCKET) {
                                            throw TransferException("Connection has been closed and not reconnected so far");
                                        }
                                        size_t written = send(socket, ss.str().c_str(), (int) ss.str().size(), 0);
                                        if(written != ss.str().size()) {
                                            throw TransferException("Error sending protocol version handshake reply: " + Networking::getLastErrorString());
                                        }
                                    }
                                }
                            } else {
                                networkError = true;
                                networkErrorString = "Incomplete transfer of protocol version";
                                threadRunning = false;
                                break;
                            }
                        } else if (cmd=="I") {
                            // Full parameter info (value and metadata): add or overwrite local parameter
                            Parameter param = ParameterSerialization::deserializeParameterFullUpdate(toks);
                            auto uid = param.getUid();
                            bool alreadyPresent = paramSet.count(uid);
                            paramSet[uid] = param;
                            if (networkReady) {
                                if (alreadyPresent && parameterUpdateCallback) {
                                    // Only call the user callback for metadata updates, but not for the initial enumeration
                                    if (parameterUpdateCallbackThreaded) {
                                        std::thread([&, uid](){parameterUpdateCallback(uid);}).detach();
                                    } else {
                                        writingProhibited = true; // thread_local
                                        parameterUpdateCallback(uid);
                                        writingProhibited = false;
                                    }
                                }
                            }
                        } else if (cmd=="M") {
                            // Metadata-only update: overwrite an existing local parameter, but preserve its previous value.
                            if (paramSet.count(toks[1])) {
                                Parameter param = ParameterSerialization::deserializeParameterFullUpdate(toks, "M");
                                auto uid = param.getUid();
                                param.setCurrentFrom(paramSet[uid]);
                                paramSet[uid] = param;
                                if (networkReady) {
                                    if (parameterUpdateCallback) {
                                        if (parameterUpdateCallbackThreaded) {
                                            std::thread([&, uid](){parameterUpdateCallback(uid);}).detach();
                                        } else {
                                            writingProhibited = true; // thread_local
                                            parameterUpdateCallback(uid);
                                            writingProhibited = false;
                                        }
                                    }
                                }
                            } else {
                                std::cerr << "Parameter not received yet - not updating metadata of: " << toks[1] << std::endl;;
                            }
                        } else if (cmd=="V") {
                            // Current-value-only update
                            if (toks.size() < 3) {
                                throw TransferException("Received malformed parameter value update");
                            }
                            if (paramSet.count(toks[1])) {
                                // In-place update
                                ParameterSerialization::deserializeParameterValueChange(toks, paramSet[toks[1]]);
                                std::string uid = toks[1];
                                if (networkReady) {
                                    if (parameterUpdateCallback) {
                                        if (parameterUpdateCallbackThreaded) {
                                            std::thread([&, uid](){parameterUpdateCallback(uid);}).detach();
                                        } else {
                                            writingProhibited = true; // thread_local
                                            parameterUpdateCallback(uid);
                                            writingProhibited = false;
                                        }
                                    }
                                }
                            } else {
                                std::cerr << "Parameter not received yet - not updating value of: " << toks[1] << std::endl;;
                            }
                        } else if (cmd=="R") {
                            // Reply with asynchronous result of a request to set parameter[s]
                            if (toks.size() < 4) {
                                throw TransferException("Received malformed reply for parameter set request");
                            }
                            std::unique_lock<std::mutex> globalLock(mapMutex);
                            auto subToks = slashTokenizer.tokenize(toks[1]);
                            int replyThreadId = atol(subToks[0].c_str());
                            std::string unblockClass = ""; // filter by block class so we can ignore non-matching replies
                            if (subToks.size() > 1) {
                                unblockClass = subToks[1];
                            }
                            bool hasCond = waitConds.count(replyThreadId);
                            if (hasCond && (waitCondClasses[replyThreadId]==unblockClass)) {
                                // Reanimating the waiting thread - it will clean up after itself
                                std::lock_guard<std::mutex> localLock(waitCondMutexes[replyThreadId]);
                                lastSetRequestResult[replyThreadId] = {toks[2] == "1", toks[3]};
                                waitConds[replyThreadId].notify_all();
                            } else {
                                if (replyThreadId != -1) { // dummy ID -1 for fire-and-forget command (no reply expected)
                                    std::cerr << "Ignoring unexpected request result " << toks[1] << " for thread " << replyThreadId << std::endl;
                                }
                            }
                        } else if (cmd=="E") {
                            // 'End of Transmission' - at least one full enumeration has arrived - we are ready
                            networkReady = true;
                            // Wake any sleeping threads that were blocked until network became ready
                            std::lock_guard<std::mutex> readyLock(readyMutex);
                            readyCond.notify_all();
                            // The actual ready state is the user-visible connected state
                            if (connectionStateChangeCallback) {
                                std::thread([&](){connectionStateChangeCallback(ConnectionState::CONNECTED);}).detach();
                            }
                        } else if (cmd=="HB") {
                            // Heartbeat
                        } else if (cmd=="X") {
                            // Reserved extension
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

void ParameterTransfer::blockingCallThisThread(std::function<void()> fn, int waitMaxMilliseconds, const std::string& waitClass) {
    bool timeout = false;
    auto tid = getThreadId();
    {
        std::unique_lock<std::mutex> globalLock(mapMutex);
        // Populate maps
        auto& localWaitCond = waitConds[tid];
        auto& localWaitCondMutex = waitCondMutexes[tid];
        waitCondClasses[tid] = waitClass;
        std::unique_lock<std::mutex> localLock(localWaitCondMutex);
        // First do the actual handshake setup, like emitting the network message
        // (The current thread is protected against a reply race at this point)
        fn();
        // Allow receiver thread to access its checks (it is still blocked by our specific localLock)
        globalLock.unlock();
        // Wait for receiver thread to notify us with the reply
        auto status = localWaitCond.wait_for(localLock, std::chrono::milliseconds(waitMaxMilliseconds));
        timeout = (status == std::cv_status::timeout);
    }
    {
    // Cleanup, so that any spurious network replies can get detected and discarded
        std::unique_lock<std::mutex> globalLock(mapMutex);
        waitConds.erase(tid);
        waitCondMutexes.erase(tid);
        waitCondClasses.erase(tid);
    }
    // Outcome
    if (timeout) {
        TimeoutException ex("Timeout waiting for request reply from parameter server");
        throw ex;
    }
}

ParameterSet& ParameterTransfer::getParameterSet() {
    waitNetworkReady();
    if (networkError) {
        // collecting deferred error from background thread
        throw TransferException("ParameterTransfer currently not operational: " + networkErrorString);
    }
    return paramSet;
}

ParameterSet const& ParameterTransfer::getParameterSet() const {
    waitNetworkReady();
    if (networkError) {
        // collecting deferred error from background thread
        throw TransferException("ParameterTransfer currently not operational: " + networkErrorString);
    }
    return paramSet;
}

void ParameterTransfer::setParameterUpdateCallback(std::function<void(const std::string& uid)> callback, bool threaded) {
    parameterUpdateCallbackThreaded = threaded;
    parameterUpdateCallback = callback;
}

void ParameterTransfer::transactionStartQueue() {
    // N.B. the flag is thread_local static
    if (featureDisabledTransactions) {
        // Fallback mode for outdated firmware versions -> ignore transaction batching
        return;
    }
    if (transactionInProgress) throw TransferException("Simultaneous and/or nested parameter transactions are not supported");
    transactionInProgress = true;
    // We are now in batch-write mode
}

void ParameterTransfer::transactionCommitQueue(int maxWaitMilliseconds) {
    static int nextTransactionId = 0;
    if (featureDisabledTransactions) {
        // Fallback mode for outdated firmware versions -> ignore transaction batching
        return;
    }

    if (!transactionInProgress) return; // Already released

    if (std::uncaught_exceptions() > 0) {
        // Transaction is NOT finalized during exception unwind
        transactionInProgress = false; // and cannot retry
        return;
    }

    // Send queued parameter transactions
    try {
        waitNetworkReady();
        if (networkError) {
            // collecting deferred error from background thread
            throw TransferException("ParameterTransfer currently not operational: " + networkErrorString);
        }

        // If there are no actual changes, do not send anything
        if (transactionQueuedWrites.size() > 0) {
            // Collect affected UIDs for transaction
            std::set<std::string> affectedUids;
            for (auto& kv: transactionQueuedWrites) {
                affectedUids.insert(kv.first);
            }

            // Start transaction on server, incorporating all affected UIDs
            std::string uniqueTransactionId = std::to_string(nextTransactionId++);
            std::string transactionId;
            if (maxWaitMilliseconds > 0) {
                transactionId = std::to_string(getThreadId()) + "/" + uniqueTransactionId; // use unique ID as unblock filter
            } else {
                // Marked as fire-and-forget (ignore later committed message)
                transactionId = std::to_string(-1) + "/" + uniqueTransactionId; // use unique ID as unblock filter
            }
            std::stringstream ss;
            ss << "TS" << "\t" << transactionId << "\t";
            bool first = true;
            for (auto& uid: affectedUids) {
                if (first) first=false; else ss << ",";
                ss << uid;
            }
            ss << "\n";
            sendNetworkCommand(ss.str(), "transaction start");

            // Play back queued writes
            for (auto& kv: transactionQueuedWrites) {
                auto& uid = kv.first;
                auto& value = kv.second;
                writeParameter(uid.c_str(), value);
            }

            // Finish transaction on server - automatic updates are then applied (and resumed).
            // The transaction will be finalized anyway on the server, but only after a timeout.
            std::stringstream ssEnd;
            ssEnd << "TE" << "\t" << transactionId << "\n";

            // Block for the returning 'completed' signal, if requested
            if (maxWaitMilliseconds > 0) {
                try {
                    blockingCallThisThread([this, &ssEnd](){
                        sendNetworkCommand(ssEnd.str(), "transaction end");
                    }, maxWaitMilliseconds, uniqueTransactionId); // timeout and unblock class
                } catch(...) {
                    transactionQueuedWrites.clear();
                    transactionInProgress = false; // May not retry
                    throw;
                }
                auto result = lastSetRequestResult[getThreadId()];
                if (result.first == false) {
                    // There was a remote error, append its info to the exception
                    throw ParameterException("Remote transaction error: " + result.second);
                }
            } else {
                sendNetworkCommand(ssEnd.str(), "transaction end");
            }

            // Cleanup
            transactionQueuedWrites.clear();
        }
    } catch(...) {
        transactionInProgress = false; // May not retry
        throw;
    }

    transactionInProgress = false;
}

void ParameterTransfer::persistParameters(const std::vector<std::string>& uids, bool synchronous) {
    if (writingProhibited) {
        throw ParameterException("Saving parameters is not valid inside an unthreaded event handler");
    }

    if (transactionInProgress) {
        throw TransferException("Saving parameters is invalid with an open transaction");
    }

    waitNetworkReady();
    if (networkError) {
        // collecting deferred error from background thread
        throw TransferException("ParameterTransfer currently not operational: " + networkErrorString);
    }

    std::stringstream ss;
    ss << "p" << "\t" << (synchronous ? getThreadId() : -1) << "\t";   // "p" -> persist request
    bool first = true;
    for (auto& id: uids) {
        if (first) {
            first=false;
        } else {
            ss << ",";
        }
        ss << id;
        if (!paramSet.count(id)) {
            throw ParameterException("Invalid parameter: " + std::string(id));
        }
    }
    ss << "\n";

    if (synchronous) {
        blockingCallThisThread([this, &ss](){
            sendNetworkCommand(ss.str(), "parameter persist");
        });
        auto result = lastSetRequestResult[getThreadId()];
        if (result.first == false) {
            // There was a remote error, append its info to the exception
            throw ParameterException("Remote parameter error: " + result.second);
        }
    } else {
        // 'Fire and forget' immediate-return mode, e.g. for sending a trigger
        sendNetworkCommand(ss.str(), "parameter persist");
    }
}

void ParameterTransfer::pollParameter(const std::string& uid, bool synchronous) {
    if (writingProhibited) {
        throw ParameterException("Polling parameters is not valid inside an unthreaded event handler");
    }

    if (transactionInProgress) {
        throw TransferException("Polling parameters is invalid within an open transaction");
    }

    waitNetworkReady();
    if (networkError) {
        // collecting deferred error from background thread
        throw TransferException("ParameterTransfer currently not operational: " + networkErrorString);
    }

    std::stringstream ss;
    // cmd "O" -> pOll for update
    ss << "O" << "\t" << (synchronous ? getThreadId() : -1) << "\t" << uid << "\n";

    if (synchronous) {
        blockingCallThisThread([this, &ss](){
            sendNetworkCommand(ss.str(), "parameter poll");
        });
        auto result = lastSetRequestResult[getThreadId()];
        if (result.first == false) {
            // There was a remote error, append its info to the exception
            throw ParameterException("Remote parameter error: " + result.second);
        }
    } else {
        // 'Fire and forget' immediate-return mode (force update, do not wait for new value)
        sendNetworkCommand(ss.str(), "parameter poll");
    }
}

void ParameterTransfer::setConnectionStateChangeCallback(std::function<void(visiontransfer::ConnectionState)> callback) {
    connectionStateChangeCallback = callback;
}

}} // namespace

