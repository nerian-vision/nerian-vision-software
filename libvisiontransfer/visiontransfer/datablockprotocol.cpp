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

#include <algorithm>
#include <iostream>
#include <cstring>

#include <iomanip>
#include <sstream>

#include "visiontransfer/datablockprotocol.h"
#include "visiontransfer/exceptions.h"

// Network headers
#ifdef _WIN32
#include <winsock2.h>
#undef min
#undef max
#else
#include <arpa/inet.h>
#endif

#define LOG_DEBUG_DBP(expr)
//#define LOG_DEBUG_DBP(expr) std::cerr << "DataBlockProtocol: " << expr << std::endl

using namespace std;
using namespace visiontransfer;
using namespace visiontransfer::internal;

namespace visiontransfer {
namespace internal {

DataBlockProtocol::DataBlockProtocol(bool server, ProtocolType protType, int maxUdpPacketSize)
        : isServer(server), protType(protType),
        transferDone(true),
        overwrittenTransferData{0},
        overwrittenTransferIndex{-1},
        overwrittenTransferBlock{-1},
        transferHeaderData{nullptr},
        transferHeaderSize{0},
        totalBytesCompleted{0}, totalTransferSize{0},
        waitingForMissingSegments(false),
        totalReceiveSize(0), connectionConfirmed(false),
        confirmationMessagePending(false), eofMessagePending(false),
        clientConnectionPending(false), resendMessagePending(false),
        lastRemoteHostActivity(), lastSentHeartbeat(),
        lastReceivedHeartbeat(std::chrono::steady_clock::now()),
        finishedReception(false), droppedReceptions(0),
        completedReceptions(0), lostSegmentRate(0.0), lostSegmentBytes(0),
        unprocessedMsgLength(0), headerReceived(false) {
    // Determine the maximum allowed payload size
    if(protType == PROTOCOL_TCP) {
        maxPayloadSize = MAX_TCP_BYTES_TRANSFER - sizeof(SegmentHeaderTCP);
        minPayloadSize = 0;
    } else {
        maxPayloadSize = maxUdpPacketSize - sizeof(SegmentHeaderUDP);
        minPayloadSize = maxPayloadSize;
    }
    zeroStructures();
    resizeReceiveBuffer();
    resetReception(false);
}
void DataBlockProtocol::splitRawOffset(int rawSegmentOffset, int& dataBlockID, int& segmentOffset) {
    int selector = (rawSegmentOffset >> 28) & 0xf;
    dataBlockID = selector & 0x7; // Note: 0x8 bit is reserved for now
    segmentOffset = rawSegmentOffset & 0x0FFFffff;
}

int DataBlockProtocol::mergeRawOffset(int dataBlockID, int segmentOffset, int reserved_defaults0) {
    return ((reserved_defaults0 & 1) << 31) | ((dataBlockID & 0x07) << 28) | (segmentOffset & 0x0FFFffff);
}

void DataBlockProtocol::zeroStructures() {
    for (int i=0; i<MAX_DATA_BLOCKS; ++i) {
        rawDataArr[i] = nullptr;
        rawDataArrStrideHackOrig[i] = 0;
        rawDataArrStrideHackRepl[i] = 0;
        rawValidBytes[i] = 0;
        transferOffset[i] = 0;
        transferSize[i] = 0;
    }
    std::memset(overwrittenTransferData, 0, sizeof(overwrittenTransferData));
    overwrittenTransferIndex = -1;
    overwrittenTransferBlock = -1;
    lastTransmittedBlock = -1;
    receiveOffset = 0;
    numReceptionBlocks = 0;
}

void DataBlockProtocol::resetTransfer() {
    transferDone = true;
    overwrittenTransferIndex = -1;
    overwrittenTransferBlock = -1;
    totalBytesCompleted = 0;
    totalTransferSize = 0;
    numTransferBlocks = 0;
    missingTransferSegments.clear();
}

void DataBlockProtocol::setTransferBytes(int block, long bytes) {
    if (transferHeaderData == nullptr) {
        throw ProtocolException("Tried to set data block size before initializing header!");
    } else if (block >= numTransferBlocks) {
        throw ProtocolException("Request to set data block size - block index too high!");
    }
    transferSize[block] = bytes;
    HeaderPreamble* hp = reinterpret_cast<HeaderPreamble*>(transferHeaderData);
    hp->netTransferSizes[block] = htonl(bytes);
}

void DataBlockProtocol::setTransferHeader(unsigned char* data, int headerSize, int blocks) {
    if(!transferDone && numTransferBlocks > 0) {
        throw ProtocolException("Header data set while transfer is active!");
    } else if(headerSize + 9 > static_cast<int>(sizeof(controlMessageBuffer))) {
        throw ProtocolException("Transfer header is too large!");
    } else if(blocks == 0) {
        throw ProtocolException("Requested transfer of 0 blocks!");
    }

    numTransferBlocks = blocks;

    transferDone = false;
    for (int i=0; i<MAX_DATA_BLOCKS; ++i) {
        this->transferSize[i] = 0; // must be set via setRawTransferBytes()
    }

    int headerBaseOffset = sizeof(HeaderPreamble);

    transferHeaderData = &data[-headerBaseOffset];
    HeaderPreamble* ourHeader = reinterpret_cast<HeaderPreamble*>(transferHeaderData);

    unsigned short netHeaderSize = htons(static_cast<unsigned short>(headerSize));
    ourHeader->netHeaderSize = netHeaderSize;
    ourHeader->netTransferSizeDummy = htonl(-1); // clashes on purpose with old recipients

    headerSize += headerBaseOffset;

    if(protType == PROTOCOL_UDP) {
        // In UDP mode we still need to make this a control message
        transferHeaderData[headerSize++] = HEADER_MESSAGE;
        transferHeaderData[headerSize++] = 0xFF;
        transferHeaderData[headerSize++] = 0xFF;
        transferHeaderData[headerSize++] = 0xFF;
        transferHeaderData[headerSize++] = 0xFF;
    }

    transferHeaderSize = headerSize;
}

void DataBlockProtocol::setTransferData(int block, unsigned char* data, int validBytes) {
    if(transferHeaderSize == 0 || transferHeaderData == nullptr) {
        throw ProtocolException("The transfer header has not yet been set!");
    }

    transferDone = false;
    rawDataArr[block] = data;
    transferOffset[block] = 0;
    overwrittenTransferIndex = -1;
    overwrittenTransferBlock = -1;
    rawValidBytes[block] = min(transferSize[block], validBytes);
    totalBytesCompleted = 0;
}

void DataBlockProtocol::setTransferValidBytes(int block, int validBytes) {
    if(validBytes >= transferSize[block]) {
        rawValidBytes[block] = transferSize[block];
    } else if(validBytes < static_cast<int>(sizeof(int))) {
        rawValidBytes[block] = 0;
    } else {
        rawValidBytes[block] = validBytes;
    }
}

std::string DataBlockProtocol::statusReport() {
    std::stringstream ss;
    ss << "DataBlockProtocol, blocks=" << numTransferBlocks << ": ";
    for (int i=0; i<numTransferBlocks; ++i) {
        ss << i << ":(len " << transferSize[i] << " ofs " << transferOffset[i] << " rawvalid " << rawValidBytes[i] << ")  ";
    }
    ss << "  total done: " << totalBytesCompleted << "/" << totalTransferSize;
    return ss.str();
}

const unsigned char* DataBlockProtocol::getTransferMessage(int& length) {
    if(transferDone || rawValidBytes == 0) {
        // No more data to be transferred
        length = 0;
        return nullptr;
    }

    // For TCP we always send the header first
    if(protType == PROTOCOL_TCP && transferHeaderData != nullptr) {
        length = transferHeaderSize;
        const unsigned char* ret = transferHeaderData;
        transferHeaderData = nullptr;
        return ret;
    }

    // The transfer buffer might have been altered by the previous transfer
    // and first needs to be restored
    restoreTransferBuffer();

    // Determine which data segment to transfer next
    int block = -1, offset = -1;
    getNextTransferSegment(block, offset, length);
    if(length == 0) {
        return nullptr;
    }

    if(protType == PROTOCOL_UDP) {
        // For udp, we always append a segment offset
        overwrittenTransferBlock = block;
        overwrittenTransferIndex = offset + length;
        SegmentHeaderUDP* segmentHeader = reinterpret_cast<SegmentHeaderUDP*>(&rawDataArr[block][offset + length]);
        std::memcpy(overwrittenTransferData, segmentHeader, sizeof(SegmentHeaderUDP));
        segmentHeader->segmentOffset = static_cast<int>(htonl(mergeRawOffset(block, offset)));
        length += sizeof(SegmentHeaderUDP);
        lastTransmittedBlock = block;
        return &rawDataArr[block][offset];
    } else {
        // For tcp, we *PRE*pend the segment header consisting of segment offset plus the packet payload size
        int headerOffset = offset - sizeof(SegmentHeaderTCP);

        SegmentHeaderTCP* segmentHeader = nullptr;
        unsigned char* dataPointer = nullptr;

        if(headerOffset < 0) {
            // For the first TCP transfer we need to copy the data as we cannot
            // prepend before the data start
            static unsigned char tcpBuffer[MAX_TCP_BYTES_TRANSFER];
            dataPointer = tcpBuffer;
            segmentHeader = reinterpret_cast<SegmentHeaderTCP*>(tcpBuffer);
            std::memcpy(&tcpBuffer[sizeof(segmentHeader)], &rawDataArr[block][offset], length);
        } else {
            // For subsequent calls we will overwrite the segment header data and
            // restore it
            dataPointer = &rawDataArr[block][headerOffset];
            segmentHeader = reinterpret_cast<SegmentHeaderTCP*>(&rawDataArr[block][headerOffset]);
            overwrittenTransferBlock = block;
            overwrittenTransferIndex = headerOffset;
            std::memcpy(overwrittenTransferData, segmentHeader, sizeof(SegmentHeaderTCP));
        }

        segmentHeader->fragmentSize = htonl(length);
        segmentHeader->segmentOffset = static_cast<int>(htonl(mergeRawOffset(block, offset)));
        length += sizeof(SegmentHeaderTCP);
        lastTransmittedBlock = block;
        return dataPointer;
    }
}

void DataBlockProtocol::getNextTransferSegment(int& block, int& offset, int& length) {
    if(missingTransferSegments.size() == 0) {
        // Select from block with the most unsent data
        int sendBlock = 0, amount = 0;
        for (int i=0; i<numTransferBlocks; ++i) {
            int avail = std::min(transferSize[i],  rawValidBytes[i]);
            avail -= transferOffset[i];
            if (avail > amount) {
                amount = avail;
                sendBlock = i;
            }
        }
        length = std::min(maxPayloadSize, amount);
        if(length == 0 || (length < minPayloadSize && rawValidBytes[sendBlock] != transferSize[sendBlock])) {
            length = 0;
            return;
        }

        block = sendBlock;
        offset = transferOffset[sendBlock];
        transferOffset[sendBlock] += length; // for next transfer
        if (protType == PROTOCOL_UDP) {
            bool complete = true;
            for (int i=0; i<numTransferBlocks; ++i) {
                if (transferOffset[i] < transferSize[i]) {
                    complete = false;
                    break;
                }
            }
            if (complete) {
                eofMessagePending = true;
            }
        }
    } else {
        // This is a segment that is re-transmitted due to packet loss
        splitRawOffset(missingTransferSegments.front().first, block, offset);
        length = std::min(maxPayloadSize, missingTransferSegments.front().second);
        LOG_DEBUG_DBP("Re-transmitting: " << offset << " -  " << (offset + length));

        int remaining = missingTransferSegments[0].second - length;
        if(remaining == 0) {
            // The segment is competed
            missingTransferSegments.pop_front();
        } else {
            // The segment is only partially complete
            missingTransferSegments.front().first += length;
            missingTransferSegments.front().second = remaining;
        }
    }
}

void DataBlockProtocol::restoreTransferBuffer() {
    if(overwrittenTransferBlock >= 0) {
        if(protType == PROTOCOL_UDP) {
            std::memcpy(&rawDataArr[overwrittenTransferBlock][overwrittenTransferIndex], overwrittenTransferData, sizeof(SegmentHeaderUDP));
        } else {
            std::memcpy(&rawDataArr[overwrittenTransferBlock][overwrittenTransferIndex], overwrittenTransferData, sizeof(SegmentHeaderTCP));
        }
    }
    overwrittenTransferIndex = -1;
    overwrittenTransferBlock = -1;
}

bool DataBlockProtocol::transferComplete() {
    for (int i=0; i<numTransferBlocks; ++i) {
        if (transferOffset[i] < transferSize[i]) return false;
    }
    return !eofMessagePending;
}

int DataBlockProtocol::getMaxReceptionSize() const {
    if(protType == PROTOCOL_TCP) {
        return MAX_TCP_BYTES_TRANSFER;
    } else  {
        return MAX_UDP_RECEPTION;
    }
}

unsigned char* DataBlockProtocol::getNextReceiveBuffer(int maxLength) {
    if(receiveOffset + maxLength > (int)receiveBuffer.size()) {
        receiveBuffer.resize(receiveOffset + maxLength);
    }
    return &receiveBuffer[receiveOffset];
}

void DataBlockProtocol::processReceivedMessage(int length, bool& transferComplete) {
    transferComplete = false;
    if(length <= 0) {
        return; // Nothing received
    }

    if(finishedReception) {
        // First reset for next frame
        resetReception(false);
    }

    if(protType == PROTOCOL_UDP) {
        processReceivedUdpMessage(length, transferComplete);
    } else {
        processReceivedTcpMessage(length, transferComplete);
    }

    transferComplete = finishedReception;
}

void DataBlockProtocol::processReceivedUdpMessage(int length, bool& transferComplete) {
    if(length < static_cast<int>(sizeof(int)) ||
            0 + length > static_cast<int>(receiveBuffer.size())) {
        throw ProtocolException("Received message size is invalid!");
    }

    // Extract the sequence number
    int rawSegmentOffset = ntohl(*reinterpret_cast<int*>(
        &receiveBuffer[0 + length - sizeof(int)]));
    // for holding the offset with blanked-out channel index
    int dataBlockID, segmentOffset;
    splitRawOffset(rawSegmentOffset, dataBlockID, segmentOffset);

    if(rawSegmentOffset == static_cast<int>(0xFFFFFFFF)) {
        // This is a control packet
        processControlMessage(length);
    } else if(headerReceived) {
        // Correct the length by subtracting the size of the segment offset
        int realPayloadOffset = 0;
        int payloadLength = length - sizeof(int);

        if(segmentOffset != blockReceiveOffsets[dataBlockID]) {
            // The segment offset doesn't match what we expected. Probably
            // a packet was dropped
            if(!waitingForMissingSegments && //receiveOffset > 0 &&
                    segmentOffset > blockReceiveOffsets[dataBlockID]
                    && segmentOffset + payloadLength <= (int)blockReceiveBuffers[dataBlockID].size()) {
                // We can just ask for a retransmission of this packet
                LOG_DEBUG_DBP("Missing segment: " << dataBlockID << " size " << payloadLength << " ofs " << segmentOffset
                    << " but blkRecvOfs " << blockReceiveOffsets[dataBlockID]
                    << " (# " << missingReceiveSegments[dataBlockID].size() << ")");

                MissingReceiveSegment missingSeg;
                missingSeg.offset = mergeRawOffset(dataBlockID, blockReceiveOffsets[dataBlockID]);
                missingSeg.length = segmentOffset - blockReceiveOffsets[dataBlockID];
                missingSeg.isEof = false;
                lostSegmentBytes += missingSeg.length;
                missingReceiveSegments[dataBlockID].push_back(missingSeg);

                // Move the received data to the right place in the buffer
                memcpy(&blockReceiveBuffers[dataBlockID][segmentOffset], &receiveBuffer[0 + realPayloadOffset], payloadLength);
                // Advance block receive offset
                blockReceiveOffsets[dataBlockID] = segmentOffset + payloadLength;
            } else {
                // In this case we cannot recover from the packet loss or
                // we just didn't get the EOF packet and everything is
                // actually fine
                resetReception(blockReceiveOffsets[0] > 0);
                if(segmentOffset > 0 ) {
                    if(blockReceiveOffsets[dataBlockID] > 0) {
                        LOG_DEBUG_DBP("Resend failed!");
                    }
                    return;
                } else {
                    LOG_DEBUG_DBP("Missed EOF message!");
                }
            }
        } else {
            if ((realPayloadOffset+payloadLength) > (int)receiveBuffer.size()) {
                throw ProtocolException("Received out-of-bound data.");
            }

            // append to correct block buffer
            memcpy(&blockReceiveBuffers[dataBlockID][segmentOffset], &receiveBuffer[0 + realPayloadOffset], payloadLength);
            // advance the expected next data offset for this block
            blockReceiveOffsets[dataBlockID] = segmentOffset + payloadLength;
            if (waitingForMissingSegments) {
                // segment extends the currently valid region (suspended once we missed out first segment)
                if ((missingReceiveSegments[dataBlockID].size() == 1) && (missingReceiveSegments[dataBlockID].front().length <= payloadLength)) {
                    // last gap closed by this segment
                    blockValidSize[dataBlockID] = blockReceiveSize[dataBlockID];
                } else {
                    blockValidSize[dataBlockID] = segmentOffset + payloadLength;
                }
            } else if (missingReceiveSegments[dataBlockID].size() == 0) {
                blockValidSize[dataBlockID] = segmentOffset + payloadLength;
            }
        }

        if(segmentOffset == 0 && dataBlockID == 0) {
            // This is the beginning of a new frame
            lastRemoteHostActivity = std::chrono::steady_clock::now();
        }

        // Try to fill missing regions
        integrateMissingUdpSegments(dataBlockID, segmentOffset, payloadLength);
    }
}

void DataBlockProtocol::integrateMissingUdpSegments(int block, int lastSegmentOffset, int lastSegmentSize) {
    if(waitingForMissingSegments && missingReceiveSegments[block].size() > 0) {
        // Things get more complicated when re-transmitting dropped packets
        int checkBlock, checkOffset;
        MissingReceiveSegment& firstSeg = missingReceiveSegments[block].front();
        splitRawOffset(firstSeg.offset, checkBlock, checkOffset);
        if((lastSegmentOffset != checkOffset) || (block != checkBlock)) {
            LOG_DEBUG_DBP("Received invalid resend: " << block << " " << lastSegmentOffset);
            resetReception(true);
        } else {
            firstSeg.offset += lastSegmentSize;
            firstSeg.length -= lastSegmentSize;
            if(firstSeg.length == 0) {
                missingReceiveSegments[block].pop_front();
            }

            // Check if ALL missing blocks are now handled
            bool done = true;
            for (int blk=0; blk<numReceptionBlocks; ++blk) {
                if(missingReceiveSegments[blk].size() > 0) {
                    done = false;
                    break;
                }
            }
            if (done) {
                waitingForMissingSegments = false;
                finishedReception = true;
            } else if (missingReceiveSegments[block].size() > 0) {
                // Another lost segment
                int newBlock, newOffset;
                MissingReceiveSegment& firstSeg = missingReceiveSegments[block].front();
                splitRawOffset(firstSeg.offset, newBlock, newOffset);
                blockReceiveOffsets[block] = newOffset;
            }
        }
    }
}

void DataBlockProtocol::processReceivedTcpMessage(int length, bool& transferComplete) {
    // In TCP mode the header must be the first data item to be transmitted
    if(!headerReceived) {
        int totalHeaderSize = parseReceivedHeader(length, 0);
        if(totalHeaderSize == 0) {
            // Not yet enough data. Keep on buffering.
            receiveOffset += length; // append in next recv
            return;
        } else {
            // Header successfully parsed
            // Move the remaining data to the beginning of the buffer
            length -= totalHeaderSize;
            // The rest is the first [part of] buffer segment data

            if(length == 0) {
                return; // No more data remaining
            }

            int movelength = receiveOffset + length; // also move the old stuff
            ::memmove(&receiveBuffer[0], &receiveBuffer[totalHeaderSize], movelength);
            receiveOffset = movelength; // append in next recv
        }
    } else {
        receiveOffset += length; // modified below if complete chunks are present
    }

    if (legacyTransfer) {
        // Legacy TCP transfer: no segment headers, just raw data for block 0, up to the expected size
        int remainingSize = blockReceiveSize[0] - blockValidSize[0];
        int availableSize = std::min(receiveOffset, remainingSize);
        // Update actual target buffer
        std::memcpy(&blockReceiveBuffers[0][blockReceiveOffsets[0]], &receiveBuffer[0], availableSize);
        blockReceiveOffsets[0] += availableSize;
        blockValidSize[0] = blockReceiveOffsets[0];
        // Extra data, store at buffer start for next reception to append to
        if (receiveOffset <= remainingSize) {
            // Start next reception at recv buffer start
            receiveOffset = 0;
        } else {
            // Mark next reception to append to unhandled data remainder
            std::memmove(&receiveBuffer[0], &receiveBuffer[remainingSize], availableSize - remainingSize);
            receiveOffset = availableSize - remainingSize;
        }
    } else {
        // Parse the SegmentHeaderTCP (if present) to see if a full fragment is present
        int ofs = 0;
        while ((receiveOffset - ofs) >= (int) sizeof(SegmentHeaderTCP)) {
            SegmentHeaderTCP* header = reinterpret_cast<SegmentHeaderTCP*>(&receiveBuffer[ofs]);
            int fragsize = ntohl(header->fragmentSize);
            int rawSegmentOffset = ntohl(header->segmentOffset);
            int block, offset;
            splitRawOffset(rawSegmentOffset, block, offset);
            if (block == 7) { // Block 7 is reserved; control message (the next header), stop moving image data
                break;
            }
            if ((receiveOffset - ofs) >= (fragsize + (int) sizeof(SegmentHeaderTCP))) {
                // Incorporate fragment
                // assert here that offset==blockReceiveOffsets[block]
                if (offset != blockReceiveOffsets[block]) {
                    throw ProtocolException("Received invalid header!");
                }
                std::memcpy(&blockReceiveBuffers[block][blockReceiveOffsets[block]], &receiveBuffer[ofs+sizeof(SegmentHeaderTCP)], fragsize);
                blockReceiveOffsets[block] += fragsize;
                blockValidSize[block] = blockReceiveOffsets[block];
                // Advance to next potential chunk
                ofs += fragsize + sizeof(SegmentHeaderTCP);
            } else {
                // Fragment incomplete, will be appended to in next recv (offset increased above)
                break;
            }
        }
        if (ofs > 0) {
            // Move start of next unaccounted-for fragment to start of buffer
            std::memmove(&receiveBuffer[0], &receiveBuffer[ofs], receiveOffset - ofs);
            receiveOffset -= ofs; // and shift append position accordingly
        }
    }

    // Determine whether all buffers are filled now
    bool complete = true;
    for (int i=0; i<numReceptionBlocks; ++i) {
        if (blockReceiveOffsets[i] < blockReceiveSize[i]) {
            complete = false;
            break;
        }
    }
    finishedReception = complete;

}

int DataBlockProtocol::parseReceivedHeader(int length, int offset) {
    int headerExtraBytes = 6; // see below

    if(length < headerExtraBytes) {
        return 0;
    }

    unsigned short headerSize = ntohs(*reinterpret_cast<unsigned short*>(&receiveBuffer[offset]));
    if (length < (headerExtraBytes + headerSize)) {
        return 0;
    }
    totalReceiveSize = static_cast<int>(ntohl(*reinterpret_cast<unsigned int*>(&receiveBuffer[offset + 2])));

    if (totalReceiveSize >= 0) { // old-style single block transfer
        legacyTransfer = true;
        headerExtraBytes = 6;
        numReceptionBlocks = 1; // ONE interleaved buffer
        blockReceiveSize[0] = totalReceiveSize;
    } else { // marked -1 for new-style multi block transfer
        legacyTransfer = false;
        headerExtraBytes = static_cast<int>(sizeof(HeaderPreamble));
        HeaderPreamble* header = reinterpret_cast<HeaderPreamble*>(&receiveBuffer[offset]);
        numReceptionBlocks = 0;
        totalReceiveSize = 0;
        for (int i=0; i<MAX_DATA_BLOCKS; ++i) {
            int s = ntohl(header->netTransferSizes[i]);
            if (s > 0) {
                blockReceiveSize[i] = s;
                numReceptionBlocks++;
                totalReceiveSize += s;
            } else {
                // first non-positive payload size signals end of blocks
                //break;
            }
        }
    }

    if (numReceptionBlocks==0) throw std::runtime_error("Received a transfer with zero blocks");
    if (numReceptionBlocks > MAX_DATA_BLOCKS) throw std::runtime_error("Received a transfer with too many blocks");

    if(headerSize + headerExtraBytes > static_cast<int>(receiveBuffer.size())
            || totalReceiveSize < 0 || headerSize + headerExtraBytes > length ) {
        throw ProtocolException("Received invalid header!");
    }

    headerReceived = true;
    receivedHeader.assign(receiveBuffer.begin() + offset + headerExtraBytes,
        receiveBuffer.begin() + offset + headerSize + headerExtraBytes);
    resizeReceiveBuffer();

    return headerSize + headerExtraBytes;
}

void DataBlockProtocol::resetReception(bool dropped) {
    numReceptionBlocks = 0;
    headerReceived = false;
    for (int blk = 0; blk<MAX_DATA_BLOCKS; ++blk) {
        missingReceiveSegments[blk].clear();
    }
    receivedHeader.clear();
    waitingForMissingSegments = false;
    totalReceiveSize = 0;
    finishedReception = false;
    lostSegmentBytes = 0;
    for (int i=0; i<MAX_DATA_BLOCKS; ++i) {
        blockReceiveOffsets[i] = 0;
        blockValidSize[i] = 0;
    }
    if(dropped) {
        droppedReceptions++;
    }
}

unsigned char* DataBlockProtocol::getReceivedData(int& length) {
    length = 0;
    return &receiveBuffer[0];
}

unsigned char* DataBlockProtocol::getReceivedHeader(int& length) {
    if(receivedHeader.size() > 0) {
        length = static_cast<int>(receivedHeader.size());
        return &receivedHeader[0];
    } else {
        return nullptr;
    }
}

bool DataBlockProtocol::processControlMessage(int length) {
    if(length < static_cast<int>(sizeof(int) + 1)) {
        return false;
    }

    int payloadLength = length - sizeof(int) - 1;
    switch(receiveBuffer[0 + payloadLength]) {
        case CONFIRM_MESSAGE:
            // Our connection request has been accepted
            connectionConfirmed = true;
            break;
        case CONNECTION_MESSAGE:
            // We establish a new connection
            connectionConfirmed = true;
            confirmationMessagePending = true;
            clientConnectionPending = true;

            // A connection request is just as good as a heartbeat
            lastReceivedHeartbeat = std::chrono::steady_clock::now();
            break;
        case HEADER_MESSAGE: {
                if (anyPayloadReceived()) {
                    if (allBlocksDone()) {
                        LOG_DEBUG_DBP("No EOF message received!");
                    } else {
                        LOG_DEBUG_DBP("Received header too late/early!");
                    }
                    resetReception(true);
                }
                if(parseReceivedHeader(payloadLength, 0) == 0) {
                    throw ProtocolException("Received header is too short!");
                }
            }
            break;
        case EOF_MESSAGE:
            // This is the end of the frame
            if(anyPayloadReceived()) {
                parseEofMessage(length);
            }
            break;
        case RESEND_MESSAGE: {
            // The client requested retransmission of missing packets
            parseResendMessage(payloadLength);
            break;
        }
        case HEARTBEAT_MESSAGE:
            // A cyclic heartbeat message
            lastReceivedHeartbeat = std::chrono::steady_clock::now();
            break;
        default:
            throw ProtocolException("Received invalid control message!");
            break;
    }

    return true;
}

bool DataBlockProtocol::isConnected() const {
    if(protType == PROTOCOL_TCP) {
        // Connection is handled by TCP and not by us
        return true;
    } else if(connectionConfirmed) {
        return !isServer || std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - lastReceivedHeartbeat).count()
        < 2*HEARTBEAT_INTERVAL_MS;
    } else return false;
}

const unsigned char* DataBlockProtocol::getNextControlMessage(int& length) {
    length = 0;

    if(protType == PROTOCOL_TCP) {
        // There are no control messages for TCP
        return nullptr;
    }

    if(confirmationMessagePending) {
        // Send confirmation message
        confirmationMessagePending = false;
        controlMessageBuffer[0] = CONFIRM_MESSAGE;
        length = 1;
    } else if(!isServer && std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - lastRemoteHostActivity).count() > RECONNECT_TIMEOUT_MS) {
        // Send a new connection request
        controlMessageBuffer[0] = CONNECTION_MESSAGE;
        length = 1;

        // Also update time stamps
        lastRemoteHostActivity = lastSentHeartbeat = std::chrono::steady_clock::now();
    } else if(transferHeaderData != nullptr && isConnected()) {
        // We need to send a new protocol header
        length = transferHeaderSize;
        const unsigned char* ret = transferHeaderData;
        transferHeaderData = nullptr;
        return ret;
    } else if(eofMessagePending) {
        // Send end of frame message
        eofMessagePending = false;
        unsigned int networkOffset = htonl(mergeRawOffset(lastTransmittedBlock, transferSize[lastTransmittedBlock]));
        memcpy(&controlMessageBuffer[0], &networkOffset, sizeof(int));
        controlMessageBuffer[sizeof(int)] = EOF_MESSAGE;
        length = 5;
    } else if(resendMessagePending) {
        // Send a re-send request for missing messages
        resendMessagePending = false;
        if(!generateResendRequest(length)) {
            length = 0;
            return nullptr;
        }
    } else if(!isServer && std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - lastSentHeartbeat).count() > HEARTBEAT_INTERVAL_MS) {
        // Send a heartbeat message
        controlMessageBuffer[0] = HEARTBEAT_MESSAGE;
        length = 1;
        lastSentHeartbeat = std::chrono::steady_clock::now();
    } else {
        return nullptr;
    }

    // Mark this message as a control message
    controlMessageBuffer[length++] = 0xff;
    controlMessageBuffer[length++] = 0xff;
    controlMessageBuffer[length++] = 0xff;
    controlMessageBuffer[length++] = 0xff;
    return controlMessageBuffer;
}

