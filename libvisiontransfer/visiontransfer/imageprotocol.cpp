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

#include <cstring>
#include <iostream>
#include <limits>
#include <vector>
#include <memory>
#include <algorithm>
#include "visiontransfer/imageprotocol.h"
#include "visiontransfer/alignedallocator.h"
#include "visiontransfer/datablockprotocol.h"
#include "visiontransfer/exceptions.h"
#include "visiontransfer/bitconversions.h"
#include "visiontransfer/internalinformation.h"

// Network headers
#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <winsock2.h>
#else
    #include <arpa/inet.h>
#endif

#define LOG_DEBUG_IMPROTO(expr)
//#define LOG_DEBUG_IMPROTO(expr) std::cerr << "ImageProtocol: " << expr << std::endl

using namespace std;
using namespace visiontransfer;
using namespace visiontransfer::internal;

namespace visiontransfer {

/*************** Pimpl class containing all private members ***********/

class ImageProtocol::Pimpl {
public:

    static const int IMAGE_HEADER_OFFSET = sizeof(DataBlockProtocol::HeaderPreamble) + 10;

    Pimpl(bool server, ProtocolType protType, int maxUdpPacketSize);

    // Redeclaration of public members
    void setTransferImageSet(const ImageSet& imageSet);
    void setRawTransferData(const ImageSet& metaData, const std::vector<unsigned char*>& rawData,
        int firstTileWidth = 0, int middleTilesWidth = 0, int lastTileWidth = 0);
    void setRawValidBytes(const std::vector<int>& validBytesVec);
    const unsigned char* getTransferMessage(int& length);
    bool transferComplete();
    void resetTransfer();
    bool getReceivedImageSet(ImageSet& imageSet);
    bool getPartiallyReceivedImageSet(ImageSet& imageSet,
        int& validRows, bool& complete);
    bool imagesReceived() const;

    unsigned char* getNextReceiveBuffer(int& maxLength);

    void processReceivedMessage(int length);
    int getProspectiveMessageSize();
    int getNumDroppedFrames() const;
    void resetReception();
    bool isConnected() const;
    const unsigned char* getNextControlMessage(int& length);
    bool newClientConnected();

    std::string statusReport();

private:
    unsigned short MAGIC_SEQUECE = 0x3D15;

    // Header data transferred in the first packet
#pragma pack(push,1)
    struct HeaderDataLegacy {
        unsigned short magic;

        unsigned char protocolVersion;
        unsigned char isRawImagePair_OBSOLETE;

        unsigned short width;
        unsigned short height;

        unsigned short firstTileWidth;
        unsigned short lastTileWidth;

        unsigned char format0;
        unsigned char format1;
        unsigned short minDisparity;
        unsigned short maxDisparity;
        unsigned char subpixelFactor;

        unsigned int seqNum;
        int timeSec;
        int timeMicrosec;

        float q[16];

        unsigned short middleTilesWidth;
    };
    // Header data v2: extensible and forwards-compatible
    struct HeaderDataV2: public HeaderDataLegacy {
        unsigned short totalHeaderSize;
        unsigned short flags;
        unsigned char numberOfImages;
        unsigned char format2;
        enum FlagBits {
            NEW_STYLE_TRANSFER = 1,
            HEADER_V3 = 2,
            HEADER_V4 = 4,
            HEADER_V5 = 8,
            // future protocol extensions should mark a new bit here
        };
    };
    // Header data v3, adds arbitrary image channel assignments
    struct HeaderDataV3: public HeaderDataV2 {
        // HEADER_V3 bit implies that this extension is present,
        //  declaring arbitrary channel roles for each of numberOfImages active channels.
        //  If not present, is it an old sender that always sends two images
        //  (channel 0: left, channel 1: right or disparity (if active))
        unsigned char imageTypes[8];
    };
    // Header data v4, adds exposure time and sync pulse
    struct HeaderDataV4: public HeaderDataV3 {
        int exposureTime; // exposure time in microseconds
        int lastSyncPulseSec;
        int lastSyncPulseMicrosec;
    };
    // Header data v5, adds format for 4th image
    struct HeaderData: public HeaderDataV4{
        unsigned char format3;
    };
#pragma pack(pop)

    // Underlying protocol for data transfers
    DataBlockProtocol dataProt;
    ProtocolType protType;

    // Transfer related variables
    std::vector<unsigned char> headerBuffer;

    // Reception related variables
    std::vector<unsigned char, AlignedAllocator<unsigned char> >decodeBuffer[ImageSet::MAX_SUPPORTED_IMAGES];
    bool receiveHeaderParsed;
    HeaderData receiveHeader;
    int lastReceivedPayloadBytes[ImageSet::MAX_SUPPORTED_IMAGES];
    bool receptionDone;

    // Copies the transmission header to the given buffer
    void copyHeaderToBuffer(const ImageSet& imageSet, int firstTileWidth,
        int middleTilesWidth, int lastTileWidth, unsigned char* buffer);

    // Decodes header information from the received data
    void tryDecodeHeader(const unsigned char* receivedData, int receivedBytes);

    // Decodes a received image from a non-interleaved buffer
    unsigned char* decodeNoninterleaved(int imageNumber, int numImages, int receivedBytes,
        unsigned char* data, int& validRows, int& rowStride);

    // Decodes a received image from an interleaved buffer
    unsigned char* decodeInterleaved(int imageNumber, int numImages, int receivedBytes,
        unsigned char* data, int& validRows, int& rowStride);

