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

#ifndef VISIONTRANSFER_IMAGEPROTOCOL_H
#define VISIONTRANSFER_IMAGEPROTOCOL_H

#include "visiontransfer/common.h"
#include "visiontransfer/imageset.h"

#include <vector>

namespace visiontransfer {

/**
 * \brief A lightweight protocol for transferring image sets.
 *
 * Two images are transferred together as a set. These are usually the left
 * and right image of a stereo camera, or the left image and a disparity map.
 *
 * The images are 8- or 12-bit monochrome, or 8-bit RGB color. For simplicity,
 * 12-bit images are inflated to 16-bit by introducing additional padding
 * bits. Both images must always have the same image size.
 *
 * When receiving, the class takes in an image set and chops it down to several
 * network messages. When sending, the class takes a set of messages and
 * assembles them into an image set. We have to differentiate between TCP and
 * UDP in both cases.
 */
class VT_EXPORT ImageProtocol {
public:
    /// Supported network protocols
    enum ProtocolType {
        /// The connection oriented TCP transport protocol
        PROTOCOL_TCP,

        /// The connection-less UDP transport protocol
        PROTOCOL_UDP
    };

    /**
     * \brief Creates a new instance for decoding / encoding network messages
     * for the given network protocol.
     *
     * \param server If set to true, this object will be a communication server.
     * \param maxUdpPacketSize Maximum allowed size of a UDP packet when sending data.
     */
    ImageProtocol(bool server, ProtocolType protType, int maxUdpPacketSize = 1472);

    ~ImageProtocol();

    /**
     * \brief Sets a new image that will be transfer.
     *
     * \param imageSet The image set that shall be transmitted.
     *
     * After setting the transfer image, subsequent calls to
     * getTransferMessage() can be made for obtaining the encoded
     * network messages.
     */
    void setTransferImageSet(const ImageSet& imageSet);

    /**
     * \brief Sets the already pre-formatted image data for the next transfer.
     *
     * \param metaData ImageSet object containing all the meta data but no
     *        pixel data.
     * \param rawData Pre-formatted data for this transfer.
     * \param firstTileWidth If not 0, specifies the width of the first tile in
     *        a tiled transfer.
     * \param middleTilesWidth If not 0, specifies the width of the tiles  between
              the first and the last tile in a tiled transfer.
     * \param lastTileWidth If not 0, specifies the width of the last tile in
     *        a tiled transfer.
     * \param validBytes The number of bytes that are valid in \c rawData.
     *
     * This method is a more efficient alternative to setTransferImage(). In this case
     * the image data already has to be pre-formatted in the format of
     * the image protocol, which means row-wise interleaving both images.
     * For 12-bit images, the pixel data must be packed in LSB order.
     *
     * The pixel data contained in \c metaData is ignored, while all
     * other data is transmitted. The actual pixel data must be encoded in
     * \c rawData.
     *
     * Parts of \c rawData will be overwritten. There must be at least 4 additional
     * allocated bytes after the pixel data in \c rawData.
     *
     * If \c validBytes is set to a value smaller than the total transfer
     * size, only a partial transfer is performed. Subsequent calls to
     * setRawValidBytes() are then necessary.
     */
    void setRawTransferData(const ImageSet& metaData, const std::vector<unsigned char*>& imageData,
        int firstTileWidth = 0, int middleTilesWidth = 0, int lastTileWidth = 0);

    /**
     * \brief Updates the number of valid bytes in a partial raw transfer.
     *
     * \param validBytes The number of already valid bytes in the previously
     *        set raw data pointer.
     *
     * This method has to be called whenever new data is available in a raw
     * transfer. \see setRawTransferData()
     */
    void setRawValidBytes(const std::vector<int>& validBytes);

    /**
     * \brief Gets the next network message for the current transfer.
     *
     * \param length The length of the network message.
     * \return Pointer to the network message data.
     *
     * If the transfer has already been completed, a null pointer is returned.
     */
    const unsigned char* getTransferMessage(int& length);

    /**
     * \brief Returns true if the current transfer has been completed.
     */
    bool transferComplete();

    /**
     * \brief Aborts the transmission of the current transfer and performs a
     * reset of the internal state.
     */
    void resetTransfer();

    /**
     * \brief Returns a received image when complete.
     *
     * \param imageSet Will be set to the received image set.
     * \return Returns true if a new image set has been received. Otherwise
     *         false.
     *
     * After obtaining a received image set, reception is reset and
     * subsequent calls to getReceivedImageSet() or imagesReceived()
     * will return false.
     *
     * Please note that the received image data is only valid until processing
     * the first network message of the next image transfer.
     */
    bool getReceivedImageSet(ImageSet& imageSet);

    /**
     * \brief Returns a partially received image.
     *
     * \param imageSet Will be set to the partially received image set.
     * \param validRows The number of valid image rows in the partially received
                        image set.
     * \param complete True if the image set has been fully received.
     * \return Returns true if a full or partial image have been received.
     *         Otherwise false.
     *
     * If a partial image has been received, the meta data returned in
     * \c imageSet will be complete. The pixel data contained in
     * \c imageSet, however, will only be valid for the first
     * \c validRows rows.
     *
     * After obtaining a complete image set, reception is reset and
     * subsequent calls to getPartiallyReceivedImageSet() or imagesReceived()
     * will return false.
     *
     * Please note that the received image data is only valid until processing
     * the first network message of the next image transfer.
     */
    bool getPartiallyReceivedImageSet(ImageSet& imageSet, int& validRows, bool& complete);

    /**
     * \brief Returns true if the images of the current transfer have been received.
     */
    bool imagesReceived() const;

    /**
     * \brief Returns the buffer for receiving the next network message.
     *
     * \param maxLength Maximum allowed length for the next network message
     * \return Pointer to the buffer memory.
     **/
    unsigned char* getNextReceiveBuffer(int& maxLength);

    /**
     * \brief Handles a received network message
     *
     * \param length Length of the received network message.
     *
     * This method has to be called for every network message that has been
     * received. The message data must be located in the most recent buffer
     * that has been obtained with getNextReceiveBuffer().
     *
     * After calling this method, please check if a new image has been received
     * by calling getReceivedImageSet() or getPartiallyReceivedImageSet().
     *
     * In order to handle connection timeouts this method should be called
     * frequently. If no new data is available, a length of 0 can be passed.
     */
    void processReceivedMessage(int length);

    /**
     * \brief Returns the number of frames that have been dropped since
     * connecting to the current remote host.
     *
     * Dropped frames are caused by dropped packets due to a poor network
     * connection
     */
    int getNumDroppedFrames() const;

    /**
     * \brief Aborts the reception of the current image transfer and resets
     * the internal state.
     */
    void resetReception();

    /**
     * \brief Returns true if the last message has established a new connection
     * from a client
     */
    bool newClientConnected();

    /**
     * \brief Returns true if a remote connection is established
     */
    bool isConnected() const;

    /**
     * \brief If a control message is pending to be transmitted then
     * the message data will be returned by this method.
     *
     * \param length Will be set to the length of the message
     * \return Pointer to the message data or NULL if no message is pending
     *
     * Control messages are only needed when using the UDP network protocol.
     */
    const unsigned char* getNextControlMessage(int& length);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    /// Prints status information to the console
    std::string statusReport();
#endif

private:
    // We follow the pimpl idiom
    class Pimpl;
    Pimpl* pimpl;

    // This class cannot be copied
    ImageProtocol(const ImageProtocol& other);
    ImageProtocol& operator=(const ImageProtocol&);
};

} // namespace

#endif