bool DataBlockProtocol::newClientConnected() {
    if(clientConnectionPending) {
        clientConnectionPending = false;
        return true;
    } else {
        return false;
    }
}

bool DataBlockProtocol::generateResendRequest(int& length) {
    length = 0;
    for (int blk = 0; blk < numReceptionBlocks; ++blk) {
        for(MissingReceiveSegment segment: missingReceiveSegments[blk]) {
            unsigned int segOffset = htonl(static_cast<unsigned int>(segment.offset));
            unsigned int segLen = htonl(static_cast<unsigned int>(segment.length));

            if (sizeof(controlMessageBuffer) < length + 2*sizeof(unsigned int) + 4) {
                // Too many UDP resend segments for control buffer, dropping the frame!
                resetReception(true);
                break;
            }

            memcpy(&controlMessageBuffer[length], &segOffset, sizeof(segOffset));
            length += sizeof(unsigned int);
            memcpy(&controlMessageBuffer[length], &segLen, sizeof(segLen));
            length += sizeof(unsigned int);

            int dbgBlk, dbgOfs;
            splitRawOffset(segment.offset, dbgBlk, dbgOfs);
            LOG_DEBUG_DBP("Req missing " << dbgBlk << " " << dbgOfs << " " << segment.length);
        }
    }

    if(length + sizeof(int) + 1 > sizeof(controlMessageBuffer)) {
        return false;
    }

    controlMessageBuffer[length++] = RESEND_MESSAGE;

    return true;
}

