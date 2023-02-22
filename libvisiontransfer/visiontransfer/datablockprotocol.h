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

#ifndef VISIONTRANSFER_DATABLOCKPROTOCOL_H
#define VISIONTRANSFER_DATABLOCKPROTOCOL_H

#include <map>
#include <vector>
#include <memory>
#include <chrono>
#include <deque>

#include "visiontransfer/alignedallocator.h"
#include "visiontransfer/exceptions.h"

namespace visiontransfer {
namespace internal {

/**
 * \brief A protocol for transmitting large blocks of data over a network.
 *
 * The protocol slices the large data block into several smaller chunks
 * that can be transmitted over a network. A user defined header is
 * always transmitted before this large data block.
 *
 * There are two different implementations for UDP and TCP. In UDP mode,
 * packet loss is handled by performing a packet re-transmission. In TCP
 * mode, data does not have to be received with the same packet size as
 * it is sent out.
 *
 * This class is intended to be used by ImageProtocol and should normally
 * not be used directly.
 */

class DataBlockProtocol {
public:
    enum ProtocolType {
        PROTOCOL_TCP,
        PROTOCOL_UDP
    };

    //
    static const int MAX_DATA_BLOCKS = 8;

    // Constants that are also used in other places.
    static const int MAX_TCP_BYTES_TRANSFER = 0xFFFF; //64K - 1
    static const int MAX_UDP_RECEPTION = 0x4000; //16K
    static const int MAX_OUTSTANDING_BYTES = 2*MAX_TCP_BYTES_TRANSFER;

#pragma pack(push,1)
    // Extends previous one-channel 6-byte raw header buffer
    //  Legacy transfers can be detected via non-zero netTransferSizeDummy
    struct HeaderPreamble {
        uint16_t netHeaderSize;
        int32_t netTransferSizeDummy; // layout compatibility, legacy detection
        uint32_t netTransferSizes[MAX_DATA_BLOCKS]; // per-block total size
    };
    struct SegmentHeaderUDP {
        uint32_t segmentOffset;
    };
    struct SegmentHeaderTCP {
        uint32_t fragmentSize;
        uint32_t segmentOffset;
    };
#pragma pack(pop)

     /**
     * \brief Creates a new instance
     *
     * \param server If set to true, this object will be a communication server.
     * \param protType The network transport protocol that is used.
     * \param maxUdpPacketSize Maximum allowed size of a UDP packet when sending data.
     */

    DataBlockProtocol(bool server, ProtocolType protType, int maxUdpPacketSize);

    /**
     * \brief Returns the size of the overhead data that is required for
     * transferring a single network message.
     */
    int getProtocolOverhead() const {
        return protType == PROTOCOL_UDP ? sizeof(int) : 0;
    }

    /**
     * \brief Returns the maximum payload size that can be received
     */
    int getMaxReceptionSize() const;

    /**
     * \brief Resets all transfer related internal variables
     */
    void resetTransfer();

    /**
     * \brief Sets a user-defined header that shall be transmitted with
     * the next transfer
     *
     * \param data Pointer to the data of the header that should be
     *        transferred.
     * \param headerSize Size of the data in \c data.
     * \param transferSize Total size of the payload for the next transfer.
     *
     * This method must be called before setTransferData(). A call before
     * the start of each transfer is necessary. There must be at least
     * 6 additional bytes of reserved memory after the end and before the
     * beginning of \c data.
     */
    void setTransferHeader(unsigned char* data, int headerSize, int blocks);

    /**
     *  \brief Sets the per-block transfer size
     *
     *  \param bytes Size of the data pointed to with the matching setTransferData()
     *
     *  Replaces the old single-buffer total size that was prepended to the
     *  second-level header alongside the header size.
     */
    void setTransferBytes(int block, long bytes);

    /**
     * \brief Sets the payload data for the next transfer.
     *
     * \param data Pointer to the data that should be transferred.
     * \param validBytes The number of bytes that are currently
     *        valid in \c data.
     *
     * Part of \c data will be overwritten. There must be at least 4 additional
     * allocated bytes at the end of \c data.
     *
     * If \c validBytes is set to a value smaller than the total transfer
     * size, only a partial transfer is performed. Subsequent calls to
     * setTransferValidBytes() are then necessary.
    */
    void setTransferData(int block, unsigned char* data,  int validBytes = 0x7FFFFFFF);

