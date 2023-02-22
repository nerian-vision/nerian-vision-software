#include <genicam/gentl.h>
#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <thread>
#include "test-common.h"

using namespace std;
using namespace GenTL;

class EventFixture: public ::testing::Test {
public:
   EventFixture(): hSystem(nullptr), hIface(nullptr), hDevice(nullptr),
        hStream(nullptr), hBuffer(nullptr) {
   }

   ~EventFixture() {
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
        ASSERT_EQ(GC_ERR_SUCCESS, DSAllocAndAnnounceBuffer(hStream, 640*480, &userData, &hBuffer));

   }

   virtual void TearDown( ) {
        EXPECT_EQ(GC_ERR_SUCCESS, DSFlushQueue(hStream, ACQ_QUEUE_ALL_DISCARD));
        if(hBuffer != nullptr) {
            ASSERT_EQ(GC_ERR_SUCCESS, DSRevokeBuffer(hStream, hBuffer, nullptr, nullptr));
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
    BUFFER_HANDLE hBuffer;
    int userData;
};

TEST_F(EventFixture, StreamErrorEvent) {
    EVENT_HANDLE hEvent = nullptr;
    EXPECT_EQ(GC_ERR_SUCCESS, GCRegisterEvent(hStream, EVENT_ERROR, &hEvent));

    GC_ERROR err;
    size_t size = sizeof(err);
    EXPECT_EQ(GC_ERR_TIMEOUT, EventGetData(hEvent, &err, &size, 100));

    // Produce a buffer too small error
    BUFFER_HANDLE hSmallBuffer;
    EXPECT_EQ(GC_ERR_SUCCESS, DSAllocAndAnnounceBuffer(hStream, 1000, &userData, &hSmallBuffer));
    EXPECT_EQ(GC_ERR_SUCCESS, DSQueueBuffer(hStream, hSmallBuffer));

    EXPECT_EQ(GC_ERR_SUCCESS, DSStartAcquisition(hStream, ACQ_START_FLAGS_DEFAULT, 1));

    // Start thread to monitor for error
    std::thread thread([&]{
        EXPECT_EQ(GC_ERR_SUCCESS, EventGetData(hEvent, &err, &size, 1000));
        EXPECT_EQ(size, sizeof(err));
        EXPECT_EQ(err, GC_ERR_BUFFER_TOO_SMALL);

        GC_ERROR err2;
        size = sizeof(err2);
        INFO_DATATYPE type;
        EXPECT_EQ(GC_ERR_SUCCESS, EventGetDataInfo(hEvent, &err, sizeof(err),
            EVENT_DATA_ID, &type, &err2, &size));
        EXPECT_EQ(type, INFO_DATATYPE_INT32);
        EXPECT_EQ(err, err2);
        EXPECT_EQ(size, sizeof(err2));

        char buf[100];
        size = sizeof(buf);
        EXPECT_EQ(GC_ERR_SUCCESS, EventGetDataInfo(hEvent, &err, sizeof(err),
            EVENT_DATA_VALUE, &type, buf, &size));
        EXPECT_EQ(type, INFO_DATATYPE_STRING);
        cout << "Error string: " << buf << endl;
    });

    thread.join();

    EXPECT_EQ(GC_ERR_SUCCESS, DSStopAcquisition(hStream, ACQ_START_FLAGS_DEFAULT));
    EXPECT_EQ(GC_ERR_SUCCESS, GCUnregisterEvent(hStream, EVENT_ERROR));

    EXPECT_EQ(GC_ERR_SUCCESS, DSFlushQueue(hStream, ACQ_QUEUE_ALL_DISCARD));
    ASSERT_EQ(GC_ERR_SUCCESS, DSRevokeBuffer(hStream, hSmallBuffer, nullptr, nullptr));
}

TEST_F(EventFixture, KillEvent) {
    EVENT_HANDLE hEvent = nullptr;
    EXPECT_EQ(GC_ERR_SUCCESS, GCRegisterEvent(hDevice, EVENT_ERROR, &hEvent));

    // Start thread to monitor for error
    std::thread thread([&]{
        GC_ERROR err;
        size_t size = sizeof(err);
        EXPECT_EQ(GC_ERR_ABORT, EventGetData(hEvent, &err, &size, 2000));
    });

    sleep(1);
    EventKill(hEvent);
    thread.join();

    EXPECT_EQ(GC_ERR_SUCCESS, GCUnregisterEvent(hDevice, EVENT_ERROR));
}

TEST_F(EventFixture, NewBufferEvent) {
    EVENT_HANDLE hEvent = nullptr;
    EXPECT_EQ(GC_ERR_SUCCESS, GCRegisterEvent(hStream, EVENT_NEW_BUFFER, &hEvent));

    EXPECT_EQ(GC_ERR_SUCCESS, DSQueueBuffer(hStream, hBuffer));
    EXPECT_EQ(GC_ERR_SUCCESS, DSStartAcquisition(hStream, ACQ_START_FLAGS_DEFAULT, GENTL_INFINITE));

    sleep(1);

    size_t sizeVal = 0;
    size_t size = sizeof(sizeVal);
    INFO_DATATYPE type;
    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_DELIVERED, &type, &sizeVal, &size));
    EXPECT_EQ(sizeVal, 0);

