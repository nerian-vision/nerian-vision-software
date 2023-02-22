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

#ifndef VISIONTRANSFER_ASYNCTRANSFER_H
#define VISIONTRANSFER_ASYNCTRANSFER_H

#include "visiontransfer/common.h"
#include "visiontransfer/imagetransfer.h"
#include "visiontransfer/imageset.h"
#include "visiontransfer/imageprotocol.h"
#include "visiontransfer/deviceinfo.h"

namespace visiontransfer {

/**
 * \brief Class for asynchronous transfer of image sets.
 *
 * This class opens a network socket for delivering or receiving image sets. All
 * operations are performed asynchronously, which means that they do not block.
 * The class encapsulates ImageTransfer.
 */
class VT_EXPORT AsyncTransfer {
public:
    /**
     * \brief Creates a new transfer object.
     *
     * \param address Address of the remote host to which a connection
     *        should be established. In server mode this can be a local
     *        interface address or NULL.
     * \param service The port number that should be used as string or
     *        as textual service name.
     * \param protType Specifies whether the UDP or TCP transport protocol
     *        shall be used.
     * \param server If set to true, this object will be a communication server.
     * \param bufferSize Buffer size for sending / receiving network data.
     * \param maxUdpPacketSize Maximum allowed size of a UDP packet when sending data.
     *
     * Please see ImageTransfer::ImageTransfer() for further details.
     */
    AsyncTransfer(const char* address, const char* service = "7681",
        ImageProtocol::ProtocolType protType = ImageProtocol::PROTOCOL_UDP,
        bool server = false, int bufferSize = 16*1048576, int maxUdpPacketSize = 1472);

    /**
     * \brief Creates a new transfer object by using the device information
     * from device enumeration
     *
     * \param device Information on the device to which a connection should
     *        be established.
     * \param bufferSize Buffer size for sending / receiving network data.
     * \param maxUdpPacketSize Maximum allowed size of a UDP packet when sending data.
     */
    AsyncTransfer(const DeviceInfo& device, int bufferSize = 16*1048576, int maxUdpPacketSize = 1472);

    ~AsyncTransfer();

    /**
     * \brief Starts an asynchronous transmission of the given image set
     *
     * \param imageSet The image set that shall be transmitted.
     * \param deleteData If set to true, the pointers to the pixel data that
     *        are contained in \c imageSet, will be deleted after the
     *        image set has been transmitted.
     *
     * If deleteData is set to false, the pixel data contained in \c imageSet
     * must not be freed before the data has been transmitted. As transmission
     * happens asynchronously, it is recommended to let AsyncTransfer delete
     * the data pointers.
     */
    void sendImageSetAsync(const ImageSet& imageSet, bool deleteData = false);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    DEPRECATED("Use sendImageSetAsync() instead")
    inline void sendImagePairAsync(const ImageSet& imageSet, bool deleteData = false) {
        sendImageSetAsync(imageSet, deleteData);
    }
#endif

    /**
     * \brief Collects the asynchronously received image.
     *
     * \param imageSet The received image set.
     * \param timeout The maximum time in seconds for which to wait if no
     *        image set has been received yet.
     * \return True if an image set has been received before the timeout.
     *
     * If no image set has been received, this method might block or return false.
     * Otherwise the returned image set is valid until the next call.
     *
     * If timeout is set to a value < 0, the function will block indefinitely.
     * If timeout = 0, the function will return immediately, and if timeout is > 0 then
     * the function will block for the given amount of time in seconds. The received
     * image set is only valid until the next call of collectReceivedImageSet().
     */
    bool collectReceivedImageSet(ImageSet& imageSet, double timeout = -1);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    DEPRECATED("Use collectReceivedImageSet() instead")
    inline bool collectReceivedImagePair(ImageSet& imageSet, double timeout = -1) {
        return collectReceivedImageSet(imageSet, timeout);
    }
#endif

    /**
     * \brief Returns the number of frames that have been dropped since
     * connecting to the current remote host.
     *
     * Dropped frames are caused by dropped packets due to a poor network
     * connection
     */
    int getNumDroppedFrames() const;

    /**
     * \brief Tries to accept a client connection.
     *
     * \return True if a client has connected..
     *
     * This method can only be used in TCP server mode. It shall be called in
     * regular intervals to allow for client connections. The method is
     * non-blocking.
     */
    bool tryAccept();

    /**
     * \brief Returns true if a remote connection is established
     */
    bool isConnected() const;

    /**
     * \brief Terminates the current connection.
     *
     * If connected to a remote host this connection will be closed.
     */
    void disconnect();

    /**
     * \brief Returns the address of the remote host
     *
     * \return Remote address or "" if no connection has been established.
     */
    std::string getRemoteAddress() const;

private:
    // We follow the pimpl idiom
    class Pimpl;
    Pimpl* pimpl;

    // This class cannot be copied
    AsyncTransfer(const AsyncTransfer& other);
    AsyncTransfer& operator=(const AsyncTransfer&);
};

} // namespace

#endif