    /**
     * \brief Updates the number of valid bytes in a partial transfer.
     *
     * \param validBytes The number of already valid bytes in the previously
     *        set data pointer.
     *
     * This method has to be called whenever new data is available in a
     * partial transfer. \see setTransferData()
     */
    void setTransferValidBytes(int block, int validBytes);

    /**
     * \brief Gets the next network message for the current transfer.
     *
     * \param length The length of the network message.
     * \return Pointer to the network message data.
     *
     * If the transfer has already been completed or if there are currently
     * no more valid bytes to be transmitted, a null pointer is returned.
     */
    const unsigned char* getTransferMessage(int& length);

    /**
     * \brief Returns true if the current transfer has been completed.
     */
    bool transferComplete();

    /**
     * \brief Gets a buffer for receiving the next network message.
     *
     * \param maxLength The expected maximum length that is required for
     *        receiving a network message.
     *
     * The returned buffer is a subsection of the internal receive buffer.
     */
    unsigned char* getNextReceiveBuffer(int maxLength);

    /**
     * \brief Resets the message reception.
     *
     * \param dropped If true, then this reset is rated as an error and
     *        internal counter for dropped transfers is increased.
     */
    void resetReception(bool dropped);

    /**
     * \brief Handles a received network message.
     *
     * \param length Length of the received network message.
     * \param transferComplete Set to true if a new transfer is complete after
     *        receiving the current packet
     *
     * Please see ImageProtocol::processReceivedMessage() for further details.
     */
    void processReceivedMessage(int length, bool& transferComplete);

    /**
     * \brief Returns the data that has been received for the current transfer.
     *
     * \param length Will be set to the number of bytes that have been received.
     * \return Pointer to the buffer containing the received data.
     *
     * The received data is valid until receiving the first network
     * message for a new transfer.
     */
    unsigned char* getReceivedData(int& length);

    /**
     * \brief Returns the header data that has been received for the
     * current transfer.
     *
     * \param length Will be set to the length of the header data in
     *        bytes.
     * \return Pointer to the buffer containing the received header data.
     *
     * The received header data is valid until receiving the first network
     * message for a new transfer.
     */
    unsigned char* getReceivedHeader(int& length);

    /**
     * \brief Returns the internal counter of dropped transfers during
     * reception.
     */
    int getDroppedReceptions() const {
        return droppedReceptions;
    }

    /**
     * \brief Returns true if the last network message has established a
     * new connection from a client
     *
     * For TCP this method always returns false as connections are
     * handled by the transport protocol.
     */
    bool newClientConnected();

    /**
     * \brief Returns true if a remote connection is established.
     *
      * For TCP this method always returns true as connections are
     * handled by the transport protocol.
     */
    bool isConnected() const;

    /**
     * \brief If a control message is pending to be transmitted, then
     * the message data will be returned by this method.
     *
     * \param length Will be set to the length of the message.
     * \return Pointer to the message data or NULL if no message is pending.
     *
     * Control messages are only used if the UDP transfer protocol is
     * selected. For TCP this method always returns a null pointer.
     */
    const unsigned char* getNextControlMessage(int& length);

    unsigned char* getBlockReceiveBuffer(int block) {
        if (block >= numReceptionBlocks) {
            throw ProtocolException("Tried to get receive buffer beyond initialized block range");
        }
        return &blockReceiveBuffers[block][0];
    }
    int getBlockValidSize(int block) {
        if (block >= numReceptionBlocks) {
            throw ProtocolException("Tried to get valid buffer index beyond initialized block range");
        }
        return blockValidSize[block];
    }
    bool isBlockDone(int block) {
        if (block >= numReceptionBlocks) {
            throw ProtocolException("Tried to get completion status of uninitialized block");
        }
        return blockValidSize[block] >= blockReceiveSize[block];
    }
    bool allBlocksDone() {
        for (int i=0; i<numReceptionBlocks; ++i) {
            if (!isBlockDone(i)) return false;
        }
        return true;
    }
    bool anyPayloadReceived() {
        for (int i=0; i<numReceptionBlocks; ++i) {
            if (blockReceiveOffsets[i] > 0) return true;
        }
        return false;
    }

    std::string statusReport();
    
