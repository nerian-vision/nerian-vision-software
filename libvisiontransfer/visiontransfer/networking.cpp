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

#include "visiontransfer/networking.h"
#include "visiontransfer/exceptions.h"
#include <cstring>
#include <fcntl.h>

using namespace std;
using namespace visiontransfer;
using namespace visiontransfer::internal;

namespace visiontransfer {
namespace internal {

// System-specific error message wrappers
#ifdef _WIN32
    error_int_type Networking::getErrno() {
        return WSAGetLastError();
    }

    std::string Networking::getErrorString(error_int_type error) {
        char* str = nullptr;
        if(FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&str, 0, nullptr) == 0 || str == nullptr) {
            return "Unknown error";
        } else {
            char buffer[512];
            snprintf(buffer, sizeof(buffer), "%s (%lu)", str, error);
            LocalFree(str);
            return std::string(buffer);
        }
    }
#else
    error_int_type Networking::getErrno() {
        return errno;
    }
    std::string Networking::getErrorString(error_int_type error) {
        return std::string(strerror(error));
    }
#endif

std::string Networking::getLastErrorString() {
    return getErrorString(getErrno());
}

void Networking::initNetworking() {
#ifdef _WIN32
    // In windows, we first have to initialize winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw TransferException("WSAStartup failed!");
    }
#endif
}

addrinfo* Networking::resolveAddress(const char* address, const char* service) {
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    addrinfo* addressInfo = nullptr;

    if(getaddrinfo(address, service, &hints, &addressInfo) != 0 || addressInfo == nullptr) {
        TransferException ex("Error resolving address: " + getLastErrorString());
        throw ex;
    }

    if(addressInfo->ai_addrlen != sizeof(sockaddr_in)) {
        throw TransferException("Illegal address length");
    }

    return addressInfo;
}

SOCKET Networking::connectTcpSocket(const addrinfo* address) {
    SOCKET sock = ::socket(address->ai_family, address->ai_socktype,
        address->ai_protocol);
    if(sock == INVALID_SOCKET) {
        TransferException ex("Error creating socket: " + getLastErrorString());
        throw ex;
    }

    if(connect(sock, address->ai_addr, static_cast<int>(address->ai_addrlen)) < 0) {
        TransferException ex("Error connection to destination address: " + getLastErrorString());
        throw ex;
    }

    return sock;
}

void Networking::setSocketTimeout(SOCKET socket, int timeoutMillisec) {
#ifdef _WIN32
    unsigned int timeout = timeoutMillisec;
#else
    struct timeval timeout;
    timeout.tv_sec = timeoutMillisec/1000;
    timeout.tv_usec = timeoutMillisec*1000;
#endif

    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));
}

void Networking::closeSocket(SOCKET& socket) {
    setSocketBlocking(socket, false);
    shutdown(socket, SHUT_WR);

    // Receive remaining data
    char buffer[1024];
    for(int i=0; i<3; i++) {
        int received = recv(socket, buffer, sizeof(buffer), 0);
        if(received <= 0) {
            break;
        }
    }

    close(socket);
    socket = INVALID_SOCKET;
}

void Networking::setSocketBlocking(SOCKET socket, bool blocking) {
#ifdef _WIN32
    unsigned long on = (blocking ? 0 : 1);
    ioctlsocket(socket, FIONBIO, &on);
#else
    int flags = fcntl(socket, F_GETFL, 0);
    if(flags != -1) {
        if(blocking) {
            flags &= ~O_NONBLOCK;
        } else {
            flags |= O_NONBLOCK;
        }
        fcntl(socket, F_SETFL, flags);
    }
#endif
}

void Networking::enableReuseAddress(SOCKET socket, bool reuse) {
    int enable = reuse ? 1 : 0;
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&enable), sizeof(int));
}

void Networking::bindSocket(SOCKET socket, const addrinfo* addressInfo) {
    if (::bind(socket, addressInfo->ai_addr, static_cast<int>(addressInfo->ai_addrlen)) < 0)  {
        TransferException ex("Error binding socket: " + getLastErrorString());
        throw ex;
    }
}

SOCKET Networking::acceptConnection(SOCKET socket, sockaddr_in& remoteAddress) {
    socklen_t clientAddressLength = sizeof(sockaddr_in);

    SOCKET newSocket = accept(socket, reinterpret_cast<sockaddr *>(&remoteAddress),
        &clientAddressLength);

    if(clientAddressLength != sizeof(sockaddr_in)) {
        throw TransferException("Received network address with invalid length");
    }

    if(newSocket == INVALID_SOCKET) {
        if(getErrno() == EWOULDBLOCK || getErrno() == ETIMEDOUT) {
            // No connection
            return INVALID_SOCKET;
        } else {
            TransferException ex("Error accepting connection: " + getLastErrorString());
            throw ex;
        }
    }

    return newSocket;
}

}} // namespace

