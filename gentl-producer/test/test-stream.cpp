#include <genicam/gentl.h>
#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unistd.h>
#include "test-common.h"

using namespace std;
using namespace GenTL;

class StreamFixture: public ::testing::Test {
public:
   StreamFixture(): hSystem(nullptr), hIface(nullptr), hDevice(nullptr),
        hStream(nullptr), hBuffer0(nullptr), hBuffer1(nullptr) {
   }

   ~StreamFixture() {
   }

   virtual void SetUp( ) {
        ASSERT_EQ(GC_ERR_SUCCESS, GCInitLib());
        ASSERT_EQ(GC_ERR_SUCCESS, TLOpen(&hSystem));
        ASSERT_EQ(GC_ERR_SUCCESS, TLOpenInterface(hSystem, "eth", &hIface));
        EXPECT_EQ(GC_ERR_SUCCESS, IFUpdateDeviceList(hIface, nullptr, 1000));

        // open second device
        char buffer[100];
        size_t size = sizeof(buffer);
        EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, 1, buffer, &size));
        ASSERT_EQ(GC_ERR_SUCCESS, IFOpenDevice(hIface, buffer, DEVICE_ACCESS_READONLY, &hDevice));

        ASSERT_EQ(GC_ERR_SUCCESS, DevOpenDataStream(hDevice, "default", &hStream));
        ASSERT_EQ(GC_ERR_SUCCESS, DSAllocAndAnnounceBuffer(hStream, 640*480, nullptr, &hBuffer0));
        ASSERT_EQ(GC_ERR_SUCCESS, DSAllocAndAnnounceBuffer(hStream, 640*480, nullptr, &hBuffer1));
   }

   virtual void TearDown( ) {
        EXPECT_EQ(GC_ERR_SUCCESS, DSFlushQueue(hStream, ACQ_QUEUE_ALL_DISCARD));
        if(hBuffer0 != nullptr) {
            ASSERT_EQ(GC_ERR_SUCCESS, DSRevokeBuffer(hStream, hBuffer0, nullptr, nullptr));
        }
        if(hBuffer1 != nullptr) {
            ASSERT_EQ(GC_ERR_SUCCESS, DSRevokeBuffer(hStream, hBuffer1, nullptr, nullptr));
        }
        ASSERT_EQ(GC_ERR_SUCCESS, DSClose(hStream));
        ASSERT_EQ(GC_ERR_SUCCESS, DevClose(hDevice));
        ASSERT_EQ(GC_ERR_SUCCESS, IFClose(hIface));
        ASSERT_EQ(GC_ERR_SUCCESS, TLClose(hSystem));
        ASSERT_EQ(GC_ERR_SUCCESS, GCCloseLib());
   }

protected:
    TL_HANDLE hSystem;
    IF_HANDLE hIface;
    DEV_HANDLE hDevice;
    DS_HANDLE hStream;
    BUFFER_HANDLE hBuffer0;
    BUFFER_HANDLE hBuffer1;
};


TEST_F(StreamFixture, Reopen) {
    DS_HANDLE hStream2;
    ASSERT_EQ(GC_ERR_RESOURCE_IN_USE, DevOpenDataStream(hDevice, "default", &hStream2));
    ASSERT_EQ(GC_ERR_SUCCESS, DSClose(hStream2));
    ASSERT_EQ(GC_ERR_SUCCESS, DevOpenDataStream(hDevice, "default", &hStream));
}

TEST_F(StreamFixture, BufferAllocation) {
    // First free the default buffers
    ASSERT_EQ(GC_ERR_SUCCESS, DSRevokeBuffer(hStream, hBuffer0, nullptr, nullptr));
    ASSERT_EQ(GC_ERR_SUCCESS, DSRevokeBuffer(hStream, hBuffer1, nullptr, nullptr));

    // Buffer allocations
    size_t sizeVal = 0;
    size_t size = sizeof(sizeVal);
    INFO_DATATYPE type;
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_ANNOUNCED, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 0);

    vector<unsigned char> buffer(640*480);
    size = buffer.size();
    int privateData = 0;
    EXPECT_EQ(GC_ERR_SUCCESS, DSAnnounceBuffer(hStream, &buffer[0], size, &privateData, &hBuffer0));

    EXPECT_EQ(GC_ERR_SUCCESS, DSAllocAndAnnounceBuffer(hStream, size, &privateData, &hBuffer1));

    size = sizeof(sizeVal);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_ANNOUNCED, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 2);

    void* dataPtr = nullptr;
    void* privatePtr = nullptr;
    ASSERT_EQ(GC_ERR_SUCCESS, DSRevokeBuffer(hStream, hBuffer0, &dataPtr, &privatePtr));
    ASSERT_EQ(dataPtr, &buffer[0]);
    ASSERT_EQ(privatePtr, &privateData);

    ASSERT_EQ(GC_ERR_SUCCESS, DSRevokeBuffer(hStream, hBuffer1, &dataPtr, &privatePtr));
    ASSERT_EQ(dataPtr, nullptr);
    ASSERT_EQ(privatePtr, &privateData);

    hBuffer0 = nullptr;
    hBuffer1 = nullptr;
}

