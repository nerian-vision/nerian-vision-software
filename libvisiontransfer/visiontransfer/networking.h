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

/*******************************************************************************
 * This header file contains include statements and definitions for simplifying
 * cross platform network development
*******************************************************************************/

#ifndef VISIONTRANSFER_NETWORKING_H
#define VISIONTRANSFER_NETWORKING_H

// Network headers
#ifdef _WIN32
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x501
    #endif
    #define _WINSOCK_DEPRECATED_NO_WARNINGS

    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    #include <string>
    #include <cstdio>
    #include <winsock2.h>
    #include <ws2tcpip.h>

    #include <Ipmib.h>
    #include <Iprtrmib.h>
    #include <Iphlpapi.h>

    // Some defines to make windows socket look more like
    // posix sockets.
    #ifdef EWOULDBLOCK
        #undef EWOULDBLOCK
    #endif
    #ifdef ECONNRESET
        #undef ECONNRESET
    #endif
    #ifdef ETIMEDOUT
        #undef ETIMEDOUT
    #endif
    #ifdef EPIPE
        #undef EPIPE
    #endif

    #define EWOULDBLOCK WSAEWOULDBLOCK
    #define ECONNRESET WSAECONNRESET
    #define ETIMEDOUT WSAETIMEDOUT
    #define EPIPE WSAECONNABORTED
    #define MSG_DONTWAIT 0
    #define SHUT_WR SD_BOTH

    inline int close(SOCKET s) {
        return closesocket(s);
    }

    // Visual studio does not come with snprintf
    #ifndef snprintf
        #define snprintf _snprintf_s
    #endif

    typedef int socklen_t;

    typedef unsigned long error_int_type;

#else
    #include <arpa/inet.h>
    #include <netinet/tcp.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <errno.h>
    #include <unistd.h>
    #include <signal.h>
    #include <ifaddrs.h>
    #include <poll.h>

    #include <string>

    // Unfortunately we have to use a winsock like socket type
    typedef int SOCKET;
    #define INVALID_SOCKET -1

    // Also we need some additional winsock defines
    #define WSA_IO_PENDING 0
    #define WSAECONNRESET 0

    typedef int error_int_type;

#endif

namespace visiontransfer {
namespace internal {

/**
 * \brief A collection of helper functions for implementing network communication.
 */
class Networking {
public:
    static void initNetworking();
    static addrinfo* resolveAddress(const char* address, const char* service);
    static SOCKET connectTcpSocket(const addrinfo* address);
    static void setSocketTimeout(SOCKET socket, int timeoutMillisec);
    static void closeSocket(SOCKET& socket);
    static void setSocketBlocking(SOCKET socket, bool blocking);
    static void enableReuseAddress(SOCKET socket, bool reuse);
    static void bindSocket(SOCKET socket, const addrinfo* addressInfo);
    static SOCKET acceptConnection(SOCKET socket, sockaddr_in& remoteAddress);
    static error_int_type getErrno();
    static std::string getErrorString(error_int_type error);
    static std::string getLastErrorString();

};

}} // namespace

#endif
