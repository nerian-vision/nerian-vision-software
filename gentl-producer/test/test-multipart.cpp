#include <genicam/gentl.h>
#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unistd.h>
#include "test-common.h"

using namespace std;
using namespace GenTL;

class MultipartFixture: public ::testing::Test {
public:
   MultipartFixture(): hSystem(nullptr), hIface(nullptr), hDevice(nullptr),
        hStream(nullptr), hBuffer0(nullptr), hBuffer1(nullptr), hBuffer2(nullptr) {
   }

   ~MultipartFixture() {
   }

   virtual void SetUp( ) {
        ASSERT_EQ(GC_ERR_SUCCESS, GCInitLib());
        ASSERT_EQ(GC_ERR_SUCCESS, TLOpen(&hSystem));
        ASSERT_EQ(GC_ERR_SUCCESS, TLOpenInterface(hSystem, "eth", &hIface));
        EXPECT_EQ(GC_ERR_SUCCESS, IFUpdateDeviceList(hIface, nullptr, 1000));

        // Open multi-part device
        char buffer[100];
        size_t size = sizeof(buffer);
        EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, 0, buffer, &size));
        ASSERT_EQ(GC_ERR_SUCCESS, IFOpenDevice(hIface, buffer, DEVICE_ACCESS_READONLY, &hDevice));

        ASSERT_EQ(GC_ERR_SUCCESS, DevOpenDataStream(hDevice, "default", &hStream));
        ASSERT_EQ(GC_ERR_SUCCESS, DSAllocAndAnnounceBuffer(hStream, 15*640*480, nullptr, &hBuffer0));
        ASSERT_EQ(GC_ERR_SUCCESS, DSAllocAndAnnounceBuffer(hStream, 15*640*480, nullptr, &hBuffer1));
        ASSERT_EQ(GC_ERR_SUCCESS, DSAllocAndAnnounceBuffer(hStream, 15*640*480, nullptr, &hBuffer2));
   }

   virtual void TearDown( ) {
        EXPECT_EQ(GC_ERR_SUCCESS, DSFlushQueue(hStream, ACQ_QUEUE_ALL_DISCARD));
        if(hBuffer0 != nullptr) {
            ASSERT_EQ(GC_ERR_SUCCESS, DSRevokeBuffer(hStream, hBuffer0, nullptr, nullptr));
        }
        if(hBuffer1 != nullptr) {
            ASSERT_EQ(GC_ERR_SUCCESS, DSRevokeBuffer(hStream, hBuffer1, nullptr, nullptr));
        }
        if(hBuffer2 != nullptr) {
            ASSERT_EQ(GC_ERR_SUCCESS, DSRevokeBuffer(hStream, hBuffer2, nullptr, nullptr));
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
    BUFFER_HANDLE hBuffer2;
};

TEST_F(MultipartFixture, MultiPart) {
    // Test the multipart stream
    uint32_t numParts = 0;

    EXPECT_EQ(GC_ERR_SUCCESS, DSGetNumBufferParts(hStream, hBuffer0, &numParts));
    EXPECT_EQ(3, numParts);

    EXPECT_EQ(GC_ERR_SUCCESS, DSFlushQueue(hStream, ACQ_QUEUE_ALL_DISCARD));
}