TEST_F(StreamFixture, BufferQueueing) {
    unsigned char boolData = 0xff;
    size_t size = sizeof(boolData);
    INFO_DATATYPE type;
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferInfo(hStream, hBuffer0, BUFFER_INFO_IS_QUEUED, &type, &boolData, &size));
    EXPECT_EQ(boolData, 0);

    EXPECT_EQ(GC_ERR_SUCCESS, DSQueueBuffer(hStream, hBuffer0));

    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferInfo(hStream, hBuffer0, BUFFER_INFO_IS_QUEUED, &type, &boolData, &size));
    EXPECT_EQ(boolData, 1);
}

TEST_F(StreamFixture, ParentDevice) {
    DEV_HANDLE hParent;
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetParentDev(hStream, &hParent));
    EXPECT_EQ(hParent, hDevice);
}

TEST_F(StreamFixture, SingleFrameAcquisition) {
    uint8_t boolData;
    size_t size = sizeof(boolData);
    INFO_DATATYPE type;

    EXPECT_EQ(GC_ERR_SUCCESS, DSQueueBuffer(hStream, hBuffer0));

    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_IS_GRABBING, &type, &boolData, &size));
    EXPECT_EQ(boolData, 0);

    size = sizeof(boolData);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferInfo(hStream, hBuffer0, BUFFER_INFO_NEW_DATA, &type, &boolData, &size));
    EXPECT_EQ(boolData, 0);

    size_t sizeVal;
    size = sizeof(sizeVal);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_QUEUED, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 1);

    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_AWAIT_DELIVERY, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 0);

    EXPECT_EQ(GC_ERR_SUCCESS, DSStartAcquisition(hStream, ACQ_START_FLAGS_DEFAULT, 1));

    sleep(1);

    size = sizeof(boolData);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_IS_GRABBING, &type, &boolData, &size));
    EXPECT_EQ(boolData, 0);

    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferInfo(hStream, hBuffer0, BUFFER_INFO_NEW_DATA, &type, &boolData, &size));
    EXPECT_EQ(boolData, 1);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferInfo(hStream, hBuffer1, BUFFER_INFO_NEW_DATA, &type, &boolData, &size));
    EXPECT_EQ(boolData, 0);

    size = sizeof(sizeVal);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_QUEUED, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 0);

    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_AWAIT_DELIVERY, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 1);
}

TEST_F(StreamFixture, ContinuousAcquisition) {
    EXPECT_EQ(GC_ERR_SUCCESS, DSFlushQueue(hStream, ACQ_QUEUE_ALL_TO_INPUT));
    EXPECT_EQ(GC_ERR_SUCCESS, DSStartAcquisition(hStream, ACQ_START_FLAGS_DEFAULT, GENTL_INFINITE));

    sleep(1);

    uint8_t boolData;
    size_t size = sizeof(boolData);
    INFO_DATATYPE type;

    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferInfo(hStream, hBuffer0, BUFFER_INFO_NEW_DATA, &type, &boolData, &size));
    EXPECT_EQ(boolData, 1);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferInfo(hStream, hBuffer0, BUFFER_INFO_NEW_DATA, &type, &boolData, &size));
    EXPECT_EQ(boolData, 1);

    size_t sizeVal;
    size = sizeof(sizeVal);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_QUEUED, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 0);

    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_AWAIT_DELIVERY, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 2);

    size = sizeof(boolData);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_IS_GRABBING, &type, &boolData, &size));
    EXPECT_EQ(boolData, 1);

    EXPECT_EQ(GC_ERR_SUCCESS, DSStopAcquisition(hStream, ACQ_START_FLAGS_DEFAULT));

    size = sizeof(boolData);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_IS_GRABBING, &type, &boolData, &size));
    EXPECT_EQ(boolData, 0);
}