    S_EVENT_NEW_BUFFER bufferEvent;
    size = sizeof(bufferEvent);
    EXPECT_EQ(GC_ERR_SUCCESS, EventGetData(hEvent, &bufferEvent, &size, 100));
    EXPECT_EQ(bufferEvent.BufferHandle, hBuffer);
    EXPECT_EQ(bufferEvent.pUserPointer, &userData);

    EXPECT_EQ(GC_ERR_SUCCESS, DSGetInfo(hStream, STREAM_INFO_NUM_DELIVERED, &type, &sizeVal, &size));
    EXPECT_EQ(1, sizeVal);

    BUFFER_HANDLE hBuffer2;
    void* userPtr2;
    size = sizeof(hBuffer2);
    EXPECT_EQ(GC_ERR_SUCCESS, EventGetDataInfo(hEvent, &bufferEvent, sizeof(bufferEvent),
        EVENT_DATA_ID, &type, &hBuffer2, &size));
    EXPECT_EQ(type, INFO_DATATYPE_PTR);
    EXPECT_EQ(hBuffer2, hBuffer);
    EXPECT_EQ(size, sizeof(hBuffer2));

    size = sizeof(userPtr2);
    EXPECT_EQ(GC_ERR_SUCCESS, EventGetDataInfo(hEvent, &bufferEvent, sizeof(bufferEvent),
        EVENT_DATA_VALUE, &type, &userPtr2, &size));
    EXPECT_EQ(type, INFO_DATATYPE_PTR);
    EXPECT_EQ(userPtr2, &userData);
    EXPECT_EQ(size, sizeof(userPtr2));

    EXPECT_EQ(GC_ERR_SUCCESS, GCUnregisterEvent(hStream, EVENT_NEW_BUFFER));
}

TEST_F(EventFixture, Flush) {
    EVENT_HANDLE hEvent = nullptr;
    EXPECT_EQ(GC_ERR_SUCCESS, GCRegisterEvent(hStream, EVENT_NEW_BUFFER, &hEvent));

    EXPECT_EQ(GC_ERR_SUCCESS, DSQueueBuffer(hStream, hBuffer));
    EXPECT_EQ(GC_ERR_SUCCESS, DSStartAcquisition(hStream, ACQ_START_FLAGS_DEFAULT, 1));

    sleep(1);

    EXPECT_EQ(GC_ERR_SUCCESS, EventFlush(hEvent));

    S_EVENT_NEW_BUFFER bufferEvent;
    size_t size = sizeof(bufferEvent);
    EXPECT_EQ(GC_ERR_TIMEOUT, EventGetData(hEvent, &bufferEvent, &size, 100));

    EXPECT_EQ(GC_ERR_SUCCESS, GCUnregisterEvent(hStream, EVENT_NEW_BUFFER));
}

