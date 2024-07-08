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

#ifndef VISIONTRANSFER_PARAMETERTRANSFER_H
#define VISIONTRANSFER_PARAMETERTRANSFER_H

#include "visiontransfer/common.h"
#include "visiontransfer/types.h"
#include "visiontransfer/parameterinfo.h"
#include "visiontransfer/parameterset.h"
#include "visiontransfer/internal/tokenizer.h"
#include "visiontransfer/internal/networking.h"

#include <map>
#include <set>
#include <memory>
#include <thread>
#include <condition_variable>
#include <functional>

namespace visiontransfer {
namespace internal {

/**
 * \brief Allows a configuration of device parameters over the network.
 *
 * A TCP connection is established to a parameter server. The protocol
 * allows writing and reading of individual parameters, which are
 * identified by a unique ID. There are three supported types of
 * parameters: integers, double precision floating point values, and
 * booleans.
 *
 * This class is only used internally. Users should use the class
 * \ref DeviceParameters instead.
 */

class ParameterTransfer {
public:
    /**
     * \brief Creates an object and connects to the given server.
     *
     * \param address   IP address or host name of the server.
     * \param service   The port number that should be used as string or
     *                  as textual service name.
     */
    ParameterTransfer(const char* address, const char* service = "7683");
    ~ParameterTransfer();

    /**
     * \brief Returns whether the background connection is currently up and running
     *  (it may be temporarily false during a disconnected/auto-reconnection phase).
     */
    bool isConnected() const;

    /**
     * \brief Reads an integer value from the parameter server.
     *
     * \param id    Unique ID of the parameter to be read.
     * \return      If successful, the value of the parameter that has
     *              been read
     *
     * If reading the parameter fails, then an exception of type
     * TransferException or ParameterException is thrown.
     */
    int readIntParameter(const char* id);

    /**
     * \brief Reads a double precision floating point value from the
     * parameter server.
     *
     * \param id    Unique ID of the parameter to be read.
     * \return      If successful, the value of the parameter that has
     *              been read
     *
     * If reading the parameter fails, then an exception of type
     * TransferException or ParameterException is thrown.
     */
    double readDoubleParameter(const char* id);

    /**
     * \brief Reads a boolean value from the parameter server.
     *
     * \param id    Unique ID of the parameter to be read.
     * \return      If successful, the value of the parameter that has
     *              been read
     *
     * If reading the parameter fails, then an exception of type
     * TransferException or ParameterException is thrown.
     */
    bool readBoolParameter(const char* id);

    /**
     * \brief Writes an integer value to a parameter of the parameter
     * server.
     *
     * \param id    Unique ID of the parameter to be written.
     * \param value Value that should be written to the parameter.
     *
     * If writing the parameter fails, then an exception of type
     * TransferException or ParameterException is thrown.
     */
    void writeIntParameter(const char* id, int value);

    /**
     * \brief Writes a double precision floating point value to a
     * parameter of the parameter server.
     *
     * \param id    Unique ID of the parameter to be written.
     * \param value Value that should be written to the parameter.
     *
     * If writing the parameter fails, then an exception of type
     * TransferException or ParameterException is thrown.
     */
    void writeDoubleParameter(const char* id, double value);

    /**
     * \brief Writes a boolean value to a parameter of the parameter
     * server.
     *
     * \param id    Unique ID of the parameter to be written.
     * \param value Value that should be written to the parameter.
     *
     * If writing the parameter fails, then an exception of type
     * TransferException or ParameterException is thrown.
     */
    void writeBoolParameter(const char* id, bool value);

    /**
     * \brief Writes a scalar value to a parameter of the parameter
     * server.
     *
     * \param id    Unique ID of the parameter to be written.
     * \param value Value that should be written to the parameter.
     *
     * If writing the parameter fails, then an exception of type
     * TransferException or ParameterException is thrown.
     */
    template<typename T>
    void writeParameter(const char* id, const T& value, bool synchronous=true);

    /**
     * \brief Writes a scalar value to a parameter of the parameter server,
     * transparently deferring for a batch update if a transaction has been started.
     *
     * \param id    Unique ID of the parameter to be written.
     * \param value Value that should be written to the parameter.
     *
     * If writing the parameter fails, then an exception of type
     * TransferException or ParameterException is thrown.
     */
    template<typename T>
    void writeParameterTransactionGuarded(const char* id, const T& value);

    /**
     * \brief Writes a scalar value to a parameter of the parameter server,
     * using 'fire-and-forget' for real-time commands (no replies expected).
     *
     * \param id    Unique ID of the parameter to be written.
     * \param value Value that should be written to the parameter.
     *
     * If writing the parameter fails immediately, an exception of type
     * TransferException is thrown.
     */
    template<typename T>
    void writeParameterTransactionUnguarded(const char* id, const T& value);

    /**
     * \brief Enumerates all parameters as reported by the device.
     */
    std::map<std::string, ParameterInfo> getAllParameters();

    /**
     * \brief Returns a reference to the internal parameter set (once the network handshake is complete)
     */
    param::ParameterSet& getParameterSet();