    int getNumTiles(int width, int firstTileWidth, int middleTilesWidth, int lastTileWidth);

    int getFrameSize(int width, int height, int firstTileWidth, int middleTilesWidth,
        int lastTileWidth, int totalBits);

    int getFormatBits(ImageSet::ImageFormat format, bool afterDecode);

    void decodeTiledImage(int imageNumber, int lastReceivedPayloadBytes, int receivedPayloadBytes,
        const unsigned char* data, int firstTileStride, int middleTilesStride, int lastTileStride,
        int& validRows, ImageSet::ImageFormat format, bool dataIsInterleaved);

    void decodeRowsFromTile(int startRow, int stopRow, unsigned const char* src,
        unsigned char* dst, int srcStride, int dstStride, int tileWidth);

    void allocateDecodeBuffer(int imageNumber);
};


/******************** Stubs for all public members ********************/

ImageProtocol::ImageProtocol(bool server, ProtocolType protType, int maxUdpPacketSize)
    : pimpl(new Pimpl(server, protType, maxUdpPacketSize)) {
    // All initializations are done by the Pimpl class
}

ImageProtocol::~ImageProtocol() {
    delete pimpl;
}

void ImageProtocol::setTransferImageSet(const ImageSet& imageSet) {
    pimpl->setTransferImageSet(imageSet);
}

void ImageProtocol::setRawTransferData(const ImageSet& metaData, const std::vector<unsigned char*>& imageData,
        int firstTileWidth, int middleTilesWidth, int lastTileWidth) {
    pimpl->setRawTransferData(metaData, imageData, firstTileWidth, middleTilesWidth, lastTileWidth);
}

void ImageProtocol::setRawValidBytes(const std::vector<int>& validBytesVec) {
    pimpl->setRawValidBytes(validBytesVec);
}

const unsigned char* ImageProtocol::getTransferMessage(int& length) {
    return pimpl->getTransferMessage(length);
}

bool ImageProtocol::transferComplete() {
    return pimpl->transferComplete();
}

void ImageProtocol::resetTransfer() {
    pimpl->resetTransfer();
}

bool ImageProtocol::getReceivedImageSet(ImageSet& imageSet) {
    return pimpl->getReceivedImageSet(imageSet);
}

bool ImageProtocol::getPartiallyReceivedImageSet(
        ImageSet& imageSet, int& validRows, bool& complete) {
    return pimpl->getPartiallyReceivedImageSet(imageSet, validRows, complete);
}

bool ImageProtocol::imagesReceived() const {
    return pimpl->imagesReceived();
}

unsigned char* ImageProtocol::getNextReceiveBuffer(int& maxLength) {
    return pimpl->getNextReceiveBuffer(maxLength);
}

void ImageProtocol::processReceivedMessage(int length) {
    pimpl->processReceivedMessage(length);
}

int ImageProtocol::getNumDroppedFrames() const {
    return pimpl->getNumDroppedFrames();
}

void ImageProtocol::resetReception() {
    pimpl->resetReception();
}

bool ImageProtocol::isConnected() const {
    return pimpl->isConnected();
}

const unsigned char* ImageProtocol::getNextControlMessage(int& length) {
    return pimpl->getNextControlMessage(length);
}

bool ImageProtocol::newClientConnected() {
    return pimpl->newClientConnected();
}

/******************** Implementation in pimpl class *******************/

ImageProtocol::Pimpl::Pimpl(bool server, ProtocolType protType, int maxUdpPacketSize)
        :dataProt(server, (DataBlockProtocol::ProtocolType)protType,
        maxUdpPacketSize), protType(protType),
        receiveHeaderParsed(false), lastReceivedPayloadBytes{0},
        receptionDone(false) {
    headerBuffer.resize(sizeof(HeaderData) + 128);
    memset(&headerBuffer[0], 0, sizeof(headerBuffer.size()));
    memset(&receiveHeader, 0, sizeof(receiveHeader));
}

void ImageProtocol::Pimpl::setTransferImageSet(const ImageSet& imageSet) {
    for (int i=0; i<imageSet.getNumberOfImages(); ++i) {
        if(imageSet.getPixelData(i) == nullptr) {
            throw ProtocolException("Image data is null pointer!");
        }
    }

    // Set header as first piece of data
    copyHeaderToBuffer(imageSet, 0, 0, 0, &headerBuffer[IMAGE_HEADER_OFFSET]);
    dataProt.resetTransfer();
    int numTransferBlocks = imageSet.getNumberOfImages();
    dataProt.setTransferHeader(&headerBuffer[IMAGE_HEADER_OFFSET], sizeof(HeaderData), numTransferBlocks);
    for (int i=0; i<imageSet.getNumberOfImages(); ++i) {
        int bits = getFormatBits(imageSet.getPixelFormat(i), false);
        int rawDataLength = getFrameSize(imageSet.getWidth(), imageSet.getHeight(), 0, 0, 0, bits);
        dataProt.setTransferBytes(i, rawDataLength);
    }

    // Perform 12 bit packed encoding if necessary
    int bits[ImageSet::MAX_SUPPORTED_IMAGES] = {0};
    int rowSize[ImageSet::MAX_SUPPORTED_IMAGES] = {0};
    const unsigned char* pixelData[ImageSet::MAX_SUPPORTED_IMAGES] = {nullptr};

    for(int i = 0; i<imageSet.getNumberOfImages(); i++) {
        bits[i] = getFormatBits(imageSet.getPixelFormat(i), false);
        rowSize[i] = imageSet.getWidth()*bits[i]/8;

        if(imageSet.getPixelFormat(i) != ImageSet::FORMAT_12_BIT_MONO) {
            pixelData[i] = imageSet.getPixelData(i);
        } else {
            static std::vector<unsigned char> encodingBuffer[ImageSet::MAX_SUPPORTED_IMAGES];
            encodingBuffer[i].resize(rowSize[i] * imageSet.getHeight());
            BitConversions::encode12BitPacked(0, imageSet.getHeight(), imageSet.getPixelData(i),
                &encodingBuffer[i][0], imageSet.getRowStride(i), rowSize[i], imageSet.getWidth());
            pixelData[i] = &encodingBuffer[i][0];
        }
    }

    for (int i=0; i<imageSet.getNumberOfImages(); ++i) {
        dataProt.setTransferData(i, const_cast<unsigned char*>(pixelData[i])); // these are always reserved memory or untile buffers
    }
}

void ImageProtocol::Pimpl::setRawTransferData(const ImageSet& metaData, const std::vector<unsigned char*>& rawData,
        int firstTileWidth, int middleTilesWidth, int lastTileWidth) {
    if(static_cast<int>(rawData.size()) != metaData.getNumberOfImages()) {
        throw ProtocolException("Mismatch between metadata and number of image buffers!");
    }

    // Set header as first piece of data
    copyHeaderToBuffer(metaData, firstTileWidth, middleTilesWidth, lastTileWidth, &headerBuffer[IMAGE_HEADER_OFFSET]);
    dataProt.resetTransfer();
    int numTransferBlocks = metaData.getNumberOfImages();
    dataProt.setTransferHeader(&headerBuffer[IMAGE_HEADER_OFFSET], sizeof(HeaderData), numTransferBlocks);
    // Now set the size per channel (replaces old final size argument to setTransferHeader()
    for (int i=0; i<metaData.getNumberOfImages(); ++i) {
        int rawDataLength = getFrameSize(metaData.getWidth(), metaData.getHeight(),
          firstTileWidth, middleTilesWidth, lastTileWidth, metaData.getBitsPerPixel(i));
        dataProt.setTransferBytes(i, rawDataLength);
    }

    for (int i=0; i<metaData.getNumberOfImages(); ++i) {
        dataProt.setTransferData(i, rawData[i]);
    }
}

void ImageProtocol::Pimpl::setRawValidBytes(const std::vector<int>& validBytesVec) {
    for (int i=0; i<static_cast<int>(validBytesVec.size()); ++i) {
        dataProt.setTransferValidBytes(i, validBytesVec[i]);
    }
}

const unsigned char* ImageProtocol::Pimpl::getTransferMessage(int& length) {
    const unsigned char* msg = dataProt.getTransferMessage(length);

    if(msg == nullptr) {
        msg = dataProt.getTransferMessage(length);
    }

    return msg;
}

bool ImageProtocol::Pimpl::transferComplete() {
    return dataProt.transferComplete();
}

int ImageProtocol::Pimpl::getNumTiles(int width, int firstTileWidth, int middleTilesWidth, int lastTileWidth) {
    if(lastTileWidth == 0) {
        return 1;
    } else if(middleTilesWidth == 0) {
        return 2;
    } else {
        int tileWidth = firstTileWidth + lastTileWidth - middleTilesWidth;
        return (width - 2*tileWidth + firstTileWidth + lastTileWidth) / (firstTileWidth + lastTileWidth - tileWidth);
    }
}

int ImageProtocol::Pimpl::getFrameSize(int width, int height, int firstTileWidth,
        int middleTilesWidth, int lastTileWidth, int totalBits) {
    return (width * height * totalBits) /8;
}

int ImageProtocol::Pimpl::getFormatBits(ImageSet::ImageFormat format, bool afterDecode) {
    if(afterDecode) {
        return ImageSet::getBytesPerPixel(format)*8;
    } else {
        switch(format) {
            case ImageSet::FORMAT_8_BIT_MONO: return 8;
            case ImageSet::FORMAT_12_BIT_MONO: return 12;
            case ImageSet::FORMAT_8_BIT_RGB: return 24;
            default: throw ProtocolException("Illegal pixel format!");
        }
    }
}

void ImageProtocol::Pimpl::copyHeaderToBuffer(const ImageSet& imageSet,
        int firstTileWidth, int middleTilesWidth, int lastTileWidth, unsigned char* buffer) {
    int timeSec = 0, timeMicrosec = 0;
    HeaderData* transferHeader = reinterpret_cast<HeaderData*>(buffer);

    memset(transferHeader, 0, sizeof(*transferHeader));
    transferHeader->magic = htons(MAGIC_SEQUECE);
    transferHeader->protocolVersion = InternalInformation::CURRENT_PROTOCOL_VERSION;
    transferHeader->isRawImagePair_OBSOLETE = 0;
    transferHeader->width = htons(imageSet.getWidth());
    transferHeader->height = htons(imageSet.getHeight());
    transferHeader->firstTileWidth = htons(firstTileWidth);
    transferHeader->lastTileWidth = htons(lastTileWidth);
    transferHeader->middleTilesWidth = htons(middleTilesWidth);
    transferHeader->format0 = static_cast<unsigned char>(imageSet.getPixelFormat(0));
    transferHeader->format1 = (imageSet.getNumberOfImages() <= 1) ? 0 : static_cast<unsigned char>(imageSet.getPixelFormat(1));
    transferHeader->seqNum = static_cast<unsigned int>(htonl(imageSet.getSequenceNumber()));
    transferHeader->format2 = (imageSet.getNumberOfImages() <= 2) ? 0 : static_cast<unsigned char>(imageSet.getPixelFormat(2));
    transferHeader->format3 = (imageSet.getNumberOfImages() <= 3) ? 0 : static_cast<unsigned char>(imageSet.getPixelFormat(3));
    transferHeader->numberOfImages = static_cast<unsigned char>(imageSet.getNumberOfImages());
    transferHeader->exposureTime = htonl(imageSet.getExposureTime());

    imageSet.getLastSyncPulse(timeSec, timeMicrosec);
    transferHeader->lastSyncPulseSec = htonl(timeSec);
    transferHeader->lastSyncPulseMicrosec = htonl(timeMicrosec);

    transferHeader->totalHeaderSize = htons(sizeof(HeaderData));
    transferHeader->flags = htons(HeaderData::FlagBits::NEW_STYLE_TRANSFER | HeaderData::FlagBits::HEADER_V3
        | HeaderData::FlagBits::HEADER_V4 | HeaderData::FlagBits::HEADER_V5);

    int minDisp = 0, maxDisp = 0;
    imageSet.getDisparityRange(minDisp, maxDisp);
    transferHeader->minDisparity = minDisp;
    transferHeader->maxDisparity = maxDisp;

    transferHeader->subpixelFactor = imageSet.getSubpixelFactor();

    imageSet.getTimestamp(timeSec, timeMicrosec);
    transferHeader->timeSec = static_cast<int>(htonl(static_cast<unsigned int>(timeSec)));
    transferHeader->timeMicrosec = static_cast<int>(htonl(static_cast<unsigned int>(timeMicrosec)));

    int numImageChannels = 0;
    for (int i=0; i<(int) sizeof(transferHeader->imageTypes); ++i) {
        transferHeader->imageTypes[i] = static_cast<unsigned char>(ImageSet::ImageType::IMAGE_UNDEFINED);
    }
    int idx = imageSet.getIndexOf(ImageSet::ImageType::IMAGE_LEFT);
    if (idx>=0) {
        transferHeader->imageTypes[idx] = static_cast<unsigned char>(ImageSet::ImageType::IMAGE_LEFT);
        numImageChannels++;
    }
    idx = imageSet.getIndexOf(ImageSet::ImageType::IMAGE_RIGHT);
    if (idx>=0) {
        transferHeader->imageTypes[idx] = static_cast<unsigned char>(ImageSet::ImageType::IMAGE_RIGHT);
        numImageChannels++;
    }
    idx = imageSet.getIndexOf(ImageSet::ImageType::IMAGE_DISPARITY);
    if (idx>=0) {
        transferHeader->imageTypes[idx] = static_cast<unsigned char>(ImageSet::ImageType::IMAGE_DISPARITY);
        numImageChannels++;
    }
    idx = imageSet.getIndexOf(ImageSet::ImageType::IMAGE_COLOR);
    if (idx>=0) {
        transferHeader->imageTypes[idx] = static_cast<unsigned char>(ImageSet::ImageType::IMAGE_COLOR);
        numImageChannels++;
    }
    if (numImageChannels != imageSet.getNumberOfImages()) {
        throw std::runtime_error("Mismatch between reported number of images and enabled channel selection!");
    }


    if(imageSet.getQMatrix() != nullptr) {
        memcpy(transferHeader->q, imageSet.getQMatrix(), sizeof(float)*16);
    }
}

void ImageProtocol::Pimpl::resetTransfer() {
    dataProt.resetTransfer();
}

unsigned char* ImageProtocol::Pimpl::getNextReceiveBuffer(int& maxLength) {
    maxLength = dataProt.getMaxReceptionSize();
    return dataProt.getNextReceiveBuffer(maxLength);
}

void ImageProtocol::Pimpl::processReceivedMessage(int length) {
    receptionDone = false;

    // Add the received message
    dataProt.processReceivedMessage(length, receptionDone);
    if(!dataProt.wasHeaderReceived() && receiveHeaderParsed) {
        // Something went wrong. We need to reset!
        LOG_DEBUG_IMPROTO("Resetting image protocol!");
        resetReception();
        return;
    }

    int receivedBytes = 0;
    dataProt.getReceivedData(receivedBytes);

    // Immediately try to decode the header
    if(!receiveHeaderParsed) {
        int headerLen = 0;
        unsigned char* headerData = dataProt.getReceivedHeader(headerLen);
        if(headerData != nullptr) {
            tryDecodeHeader(headerData, headerLen);
        }
    }
}

void ImageProtocol::Pimpl::tryDecodeHeader(const
unsigned char* receivedData, int receivedBytes) {
    // Extra data fields that have been added to the header. Must be
    // removed when the protocol version number is updated
    constexpr int optionalDataSize = sizeof(receiveHeader.middleTilesWidth);
    constexpr int mandatoryDataSize = static_cast<int>(sizeof(HeaderDataLegacy)) - optionalDataSize;
    constexpr int fullyExtensibleHeaderSize = static_cast<int>(sizeof(HeaderDataV2));
    bool isCompleteHeader = false;

    if(receivedBytes >= mandatoryDataSize) {
        if (receivedBytes < fullyExtensibleHeaderSize) {
            *(static_cast<HeaderDataLegacy*>(&receiveHeader)) =  *reinterpret_cast<const HeaderDataLegacy*>(receivedData);
        } else {
            memcpy(&receiveHeader, receivedData, std::min((size_t)receivedBytes, sizeof(HeaderData)));
            receiveHeader = *reinterpret_cast<const HeaderData*>(receivedData);
            isCompleteHeader = true;
        }
        if(receiveHeader.magic != htons(MAGIC_SEQUECE)) {
            // Let's not call this an error. Perhaps it's just not a header
            // packet
            return;
        }

        if(receiveHeader.protocolVersion != InternalInformation::CURRENT_PROTOCOL_VERSION) {
            throw ProtocolException("Protocol version mismatch!");
        }

        // Convert byte order
        receiveHeader.width = ntohs(receiveHeader.width);
        receiveHeader.height = ntohs(receiveHeader.height);
        receiveHeader.firstTileWidth = ntohs(receiveHeader.firstTileWidth);
        receiveHeader.lastTileWidth = ntohs(receiveHeader.lastTileWidth);

        receiveHeader.timeSec = static_cast<int>(
            ntohl(static_cast<unsigned int>(receiveHeader.timeSec)));
        receiveHeader.timeMicrosec = static_cast<int>(
            ntohl(static_cast<unsigned int>(receiveHeader.timeMicrosec)));
        receiveHeader.seqNum = ntohl(receiveHeader.seqNum);

        // Optional data items
        if(receivedBytes >= mandatoryDataSize + optionalDataSize) {
            receiveHeader.middleTilesWidth = ntohs(receiveHeader.middleTilesWidth);
        } else {
            receiveHeader.middleTilesWidth = 0;
        }
        if (isCompleteHeader) {
            // This is a header of v2 or above, which self-reports its extension level in the flags field
            receiveHeader.totalHeaderSize = ntohs(receiveHeader.totalHeaderSize);
            receiveHeader.flags = ntohs(receiveHeader.flags);
            receiveHeader.exposureTime = ntohl(receiveHeader.exposureTime);
            receiveHeader.lastSyncPulseSec = htonl(receiveHeader.lastSyncPulseSec);
            receiveHeader.lastSyncPulseMicrosec = htonl(receiveHeader.lastSyncPulseMicrosec);
        } else {
            // Infer missing fields for legacy compatibility transfers
            receiveHeader.totalHeaderSize = (receivedBytes <= mandatoryDataSize) ? mandatoryDataSize : static_cast<int>(sizeof(HeaderDataLegacy));
            receiveHeader.flags = 0;
            receiveHeader.numberOfImages = 2;
            receiveHeader.format2 = 0;
            receiveHeader.format3 = 0;
            receiveHeader.exposureTime = 0;
            receiveHeader.lastSyncPulseSec = 0;
            receiveHeader.lastSyncPulseMicrosec = 0;
        }

        receiveHeaderParsed = true;
    }
}

bool ImageProtocol::Pimpl::imagesReceived() const {
    return receptionDone && receiveHeaderParsed;
}

bool ImageProtocol::Pimpl::getReceivedImageSet(ImageSet& imageSet) {
    bool complete = false;
    int validRows;
    bool ok = getPartiallyReceivedImageSet(imageSet, validRows, complete);

    return (ok && complete);
}

bool ImageProtocol::Pimpl::getPartiallyReceivedImageSet(ImageSet& imageSet, int& validRows, bool& complete) {
    imageSet.setWidth(0);
    imageSet.setHeight(0);

    complete = false;

    if(!receiveHeaderParsed) {
        // We haven't even received the image header yet
        return false;
    } else {
        // We received at least some pixel data
        imageSet.setNumberOfImages(receiveHeader.numberOfImages);
        bool flaggedDisparityPair = (receiveHeader.isRawImagePair_OBSOLETE == 0); // only meaningful in headers <=V2
        bool isInterleaved = (receiveHeader.flags & HeaderData::FlagBits::NEW_STYLE_TRANSFER) == 0;
        bool arbitraryChannels = (receiveHeader.flags & HeaderData::FlagBits::HEADER_V3) > 0;
        bool hasExposureTime = (receiveHeader.flags & HeaderData::FlagBits::HEADER_V4) > 0;

        // Forward compatibility check: mask out all known flag bits and see what remains
        unsigned short unaccountedFlags = receiveHeader.flags & ~(HeaderData::FlagBits::NEW_STYLE_TRANSFER
            | HeaderData::FlagBits::HEADER_V3 | HeaderData::FlagBits::HEADER_V4 | HeaderData::FlagBits::HEADER_V5);
        if (unaccountedFlags != 0) {
            // Newer protocol (unknown flag present) - we will try to continue
            //   since connection has not been refused earlier
            static bool warnedOnceForward = false;
            if (!warnedOnceForward) {
                LOG_DEBUG_IMPROTO("Warning: forward-compatible mode; will attempt to process image stream with unknown extra flags. Consider upgrading the client software.");
                warnedOnceForward = true;
            }
        }

        imageSet.setWidth(receiveHeader.width);
        imageSet.setHeight(receiveHeader.height);

        imageSet.setPixelFormat(0, static_cast<ImageSet::ImageFormat>(receiveHeader.format0));
        if (imageSet.getNumberOfImages() > 1) imageSet.setPixelFormat(1, static_cast<ImageSet::ImageFormat>(receiveHeader.format1));
        if (imageSet.getNumberOfImages() > 2) imageSet.setPixelFormat(2, static_cast<ImageSet::ImageFormat>(receiveHeader.format2));
        if (imageSet.getNumberOfImages() > 3) imageSet.setPixelFormat(3, static_cast<ImageSet::ImageFormat>(receiveHeader.format3));

        int rowStrideArr[ImageSet::MAX_SUPPORTED_IMAGES] = {0};
        int validRowsArr[ImageSet::MAX_SUPPORTED_IMAGES] = {0};
        unsigned char* pixelArr[ImageSet::MAX_SUPPORTED_IMAGES] = {nullptr};

        if (isInterleaved) {
            // OLD transfer (forced to interleaved 2 images mode)
            static bool warnedOnceBackward = false;
            if (!warnedOnceBackward) {
                LOG_DEBUG_IMPROTO("Info: backward-compatible mode; the device is sending with a legacy protocol. Consider upgrading its firmware.");
                warnedOnceBackward = true;
            }
            unsigned char* data = dataProt.getBlockReceiveBuffer(0);
            int validBytes = dataProt.getBlockValidSize(0);
            for (int i=0; i < 2; ++i) {
                pixelArr[i] = decodeInterleaved(i, imageSet.getNumberOfImages(), validBytes, data, validRowsArr[i], rowStrideArr[i]);
            }
            // Legacy sender with mode-dependent channel selection
            imageSet.setIndexOf(ImageSet::ImageType::IMAGE_LEFT, 0);
            imageSet.setIndexOf(ImageSet::ImageType::IMAGE_RIGHT, flaggedDisparityPair ? -1 : 1);
            imageSet.setIndexOf(ImageSet::ImageType::IMAGE_DISPARITY, flaggedDisparityPair ? 1 : -1);
            imageSet.setIndexOf(ImageSet::ImageType::IMAGE_COLOR, -1);
        } else {
            // NEW transfer
            try {
                for (int i=0; i<receiveHeader.numberOfImages; ++i) {
                    unsigned char* data = dataProt.getBlockReceiveBuffer(i);
                    int validBytes = dataProt.getBlockValidSize(i);
                    pixelArr[i] = decodeNoninterleaved(i, imageSet.getNumberOfImages(), validBytes, data, validRowsArr[i], rowStrideArr[i]);
                }
            } catch(const ProtocolException& ex) {
                LOG_DEBUG_IMPROTO("Protocol exception: " << ex.what());
                resetReception();
                return false;
            }
            if (arbitraryChannels) {
                // Completely customizable channel selection
                imageSet.setIndexOf(ImageSet::ImageType::IMAGE_LEFT, -1);
                imageSet.setIndexOf(ImageSet::ImageType::IMAGE_RIGHT, -1);
                imageSet.setIndexOf(ImageSet::ImageType::IMAGE_DISPARITY, -1);
                imageSet.setIndexOf(ImageSet::ImageType::IMAGE_COLOR, -1);
                for (int i=0; i<imageSet.getNumberOfImages(); ++i) {
                    int typ = receiveHeader.imageTypes[i];
                    ImageSet::ImageType imgtype = static_cast<ImageSet::ImageType>(typ);
                    imageSet.setIndexOf(imgtype, i);
                }
            } else {
                static bool warnedOnceV2 = false;
                if (!warnedOnceV2) {
                    LOG_DEBUG_IMPROTO("Info: received a transfer with header v2");
                    warnedOnceV2 = true;
                }
                // Older v2 header; accessing imageTypes is not valid
                //  Two-image sender with mode-dependent channel selection
                imageSet.setIndexOf(ImageSet::ImageType::IMAGE_LEFT, 0);
                imageSet.setIndexOf(ImageSet::ImageType::IMAGE_RIGHT, flaggedDisparityPair ? -1 : 1);
                imageSet.setIndexOf(ImageSet::ImageType::IMAGE_DISPARITY, flaggedDisparityPair ? 1 : -1);
                imageSet.setIndexOf(ImageSet::ImageType::IMAGE_COLOR, -1);
            }
            if(hasExposureTime) {
                imageSet.setExposureTime(receiveHeader.exposureTime);
                imageSet.setLastSyncPulse(receiveHeader.lastSyncPulseSec, receiveHeader.lastSyncPulseMicrosec);
            }
        }

        for (int i=0; i<receiveHeader.numberOfImages; ++i) {
            imageSet.setRowStride(i, rowStrideArr[i]);
            imageSet.setPixelData(i, pixelArr[i]);
        }
        imageSet.setQMatrix(receiveHeader.q);

        imageSet.setSequenceNumber(receiveHeader.seqNum);
        imageSet.setTimestamp(receiveHeader.timeSec, receiveHeader.timeMicrosec);
        imageSet.setDisparityRange(receiveHeader.minDisparity, receiveHeader.maxDisparity);
        imageSet.setSubpixelFactor(receiveHeader.subpixelFactor);

        validRows = validRowsArr[0];
        for (int i=0; i<receiveHeader.numberOfImages; ++i) {
            if (validRowsArr[i] < validRows) {
                validRows = validRowsArr[i];
            }
        }

        if(validRows == receiveHeader.height || receptionDone) {
            complete = true;
            resetReception();
        }

        return true;
    }
}

unsigned char* ImageProtocol::Pimpl::decodeNoninterleaved(int imageNumber, int numImages, int receivedBytes,
        unsigned char* data, int& validRows, int& rowStride) {
    ImageSet::ImageFormat format;
    int bits = 8;
    switch (imageNumber) {
        case 0:
            format = static_cast<ImageSet::ImageFormat>(receiveHeader.format0);
            break;
        case 1:
            format = static_cast<ImageSet::ImageFormat>(receiveHeader.format1);
            break;
        case 2:
            format = static_cast<ImageSet::ImageFormat>(receiveHeader.format2);
            break;
        case 3:
            format = static_cast<ImageSet::ImageFormat>(receiveHeader.format3);
            break;
        default:
            throw ProtocolException("Not implemented: decodeNoninterleaved with image index > 2");
    }
    bits = getFormatBits(static_cast<ImageSet::ImageFormat>(format), false);

    int totalBits = bits;
    unsigned char* ret = nullptr;

    if(receiveHeader.lastTileWidth == 0) {
        int bufferOffset0 = 0;
        int bufferRowStride = receiveHeader.width*(totalBits) / 8;

        if(format == ImageSet::FORMAT_8_BIT_MONO || format == ImageSet::FORMAT_8_BIT_RGB) {
            // No decoding is necessary. We can just pass through the
            // data pointer
            ret = &data[bufferOffset0];
            rowStride = bufferRowStride;
            validRows = std::min(receivedBytes / bufferRowStride, (int)receiveHeader.height);
        } else {
            // Perform 12-bit => 16 bit decoding
            allocateDecodeBuffer(imageNumber);
            validRows = std::min(receivedBytes / bufferRowStride, (int)receiveHeader.height);
            rowStride = 2*receiveHeader.width;
            int lastRow = std::min(lastReceivedPayloadBytes[imageNumber] / bufferRowStride, validRows);

            BitConversions::decode12BitPacked(lastRow, validRows, &data[bufferOffset0],
                &decodeBuffer[imageNumber][0], bufferRowStride, rowStride, receiveHeader.width);

            ret = &decodeBuffer[imageNumber][0];
        }
    } else {
        // Decode the tiled transfer
        decodeTiledImage(imageNumber,
            lastReceivedPayloadBytes[imageNumber], receivedBytes, data,
            receiveHeader.firstTileWidth * (totalBits) / 8,
            receiveHeader.middleTilesWidth * (totalBits) / 8,
            receiveHeader.lastTileWidth * (totalBits) / 8,
            validRows, format, false);
        ret = &decodeBuffer[imageNumber][0];
        rowStride = receiveHeader.width*getFormatBits(
            static_cast<ImageSet::ImageFormat>(format), true)/8;
    }

    lastReceivedPayloadBytes[imageNumber] = receivedBytes;
    return ret;
}


unsigned char* ImageProtocol::Pimpl::decodeInterleaved(int imageNumber, int numImages, int receivedBytes,
        unsigned char* data, int& validRows, int& rowStride) {
    ImageSet::ImageFormat format = static_cast<ImageSet::ImageFormat>(
        imageNumber == 0 ? receiveHeader.format0 : receiveHeader.format1);
    int bits0 = getFormatBits(static_cast<ImageSet::ImageFormat>(receiveHeader.format0), false);
    int bits1 = getFormatBits(static_cast<ImageSet::ImageFormat>(receiveHeader.format1), false);
    int bits2 = getFormatBits(static_cast<ImageSet::ImageFormat>(receiveHeader.format2), false);
    int bits3 = getFormatBits(static_cast<ImageSet::ImageFormat>(receiveHeader.format3), false);

    int totalBits = (numImages<3)?(bits0 + bits1):(bits0 + bits1 + bits2 + bits3);

    unsigned char* ret = nullptr;

    if(receiveHeader.lastTileWidth == 0) {
        int bufferOffset;
        switch (imageNumber) {
            case 0: { bufferOffset = 0; break; }
            case 1: { bufferOffset = receiveHeader.width * bits0/8; break; }
            case 2: { bufferOffset = receiveHeader.width * (bits0 + bits1)/8; break; }
            default:
                throw ProtocolException("Not implemented: image index > 2");
        }
        int bufferRowStride = receiveHeader.width*(totalBits) / 8;

        if(format == ImageSet::FORMAT_8_BIT_MONO || format == ImageSet::FORMAT_8_BIT_RGB) {
            // No decoding is necessary. We can just pass through the
            // data pointer
            ret = &data[bufferOffset];
            rowStride = bufferRowStride;
            validRows = receivedBytes / bufferRowStride;
        } else {
            // Perform 12-bit => 16 bit decoding
            allocateDecodeBuffer(imageNumber);
            validRows = std::min(receivedBytes / bufferRowStride, (int)receiveHeader.height);
            rowStride = 2*receiveHeader.width;
            int lastRow = lastReceivedPayloadBytes[imageNumber] / bufferRowStride;

            BitConversions::decode12BitPacked(lastRow, validRows, &data[bufferOffset],
                &decodeBuffer[imageNumber][0], bufferRowStride, rowStride, receiveHeader.width);

            ret = &decodeBuffer[imageNumber][0];
        }
    } else {
        // Decode the tiled transfer
        decodeTiledImage(imageNumber,
            lastReceivedPayloadBytes[imageNumber], receivedBytes, data,
            receiveHeader.firstTileWidth * (totalBits) / 8,
            receiveHeader.middleTilesWidth * (totalBits) / 8,
            receiveHeader.lastTileWidth * (totalBits) / 8,
            validRows, format, true);
        ret = &decodeBuffer[imageNumber][0];
        rowStride = receiveHeader.width*getFormatBits(
            static_cast<ImageSet::ImageFormat>(format), true)/8;
    }

    lastReceivedPayloadBytes[imageNumber] = receivedBytes;
    return ret;
}

void ImageProtocol::Pimpl::allocateDecodeBuffer(int imageNumber) {
    ImageSet::ImageFormat format;
    switch (imageNumber) {
        case 0:
            format = static_cast<ImageSet::ImageFormat>(receiveHeader.format0);
            break;
        case 1:
            format = static_cast<ImageSet::ImageFormat>(receiveHeader.format1);
            break;
        case 2:
            format = static_cast<ImageSet::ImageFormat>(receiveHeader.format2);
            break;
        case 3:
            format = static_cast<ImageSet::ImageFormat>(receiveHeader.format3);
            break;
        default:
            throw ProtocolException("Not implemented: allocateDecodeBuffer with image index > 2");
    }
    int bitsPerPixel = getFormatBits(format, true);
    int bufferSize = receiveHeader.width * receiveHeader.height * bitsPerPixel / 8;

    if(decodeBuffer[imageNumber].size() != static_cast<unsigned int>(bufferSize)) {
        decodeBuffer[imageNumber].resize(bufferSize);
    }
}

void ImageProtocol::Pimpl::decodeTiledImage(int imageNumber, int lastReceivedPayloadBytes, int receivedPayloadBytes,
        const unsigned char* data, int firstTileStride, int middleTilesStride, int lastTileStride, int& validRows,
        ImageSet::ImageFormat format, bool dataIsInterleaved) {
    // Allocate a decoding buffer
    allocateDecodeBuffer(imageNumber);

    // Get beginning and end of first tile
    int numTiles = getNumTiles(receiveHeader.width, receiveHeader.firstTileWidth,
        receiveHeader.middleTilesWidth, receiveHeader.lastTileWidth);
    int payloadOffset = 0;
    int decodeXOffset = 0;
    int prevTileStrides = 0;
    for(int i = 0; i < numTiles; i++) {
        // Get relevant parameters
        int tileWidth = 0;
        int tileStride = 0;

        if(i == 0) {
            tileStride = firstTileStride;
            tileWidth = receiveHeader.firstTileWidth;
        } else if(i == numTiles-1) {
            tileStride = lastTileStride;
            tileWidth = receiveHeader.lastTileWidth;
        } else {
            tileStride = middleTilesStride;
            tileWidth = receiveHeader.middleTilesWidth;
        }

        int tileStart = std::max(0, (lastReceivedPayloadBytes - payloadOffset) / tileStride);
        int tileStop = std::min(std::max(0, (receivedPayloadBytes - payloadOffset) / tileStride), (int)receiveHeader.height);
        int tileOffset;
        if (dataIsInterleaved) {
            switch (imageNumber) {
                case 0: { tileOffset = 0; break; }
                case 1: { tileOffset = tileWidth * (
                                getFormatBits(static_cast<ImageSet::ImageFormat>(receiveHeader.format0), false)
                                )/8; break; }
                case 2: { tileOffset = tileWidth * (
                                getFormatBits(static_cast<ImageSet::ImageFormat>(receiveHeader.format0), false)
                                + getFormatBits(static_cast<ImageSet::ImageFormat>(receiveHeader.format1), false)
                                )/8; break; }
                default:
                    throw ProtocolException("Not implemented: image index > 2");
            }
        } else {
            tileOffset = 0;
        }
        if(i > 0) {
            tileOffset += receiveHeader.height * prevTileStrides;
        }

        // Decode
        int bytesPixel;
        if(format == ImageSet::FORMAT_12_BIT_MONO) {
            bytesPixel = 2;
            BitConversions::decode12BitPacked(tileStart, tileStop, &data[tileOffset],
                &decodeBuffer[imageNumber][decodeXOffset], tileStride, 2*receiveHeader.width, tileWidth);
        } else {
            bytesPixel = (format == ImageSet::FORMAT_8_BIT_RGB ? 3 : 1);
            decodeRowsFromTile(tileStart, tileStop, &data[tileOffset],
                &decodeBuffer[imageNumber][decodeXOffset], tileStride,
                receiveHeader.width*bytesPixel, tileWidth*bytesPixel);
        }

        payloadOffset += receiveHeader.height * tileStride;
        decodeXOffset += tileWidth * bytesPixel;
        prevTileStrides += tileStride;
        if(i == numTiles-1) {
            validRows = tileStop;
        }
    }
}

void ImageProtocol::Pimpl::decodeRowsFromTile(int startRow, int stopRow, unsigned const char* src,
        unsigned char* dst, int srcStride, int dstStride, int tileWidth) {
    for(int y = startRow; y < stopRow; y++) {
        memcpy(&dst[y*dstStride], &src[y*srcStride], tileWidth);
    }
}

void ImageProtocol::Pimpl::resetReception() {
    receiveHeaderParsed = false;
    for (int i=0; i<ImageSet::MAX_SUPPORTED_IMAGES; ++i) {
        lastReceivedPayloadBytes[i] = 0;
    }
    dataProt.resetReception(false);
    receptionDone = false;
}

bool ImageProtocol::Pimpl::isConnected() const {
    return dataProt.isConnected();
}

const unsigned char* ImageProtocol::Pimpl::getNextControlMessage(int& length) {
    return dataProt.getNextControlMessage(length);
}

bool ImageProtocol::Pimpl::newClientConnected() {
    return dataProt.newClientConnected();
}

int ImageProtocol::Pimpl::getNumDroppedFrames() const {
    return dataProt.getDroppedReceptions();
}

std::string ImageProtocol::statusReport() {
    return pimpl->statusReport();
}
std::string ImageProtocol::Pimpl::statusReport() {
    return dataProt.statusReport();
}



} // namespace