    bool wasHeaderReceived() const {
        return headerReceived;
    }

private:
    // The pimpl idiom is not necessary here, as this class is usually not
    // used directly

    struct MissingReceiveSegment {
        int offset;
        int length;
        bool isEof;
        unsigned char subsequentData[4];
    };

    static constexpr int HEARTBEAT_INTERVAL_MS = 1000;
    static constexpr int RECONNECT_TIMEOUT_MS = 2000;

    static constexpr unsigned char CONNECTION_MESSAGE = 0x01;
    static constexpr unsigned char CONFIRM_MESSAGE = 0x02;
    static constexpr unsigned char HEADER_MESSAGE = 0x03;
    static constexpr unsigned char RESEND_MESSAGE = 0x04;
    static constexpr unsigned char EOF_MESSAGE = 0x05;
    static constexpr unsigned char HEARTBEAT_MESSAGE = 0x06;

    bool isServer;
    ProtocolType protType;
    int maxPayloadSize;
    int minPayloadSize;

    // Transfer related variables
    bool transferDone;
    unsigned char* rawDataArr[MAX_DATA_BLOCKS];
    int rawDataArrStrideHackOrig[MAX_DATA_BLOCKS];
    int rawDataArrStrideHackRepl[MAX_DATA_BLOCKS];
    int rawValidBytes[MAX_DATA_BLOCKS];
    int transferOffset[MAX_DATA_BLOCKS];
    int transferSize[MAX_DATA_BLOCKS];
    char overwrittenTransferData[sizeof(SegmentHeaderTCP)];
    int overwrittenTransferIndex;
    int overwrittenTransferBlock;
    unsigned char* transferHeaderData;
    int transferHeaderSize;
    int totalBytesCompleted;
    int totalTransferSize;
    int numTransferBlocks;
    int lastTransmittedBlock;

    // Reliability related variables
    std::deque<MissingReceiveSegment> missingReceiveSegments[MAX_DATA_BLOCKS];
    std::deque<std::pair<int, int> > missingTransferSegments;
    bool waitingForMissingSegments;
    int totalReceiveSize;

    unsigned char controlMessageBuffer[1024 * 16];

    // Connection related variables
    bool connectionConfirmed;
    bool confirmationMessagePending;
    bool eofMessagePending;
    bool clientConnectionPending;
    bool resendMessagePending;
    std::chrono::steady_clock::time_point lastRemoteHostActivity;
    std::chrono::steady_clock::time_point lastSentHeartbeat;
    std::chrono::steady_clock::time_point lastReceivedHeartbeat;

    // Reception related variables
    std::vector<unsigned char, AlignedAllocator<unsigned char> > receiveBuffer;
    std::vector<unsigned char, AlignedAllocator<unsigned char> > blockReceiveBuffers[MAX_DATA_BLOCKS];
    int blockReceiveOffsets[MAX_DATA_BLOCKS];
    int blockReceiveSize[MAX_DATA_BLOCKS];
    int blockValidSize[MAX_DATA_BLOCKS];
    std::vector<unsigned char> receivedHeader;
    bool finishedReception;
    int droppedReceptions;
    int completedReceptions;
    double lostSegmentRate;
    int lostSegmentBytes;
    unsigned char unprocessedMsgPart[MAX_OUTSTANDING_BYTES];
    int unprocessedMsgLength;
    bool headerReceived;
    bool legacyTransfer;
    int numReceptionBlocks;
    int receiveOffset;

    const unsigned char* extractPayload(const unsigned char* data, int& length, bool& error);
    bool processControlMessage(int length);
    void restoreTransferBuffer();
    bool generateResendRequest(int& length);
    void getNextTransferSegment(int& block, int& offset, int& length);
    void parseResendMessage(int length);
    void parseEofMessage(int length);
    void integrateMissingUdpSegments(int block, int lastSegmentOffset, int lastSegmentSize);
    void processReceivedUdpMessage(int length, bool& transferComplete);
    void processReceivedTcpMessage(int length, bool& transferComplete);
    void resizeReceiveBuffer();
    int parseReceivedHeader(int length, int offset);
    void zeroStructures();
    void splitRawOffset(int rawSegmentOffset, int& dataBlockID, int& segmentOffset);
    int mergeRawOffset(int dataBlockID, int segmentOffset, int reserved=0);

};

}} // namespace

#endif
