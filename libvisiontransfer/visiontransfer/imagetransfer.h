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

#ifndef VISIONTRANSFER_IMAGETRANSFER_H
#define VISIONTRANSFER_IMAGETRANSFER_H

#include <string>
#include "visiontransfer/common.h"
#include "visiontransfer/types.h"
#include "visiontransfer/imageprotocol.h"
#include "visiontransfer/imageset.h"
#include "visiontransfer/deviceinfo.h"
#include "visiontransfer/externalbuffer.h"

#if VISIONTRANSFER_CPLUSPLUS_VERSION >= 201103L
#include <functional>
#endif

namespace visiontransfer {

/**
 * \brief Class for synchronous transfer of image sets.
 *
 * This class opens a network socket for delivering or receiving image sets. All
 * operations are performed synchronously, which means that they might block.
 * The class encapsulates ImageProtocol.
 *
 * This class is thread safe for as long as sending and receiving data
 * each has its dedicated thread.
 *
 * **Note: For normal applications and typical use cases, we recommend
 * using AsyncTransfer instead, which places network operations into an
 * automatic background thread, decoupled from your own loop timing.**
 */
class VT_EXPORT ImageTransfer {
public:
    /// A configuration object for creating an ImageTransfer or AsyncTransfer
    class Config {
        public:
            /// Create and populate a config based on an IP address and global defaults
            Config(const char* address);
            /// Create and populate a config based on settings of a discovered device
            Config(DeviceInfo& deviceInfo);
            /// Destroy and deallocate Pimpl
            ~Config();
            /// Override the device address
            Config& setAddress(const char* address);
            /// Override the device image data port (default "7681")
            Config& setService(const char* service);
            /// Override the protocol type (default ImageProtocol::PROTOCOL_UDP)
            Config& setProtocolType(ImageProtocol::ProtocolType protType);
            /// Override the server mode (default false => client mode)
            Config& setServer(bool server);
            /// Override the image buffer size (default 16*1024*1024 => 16 MB)
            Config& setBufferSize(int bufferSize);
            /// Override the maximum packet size for UDP mode (default 1472)
            Config& setMaxUdpPacketSize(int maxUdpPacketSize);
            /// Override the auto-reconnect delay for lost connection
            /// (default of 1 sec; set to 0 to disable)
            Config& setAutoReconnectDelay(int autoReconnectDelay);
            /// Add an external buffer ensemble to the buffer pool. This enables
            /// external buffering mode and disables the default target image
            /// buffer allocation in the library (excepting auxiliary buffers).
            /// More than one buffer set should be added to enable reception
            /// while another ImageSet is still being processed by the user.
            Config& addExternalBufferSet(ExternalBufferSet bufset);

            /// Return the configured target address
            const char* getAddress() const;
            /// Return the configured device image data port
            const char* getService() const;
            /// Return the configured protocol type
            ImageProtocol::ProtocolType getProtocolType() const;
            /// Return the configured server mode
            bool getServer() const;
            /// Return the configured image buffer size
            int getBufferSize() const;
            /// Return the configured maximum packet size for UDP mode
            int getMaxUdpPacketSize() const;
            /// Return the configured auto-reconnect delay (0 = disabled)
            int getAutoReconnectDelay() const;
            /// Add an external buffer ensemble to the buffer pool. This enables
            /// external buffering mode and disables the default target image
            /// buffer allocation in the library (excepting auxiliary buffers).
            /// More than one buffer set should be added to enable reception
            /// while another ImageSet is still being processed by the user.
            /// Return the configured number of external buffer sets
            int getNumExternalBufferSets() const;
            /// Return the configured external buffer set at the specified index
            ExternalBufferSet getExternalBufferSet(int idx) const;
        private:
            class Pimpl;
            Pimpl* pimpl;
    };

    /// The result of a partial image transfer
    enum TransferStatus {
        /// The image set has been transferred completely.
        ALL_TRANSFERRED,

        /// The image set has been transferred partially. Further
        /// transfers are necessary.
        PARTIAL_TRANSFER,

        /// There is currently no more data that could be transmitted.
        NO_VALID_DATA,

        /// The operation would block and blocking as been disabled.
        WOULD_BLOCK,

        /// No network connection has been established
        NOT_CONNECTED
    };

    /**
     * \brief Creates a new transfer object by manually specifying the
     * target address.
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
     * \param autoReconnectDelay Auto-reconnection behavior, see setAutoReconnect
     */
    ImageTransfer(const char* address, const char* service = "7681",
        ImageProtocol::ProtocolType protType = ImageProtocol::PROTOCOL_UDP,
        bool server = false, int bufferSize = 16*1048576, int maxUdpPacketSize = 1472,
        int autoReconnectDelay=1);

    /**
     * \brief Creates a new transfer object by using the device information
     * from device enumeration.
     *
     * \param device Information on the device to which a connection should
     *        be established.
     * \param bufferSize Buffer size for sending / receiving network data.
     * \param maxUdpPacketSize Maximum allowed size of a UDP packet when sending data.
     * \param autoReconnectDelay Auto-reconnection behavior, see setAutoReconnect
     */
    ImageTransfer(const DeviceInfo& device, int bufferSize = 16 * 1048576,
        int maxUdpPacketSize = 1472, int autoReconnectDelay=1);

    /**
     * \brief Creates and initializes an ImageTransfer object based on the
     * specified configuration object
     */
    ImageTransfer(const Config& config);