void DataBlockProtocol::parseResendMessage(int length) {
    missingTransferSegments.clear();

    int num = length / (sizeof(unsigned int) + sizeof(unsigned int));
    int bufferOffset = 0;

    for(int i=0; i<num; i++) {
        unsigned int segOffsetNet = *reinterpret_cast<unsigned int*>(&receiveBuffer[bufferOffset]);
        bufferOffset += sizeof(unsigned int);
        unsigned int segLenNet = *reinterpret_cast<unsigned int*>(&receiveBuffer[bufferOffset]);
        bufferOffset += sizeof(unsigned int);

        int segmentOffsetRaw = static_cast<int>(ntohl(segOffsetNet)); // with block ID
        int segmentLength = static_cast<int>(ntohl(segLenNet));
        int dataBlockID, segmentOffset;
        splitRawOffset(segmentOffsetRaw, dataBlockID, segmentOffset);

        if(segmentOffset >= 0 && segmentLength > 0 && (segmentOffset + segmentLength) <= rawValidBytes[dataBlockID]) {
            missingTransferSegments.push_back(std::pair<int, int>(
                segmentOffsetRaw, segmentLength));
        }

    }
}

void DataBlockProtocol::parseEofMessage(int length) {

    completedReceptions++;
    lostSegmentRate = (lostSegmentRate * (completedReceptions-1) + ((double) lostSegmentBytes) / totalReceiveSize) / completedReceptions;
    LOG_DEBUG_DBP("Lost segment rate: " << lostSegmentRate);
    if(length >= 4) {
        // Find all missing segments at the end of blocks
        for (int i=0; i<numReceptionBlocks; ++i) {
            if (blockReceiveOffsets[i] < blockReceiveSize[i]) {
                MissingReceiveSegment missingSeg;
                missingSeg.offset = mergeRawOffset(i, blockReceiveOffsets[i]);
                missingSeg.length = blockReceiveSize[i] - blockReceiveOffsets[i];
                missingSeg.isEof = true;
                missingReceiveSegments[i].push_back(missingSeg);
                lostSegmentBytes += missingSeg.length;
            }
        }
        for (int blk=0; blk<numReceptionBlocks; ++blk) {
            if(missingReceiveSegments[blk].size() > 0) {
                waitingForMissingSegments = true;
                resendMessagePending = true;
                // Initialize all missing block start indices with earliest missing address
                int mblock, moffset;
                for (int i=0; i<static_cast<int>(missingReceiveSegments[blk].size()); ++i) {
                    splitRawOffset(missingReceiveSegments[blk][i].offset, mblock, moffset);
                    if (moffset < blockReceiveOffsets[mblock]) {
                        blockReceiveOffsets[mblock] = moffset;
                    }
                }
            }
        }
        if (!resendMessagePending) {
            finishedReception = true;
        }
    } else {
        LOG_DEBUG_DBP("EOF message too short, length " << length);
    }
}

void DataBlockProtocol::resizeReceiveBuffer() {
    if(totalReceiveSize < 0) {
        throw ProtocolException("Received invalid transfer size!");
    }

    // We increase the requested size to allow for one
    // additional network message and the protocol overhead
    int bufferSize = 2*getMaxReceptionSize()
        + MAX_OUTSTANDING_BYTES + sizeof(int);

    // Resize the buffer
    if(static_cast<int>(receiveBuffer.size()) < bufferSize) {
        receiveBuffer.resize(bufferSize);
    }

    for (int i=0; i<numReceptionBlocks; ++i) {
        if (static_cast<int>(blockReceiveBuffers[i].size()) < blockReceiveSize[i]) {
            blockReceiveBuffers[i].resize(blockReceiveSize[i]);
        }
    }
}

}} // namespace