TEST_F(StreamFixture, Flushing) {
    size_t sizeVal;
    size_t size = sizeof(sizeVal);
    INFO_DATATYPE type;

    EXPECT_EQ(GC_ERR_SUCCESS, DSFlushQueue(hStream, ACQ_QUEUE_ALL_TO_INPUT));
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_AWAIT_DELIVERY, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 0);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_QUEUED, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 2);

    EXPECT_EQ(GC_ERR_SUCCESS, DSFlushQueue(hStream, ACQ_QUEUE_INPUT_TO_OUTPUT));
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_AWAIT_DELIVERY, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 2);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_QUEUED, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 0);

    EXPECT_EQ(GC_ERR_SUCCESS, DSFlushQueue(hStream, ACQ_QUEUE_OUTPUT_DISCARD));
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_AWAIT_DELIVERY, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 0);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_QUEUED, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 0);

    EXPECT_EQ(GC_ERR_SUCCESS, DSFlushQueue(hStream, ACQ_QUEUE_ALL_DISCARD));
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_AWAIT_DELIVERY, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 0);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_QUEUED, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 0);

    EXPECT_EQ(GC_ERR_SUCCESS, DSQueueBuffer(hStream, hBuffer0));
    EXPECT_EQ(GC_ERR_SUCCESS, DSFlushQueue(hStream, ACQ_QUEUE_INPUT_TO_OUTPUT));
    EXPECT_EQ(GC_ERR_SUCCESS, DSFlushQueue(hStream, ACQ_QUEUE_UNQUEUED_TO_INPUT));
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_QUEUED, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 1);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_AWAIT_DELIVERY, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 1);
}

TEST_F(StreamFixture, BufferId) {
    BUFFER_HANDLE bufById = nullptr;
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferID(hStream, 0, &bufById));
    EXPECT_EQ(bufById, hBuffer0);

    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferID(hStream, 1, &bufById));
    EXPECT_EQ(bufById, hBuffer1);
}