    ~ImageTransfer();

    /**
     * \brief Sets the raw pixel data for a partial image transmission.
     *
     * This method has to be used in conjunction with transferData().
     * Please see ImageProtocol::setRawTransferData() for further details.
     */
    void setRawTransferData(const ImageSet& metaData, const std::vector<unsigned char*>& rawData,
        int firstTileWidth = 0, int middleTileWidth = 0, int lastTileWidth = 0);

    /**
     * \brief Updates the number of valid bytes in a partial raw transmission.
     *
     * Please see ImageProtocol::setRawValidBytes() for further details.
     */
    void setRawValidBytes(const std::vector<int>& validBytes);

    /**
     * \brief Sets a new image set that shall be transmitted.
     *
     * \param imageSet The image set that shall be transmitted.
     *
     * After setting the image set, subsequent calls to transferData()
     * are necessary for performing the image transmission.
     *
     * \see ImageProtocol::setTransferImageSet()
     */
    void setTransferImageSet(const ImageSet& imageSet);

    /**
     * \brief Performs a partial (or full) image transmission.
     *
     * \param block If set to true, the method will block when the network
     *        send buffers are full.
     * \return Status of the transmission. See below.
     *
     * The method transfers up to the specified number of valid bytes. It has to
     * be called in cycles in order to transfer a full image set. If there
     * is no more data to be transferred, it will return TransferStatus::NO_VALID_DATA.
     *
     * If the transfer is compete, the method will return
     * TransferStatus::ALL_TRANSFERRED. If there remains outstanding data for
     * this transfer, the return value will be TransferStatus::PARTIAL_TRANSFER.
     * If the connection is no longer open, TransferStatus::CONNECTION_CLOSED
     * is returned.
     *
     * Even after a complete transfer this method should be continued to be
     * called in case a packed needs to be retransmitted due to an unreliable
     * network connection. Also for a communication server this method should
     * be called frequently to accept incoming connections.
     */
    TransferStatus transferData();

    /**
     * \brief Waits for and receives a new image set.
     *
     * \param imageSet Will be set to the received image set.
     * \return Returns true if a new image set has been received. Otherwise
     *         false.
     *
     * The received image set is only valid until the next call of receiveImageSet().
     * The method will not block indefinitely, but return after a short timeout.
     *
     * **Note:** The call frequency of this function should be regular, and
     * not be used to limit the frame rate (use the device settings for that.)
     *
     * \see ImageProtocol::getReceivedImageSet()
     */
    bool receiveImageSet(ImageSet& imageSet);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    DEPRECATED("Use receiveImageSet() instead")
    inline bool receiveImagePair(ImageSet& imageSet) {
        return receiveImageSet(imageSet);
    }
#endif

    /**
     * \brief Returns the received image set, even if it is not yet complete.
     *
     * The received image set is only valid until calling receivePartialImageSet()
     * for the first time after the current image set has been received completely.
     * The method returns false if no image data has been received.
     *
     * Please see ImageProtocol::getPartiallyReceivedImageSet() for further details.
     */
    bool receivePartialImageSet(ImageSet& imageSet, int& validRows, bool& complete);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    DEPRECATED("Use receivePartialImageSet() instead")
    inline bool receivePartialImagePair(ImageSet& imageSet, int& validRows, bool& complete) {
        return receivePartialImageSet(imageSet, validRows, complete);
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
     * \return True if a client has connected.
     *
     * This method can only be used in TCP server mode. It shall be called in
     * regular intervals to allow for client connections. The method is
     * always non-blocking.
     */
    bool tryAccept();

    /**
     * \brief Returns true if a remote connection is established (and not temporarily disconnected).
     * For event-driven signaling of this state, see also setConnectionStateChangeCallback()
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    /// Prints status information to the console
    std::string statusReport();
#endif

#if VISIONTRANSFER_CPLUSPLUS_VERSION >= 201103L
    /**
     * \brief Install a handler that will be called when the connection state changes
     * (e.g. socket is disconnected). *[C++11]*
     */
    void setConnectionStateChangeCallback(std::function<void(visiontransfer::ConnectionState)> callback);
#endif

    /*
     * \brief Configure automatic reconnection behavior (for TCP client mode).
     *
     * When enabled, this functionality is initiated whenever a disconnection
     * is detected, instead of just reporting the disconnection.
     * By default, automatic reconnection is active with a 1-second wait time.
     *
     * NOTE: If enabled, ImageTransfer's receiving and sending functions
     * will block indefinitely on disconnection until the connection has been
     * restored. Disconnection will be reported before blocking (see
     * setConnectionStateChangeCallback).
     * 
     * \param secondsBetweenRetries Number of seconds to wait between
     *   consecutive reconnection attempts, or 0 to disable auto-reconnection.
     */
    void setAutoReconnect(int secondsBetweenRetries=1);
    
    /// See AsyncTransfer::signalImageSetDone
    void signalImageSetDone(ImageSet& imageSet);

    /// Rotate to the next free configured ExternalBufferSet (if external buffering is active)
    void assignExternalBuffer();

private:
    // We follow the pimpl idiom
    class Pimpl;
    Pimpl* pimpl;

    // This class cannot be copied
    ImageTransfer(const ImageTransfer& other);
    ImageTransfer& operator=(const ImageTransfer&);
};

} // namespace

#endif