TEST_F(EventFixture, EventInfo) {
    EVENT_HANDLE hEvent = nullptr;
    EXPECT_EQ(GC_ERR_SUCCESS, GCRegisterEvent(hDevice, EVENT_ERROR, &hEvent));

    cout << endl << "EventGetInfo():" << endl;

    TEST_INFO_BEGIN(EVENT_EVENT_TYPE, EVENT_INFO_DATA_SIZE_MAX);
    TEST_INFO_TYPE(EVENT_EVENT_TYPE, INFO_DATATYPE_INT32);
    TEST_INFO_TYPE(EVENT_NUM_IN_QUEUE, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(EVENT_NUM_FIRED, INFO_DATATYPE_UINT64);
    TEST_INFO_TYPE(EVENT_SIZE_MAX, INFO_DATATYPE_SIZET);
    TEST_INFO_TYPE(EVENT_INFO_DATA_SIZE_MAX, INFO_DATATYPE_SIZET);

    TEST_INFO_CALL(EventGetInfo(hEvent, command, &receivedType, buffer, &size));
    TEST_INFO_END();
    cout << endl;

    EXPECT_EQ(GC_ERR_SUCCESS, GCUnregisterEvent(hDevice, EVENT_ERROR));
}

TEST_F(EventFixture, EventInfo2) {
    EVENT_HANDLE hEvent = nullptr;
    EXPECT_EQ(GC_ERR_SUCCESS, GCRegisterEvent(hStream, EVENT_ERROR, &hEvent));

    // Verify that fired and queued are both 0
    size_t queued = 0;
    uint64_t fired = 0;
    size_t size = sizeof(queued);
    INFO_DATATYPE type;

    size = sizeof(queued);
    EXPECT_EQ(GC_ERR_SUCCESS, EventGetInfo(hEvent, EVENT_NUM_IN_QUEUE, &type, &queued, &size));
    EXPECT_EQ(queued, 0);
    size = sizeof(fired);
    EXPECT_EQ(GC_ERR_SUCCESS, EventGetInfo(hEvent, EVENT_NUM_FIRED, &type, &fired, &size));
    EXPECT_EQ(fired, 0);

    // Produce a buffer too small error
    BUFFER_HANDLE hSmallBuffer;
    EXPECT_EQ(GC_ERR_SUCCESS, DSAllocAndAnnounceBuffer(hStream, 1000, &userData, &hSmallBuffer));
    EXPECT_EQ(GC_ERR_SUCCESS, DSQueueBuffer(hStream, hSmallBuffer));
    EXPECT_EQ(GC_ERR_SUCCESS, DSStartAcquisition(hStream, ACQ_START_FLAGS_DEFAULT, 1));

    sleep(1);

    // Verify fired and queued
    size = sizeof(queued);
    EXPECT_EQ(GC_ERR_SUCCESS, EventGetInfo(hEvent, EVENT_NUM_IN_QUEUE, &type, &queued, &size));
    EXPECT_EQ(queued, 1);
    size = sizeof(fired);
    EXPECT_EQ(GC_ERR_SUCCESS, EventGetInfo(hEvent, EVENT_NUM_FIRED, &type, &fired, &size));
    EXPECT_EQ(fired, 1);

    // Produce another error
    EXPECT_EQ(GC_ERR_SUCCESS, DSQueueBuffer(hStream, hSmallBuffer));
    EXPECT_EQ(GC_ERR_SUCCESS, DSStartAcquisition(hStream, ACQ_START_FLAGS_DEFAULT, 1));

    sleep(1);

    // Verify fired and queued
    size = sizeof(queued);
    EXPECT_EQ(GC_ERR_SUCCESS, EventGetInfo(hEvent, EVENT_NUM_IN_QUEUE, &type, &queued, &size));
    EXPECT_EQ(queued, 2);
    size = sizeof(fired);
    EXPECT_EQ(GC_ERR_SUCCESS, EventGetInfo(hEvent, EVENT_NUM_FIRED, &type, &fired, &size));
    EXPECT_EQ(fired, 2);

    // Collect one event
    GC_ERROR err;
    size = sizeof(err);
    EXPECT_EQ(GC_ERR_SUCCESS, EventGetData(hEvent, &err, &size, 1000));

    // Verify fired and queued
    size = sizeof(queued);
    EXPECT_EQ(GC_ERR_SUCCESS, EventGetInfo(hEvent, EVENT_NUM_IN_QUEUE, &type, &queued, &size));
    EXPECT_EQ(queued, 1);
    size = sizeof(fired);
    EXPECT_EQ(GC_ERR_SUCCESS, EventGetInfo(hEvent, EVENT_NUM_FIRED, &type, &fired, &size));
    EXPECT_EQ(fired, 2);

    EXPECT_EQ(GC_ERR_SUCCESS, GCUnregisterEvent(hStream, EVENT_ERROR));
}