TEST_F(StreamFixture, BufferInfo) {
    EXPECT_EQ(GC_ERR_SUCCESS, DSFlushQueue(hStream, ACQ_QUEUE_ALL_TO_INPUT));
    EXPECT_EQ(GC_ERR_SUCCESS, DSStartAcquisition(hStream, ACQ_START_FLAGS_DEFAULT, 1));

    sleep(1);

    cout << endl << "DSGetBufferInfo():" << endl;
    TEST_INFO_BEGIN(BUFFER_INFO_BASE, BUFFER_INFO_CONTAINS_CHUNKDATA);
    TEST_INFO_TYPE(BUFFER_INFO_BASE, INFO_DATATYPE_PTR);
    TEST_INFO_TYPE(BUFFER_INFO_SIZE, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(BUFFER_INFO_USER_PTR, INFO_DATATYPE_PTR);
    TEST_INFO_TYPE(BUFFER_INFO_TIMESTAMP, INFO_DATATYPE_UINT64);
    TEST_INFO_TYPE(BUFFER_INFO_NEW_DATA, INFO_DATATYPE_BOOL8);
    TEST_INFO_TYPE(BUFFER_INFO_IS_QUEUED, INFO_DATATYPE_BOOL8);
    TEST_INFO_TYPE(BUFFER_INFO_IS_ACQUIRING, INFO_DATATYPE_BOOL8);
    TEST_INFO_TYPE(BUFFER_INFO_IS_INCOMPLETE, INFO_DATATYPE_BOOL8);
    TEST_INFO_TYPE(BUFFER_INFO_TLTYPE, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(BUFFER_INFO_SIZE_FILLED, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(BUFFER_INFO_WIDTH, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(BUFFER_INFO_HEIGHT, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(BUFFER_INFO_XOFFSET, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(BUFFER_INFO_YOFFSET, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(BUFFER_INFO_XPADDING, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(BUFFER_INFO_YPADDING, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(BUFFER_INFO_FRAMEID, INFO_DATATYPE_UINT64);
    TEST_INFO_TYPE(BUFFER_INFO_IMAGEPRESENT, INFO_DATATYPE_BOOL8);
    TEST_INFO_TYPE(BUFFER_INFO_IMAGEOFFSET, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(BUFFER_INFO_PAYLOADTYPE, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(BUFFER_INFO_PIXELFORMAT, INFO_DATATYPE_UINT64);
    TEST_INFO_TYPE(BUFFER_INFO_PIXELFORMAT_NAMESPACE, INFO_DATATYPE_UINT64);
    TEST_INFO_TYPE(BUFFER_INFO_DELIVERED_IMAGEHEIGHT, INFO_DATATYPE_SIZET);
    TEST_INFO_SKIP(BUFFER_INFO_DELIVERED_CHUNKPAYLOADSIZE);
    TEST_INFO_SKIP(BUFFER_INFO_CHUNKLAYOUTID);
    TEST_INFO_SKIP(BUFFER_INFO_FILENAME);
    TEST_INFO_TYPE(BUFFER_INFO_PIXEL_ENDIANNESS, INFO_DATATYPE_INT32);
    TEST_INFO_TYPE(BUFFER_INFO_DATA_SIZE, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(BUFFER_INFO_TIMESTAMP_NS, INFO_DATATYPE_UINT64);
    TEST_INFO_TYPE(BUFFER_INFO_DATA_LARGER_THAN_BUFFER, INFO_DATATYPE_BOOL8);
    TEST_INFO_TYPE(BUFFER_INFO_CONTAINS_CHUNKDATA, INFO_DATATYPE_BOOL8);

    TEST_INFO_CALL(DSGetBufferInfo(hStream, hBuffer0, command, &receivedType, buffer, &size));
    TEST_INFO_END();
    cout << endl;
}

TEST_F(StreamFixture, GetInfo) {
    cout << endl << "DSGetInfo():" << endl;
    TEST_INFO_BEGIN(STREAM_INFO_ID, STREAM_INFO_BUF_ALIGNMENT);
    TEST_INFO_TYPE(STREAM_INFO_ID, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(STREAM_INFO_NUM_DELIVERED, INFO_DATATYPE_UINT64);
    TEST_INFO_TYPE(STREAM_INFO_NUM_UNDERRUN, INFO_DATATYPE_UINT64);
    TEST_INFO_TYPE(STREAM_INFO_NUM_ANNOUNCED, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(STREAM_INFO_NUM_QUEUED, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(STREAM_INFO_NUM_AWAIT_DELIVERY, INFO_DATATYPE_SIZET);
    TEST_INFO_SKIP(STREAM_INFO_NUM_STARTED);
    TEST_INFO_TYPE(STREAM_INFO_PAYLOAD_SIZE, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(STREAM_INFO_IS_GRABBING, INFO_DATATYPE_BOOL8);
    TEST_INFO_TYPE(STREAM_INFO_DEFINES_PAYLOADSIZE, INFO_DATATYPE_BOOL8);
    TEST_INFO_TYPE(STREAM_INFO_TLTYPE, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(STREAM_INFO_NUM_CHUNKS_MAX, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(STREAM_INFO_BUF_ANNOUNCE_MIN, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(STREAM_INFO_BUF_ALIGNMENT, INFO_DATATYPE_SIZET);

    TEST_INFO_CALL(DSGetInfo(hStream, command, &receivedType, buffer, &size));
    TEST_INFO_END();
    cout << endl;
}

TEST_F(StreamFixture, BufferTooSmall) {
    BUFFER_HANDLE hSmallBuffer;
    EXPECT_EQ(GC_ERR_SUCCESS, DSAllocAndAnnounceBuffer(hStream, 1000, nullptr, &hSmallBuffer));

    // Capture normal buffer
    EXPECT_EQ(GC_ERR_SUCCESS, DSQueueBuffer(hStream, hBuffer0));
    EXPECT_EQ(GC_ERR_SUCCESS, DSStartAcquisition(hStream, ACQ_START_FLAGS_DEFAULT, 1));
    sleep(1);

    // Verify that all is complete
    bool8_t boolVal = 0xff;
    size_t filled = 0;
    size_t size = sizeof(boolVal);
    INFO_DATATYPE type;
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferInfo(hStream, hBuffer0, BUFFER_INFO_IS_INCOMPLETE, &type, &boolVal, &size));
    EXPECT_EQ(0 , boolVal);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferInfo(hStream, hBuffer0, BUFFER_INFO_DATA_LARGER_THAN_BUFFER, &type, &boolVal, &size));
    EXPECT_EQ(0, boolVal);
    size = sizeof(filled);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferInfo(hStream, hBuffer0, BUFFER_INFO_SIZE_FILLED, &type, &filled, &size));
    EXPECT_EQ(640*480, filled);

    // Capture too small buffer
    EXPECT_EQ(GC_ERR_SUCCESS, DSQueueBuffer(hStream, hSmallBuffer));
    EXPECT_EQ(GC_ERR_SUCCESS, DSStartAcquisition(hStream, ACQ_START_FLAGS_DEFAULT, 1));
    sleep(1);

    // Verify incompleteness
    size = sizeof(boolVal);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferInfo(hStream, hSmallBuffer, BUFFER_INFO_IS_INCOMPLETE, &type, &boolVal, &size));
    EXPECT_EQ(1 , boolVal);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferInfo(hStream, hSmallBuffer, BUFFER_INFO_DATA_LARGER_THAN_BUFFER, &type, &boolVal, &size));
    EXPECT_EQ(1, boolVal);
    size = sizeof(filled);
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferInfo(hStream, hSmallBuffer, BUFFER_INFO_SIZE_FILLED, &type, &filled, &size));
    EXPECT_EQ(0, filled);
}

TEST_F(StreamFixture, MultiPart) {
    // First test the non-multipart stream
    uint32_t numParts = 0;
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetNumBufferParts(hStream, hBuffer0, &numParts));
    EXPECT_EQ(1, numParts);

    // Test the multipart stream
    DEV_HANDLE hDeviceMulti;
    DS_HANDLE hStreamMulti;
    BUFFER_HANDLE hBufferMulti;

    char buffer[100];
    size_t size = sizeof(buffer);
    EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, 0, buffer, &size));

    ASSERT_EQ(GC_ERR_SUCCESS, IFOpenDevice(hIface, buffer, DEVICE_ACCESS_READONLY, &hDeviceMulti));
    ASSERT_EQ(GC_ERR_SUCCESS, DevOpenDataStream(hDeviceMulti, "default", &hStreamMulti));
    ASSERT_EQ(GC_ERR_SUCCESS, DSAllocAndAnnounceBuffer(hStreamMulti, 15*640*480, nullptr, &hBufferMulti));

    EXPECT_EQ(GC_ERR_SUCCESS, DSGetNumBufferParts(hStreamMulti, hBufferMulti, &numParts));
    EXPECT_EQ(3, numParts);

    EXPECT_EQ(GC_ERR_SUCCESS, DSFlushQueue(hStreamMulti, ACQ_QUEUE_ALL_DISCARD));
    ASSERT_EQ(GC_ERR_SUCCESS, DSRevokeBuffer(hStreamMulti, hBufferMulti, nullptr, nullptr));
    ASSERT_EQ(GC_ERR_SUCCESS, DSClose(hStreamMulti));
    ASSERT_EQ(GC_ERR_SUCCESS, DevClose(hDeviceMulti));
}

TEST_F(StreamFixture, getBufferInfoStacked) {
    EXPECT_EQ(GC_ERR_SUCCESS, DSFlushQueue(hStream, ACQ_QUEUE_ALL_TO_INPUT));
    EXPECT_EQ(GC_ERR_SUCCESS, DSStartAcquisition(hStream, ACQ_START_FLAGS_DEFAULT, 1));

    sleep(1);

    cout << endl << "DSGetBufferInfoStacked():" << endl;

    size_t width = 0, height = 0;
    int64_t format = 0;

    DS_BUFFER_INFO_STACKED infoStack[3];
    infoStack[0].iInfoCmd = BUFFER_INFO_WIDTH;
    infoStack[0].iType = INFO_DATATYPE_SIZET;
    infoStack[0].pBuffer = &width;
    infoStack[0].iSize = sizeof(width);

    infoStack[1].iInfoCmd = BUFFER_INFO_HEIGHT;
    infoStack[1].iType = INFO_DATATYPE_SIZET;
    infoStack[1].pBuffer = &height;
    infoStack[0].iSize = sizeof(height);

    infoStack[2].iInfoCmd = BUFFER_INFO_PIXELFORMAT;
    infoStack[2].iType = INFO_DATATYPE_UINT64;
    infoStack[2].pBuffer = &format;
    infoStack[0].iSize = sizeof(format);

    EXPECT_EQ(GC_ERR_SUCCESS, DSGetBufferInfoStacked(hStream, hBuffer0, infoStack, 3));

    for(int i=0; i<3; i++) {
        EXPECT_EQ(GC_ERR_SUCCESS, infoStack[i].iResult);
    }

    cout << "Width: " << width << endl;
    cout << "Height: " << height << endl;
    cout << "Format: " << format << endl;
    cout << endl;
}