    /**
     * \brief Returns a reference to the internal parameter set (once the network handshake is complete)
     */
    param::ParameterSet const& getParameterSet() const;

    void setParameterUpdateCallback(std::function<void(const std::string& uid)> callback, bool threaded);

    /**
     * \brief Start batch parameter transaction.
     *
     * Queues all remote set operations for writing in one batch; used for robust dependent parameter recalculation.
     */
    void transactionStartQueue();

    /** \brief Complete the started parameter transaction.
     *
     * Bundles all queued writes into a single locked request and sends them.
     */
    void transactionCommitQueue(int maxWaitMilliseconds);

    /**
     * \brief Requests to save the current values for the specified parameter UIDs to permanent storage
     */
    void persistParameters(const std::vector<std::string>& uids, bool synchronous=true);

    /**
     * \brief Sets the callback function to inform of background disconnection / reconnection
     */
    void setConnectionStateChangeCallback(std::function<void(visiontransfer::ConnectionState)> callback);

    /**
     * \brief Requests to poll for an updated value of the specified parameter UID
     */
    void pollParameter(const std::string& uid, bool synchronous=true);

private:
    static constexpr int SOCKET_TIMEOUT_MS = 500;
    static constexpr int SOCKET_RECONNECT_INTERVAL_MS = 2000;

    // Message types
    static constexpr unsigned char MESSAGE_READ_INT = 0x01;
    static constexpr unsigned char MESSAGE_READ_DOUBLE = 0x02;
    static constexpr unsigned char MESSAGE_READ_BOOL = 0x03;
    static constexpr unsigned char MESSAGE_WRITE_INT = 0x04;
    static constexpr unsigned char MESSAGE_WRITE_DOUBLE = 0x05;
    static constexpr unsigned char MESSAGE_WRITE_BOOL = 0x06;
    static constexpr unsigned char MESSAGE_ENUMERATE_PARAMS = 0x07;

    SOCKET socket;

    std::string address; // for reconnection
    std::string service;

    static constexpr unsigned int RECV_BUF_SIZE = 1024*1024;
    char recvBuf[RECV_BUF_SIZE];
    unsigned int recvBufBytes;
    unsigned int pollDelay;
    bool networkError; // background thread exception flag
    std::string networkErrorString;
    bool networkReady; // after protocol check and initial update

    // Fallback handling for connecting to out-of-date firmware versions
    bool featureDisabledTransactions;

    bool threadRunning;
    std::shared_ptr<std::thread> receiverThread;

    Tokenizer tabTokenizer;
    Tokenizer spaceTokenizer;
    Tokenizer slashTokenizer;

    param::ParameterSet paramSet;

    /// Mutex for the network-ready block
    mutable std::mutex readyMutex;
    /// Cond for the network-ready wait
    mutable std::condition_variable readyCond;
    /// Mutex to guard the request wait cond/mutex maps for atomic emplacement
    std::mutex mapMutex;
    /// Mutex to guard the socket in case of modification (invalidation by disconnect)
    std::mutex socketModificationMutex;
    /// Mutex for calling or modifying the user callback functions
    std::mutex callbackMutex;
    /// Per-thread cond to wait for network reply messages
    std::map<int, std::condition_variable> waitConds;
    /// Condition-specific mutexes
    std::map<int, std::mutex> waitCondMutexes;
    /// Per-thread record for the last received asynchronous set request result
    std::map<int, std::pair<bool, std::string> > lastSetRequestResult;
    /// Per-thread record of the active lock class
    std::map<int, std::string> waitCondClasses;

    /// User-supplied callback function that is invoked for parameter updates (but not the initial enumeration)
    std::function<void(const std::string&)> parameterUpdateCallback;
    bool parameterUpdateCallbackThreaded;

    thread_local static bool transactionInProgress;
    thread_local static std::vector<std::pair<std::string, std::string> > transactionQueuedWrites;

    thread_local static bool writingProhibited;

    /// User-supplied callback function that is invoked for disconnections and reconnections
    std::function<void(visiontransfer::ConnectionState)> connectionStateChangeCallback;

    /// Attempt to connect to configured server (or reconnect in case of dropped connection)
    void attemptConnection();

    /// Block current thread until networkReady (set by the receiver thread)
    void waitNetworkReady() const;

    /// Obtain a basic type thread ID independent of platform / thread implementation
    int getThreadId();

    /// Block current thread, call the functor, and wait with timeout before throwing
    void blockingCallThisThread(std::function<void()>, int waitMaxMilliseconds=1000, const std::string& waitClass="");

    void receiverRoutine();
    void process();

    void readParameter(unsigned char messageType, const char* id, unsigned char* dest, int length);
    void recvData(unsigned char* dest, int length);

    std::map<std::string, ParameterInfo> recvEnumeration();

    template<typename T>
    void writeParameterTransactionGuardedImpl(const char* id, const T& value);

    template<typename T>
    void writeParameterTransactionUnguardedImpl(const char* id, const T& value);

    void sendNetworkCommand(const std::string& cmdline, const std::string& diagStr);

};

}} // namespace

#endif
