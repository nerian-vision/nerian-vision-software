#include <genicam/gentl.h>
#include <gtest/gtest.h>
#include <iostream>
#include "test-common.h"

using namespace std;
using namespace GenTL;

class DeviceFixture: public ::testing::Test {
public:
   DeviceFixture(): hSystem(nullptr), hIface(nullptr), hDevice0(nullptr), hDevice1(nullptr) {
   }

   ~DeviceFixture() {
   }

   virtual void SetUp( ) {
        ASSERT_EQ(GC_ERR_SUCCESS, GCInitLib());
        ASSERT_EQ(GC_ERR_SUCCESS, TLOpen(&hSystem));
        ASSERT_EQ(GC_ERR_SUCCESS, TLOpenInterface(hSystem, "eth", &hIface));
        EXPECT_EQ(GC_ERR_SUCCESS, IFUpdateDeviceList(hIface, nullptr, 1000));

        // Open first two devices
        char buffer[100];
        size_t size = sizeof(buffer);
        EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, 0, buffer, &size));
        ASSERT_EQ(GC_ERR_SUCCESS, IFOpenDevice(hIface, buffer, DEVICE_ACCESS_READONLY, &hDevice0));

        size = sizeof(buffer);
        EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, 1, buffer, &size));
        ASSERT_EQ(GC_ERR_SUCCESS, IFOpenDevice(hIface, buffer, DEVICE_ACCESS_READONLY, &hDevice1));
   }

   virtual void TearDown( ) {
        if(hDevice0 != nullptr) {
            ASSERT_EQ(GC_ERR_SUCCESS, DevClose(hDevice0));
        }
        if(hDevice1 != nullptr) {
            ASSERT_EQ(GC_ERR_SUCCESS, DevClose(hDevice1));
        }
        ASSERT_EQ(GC_ERR_SUCCESS, IFClose(hIface));
        ASSERT_EQ(GC_ERR_SUCCESS, TLClose(hSystem));
        ASSERT_EQ(GC_ERR_SUCCESS, GCCloseLib());
   }

protected:
    TL_HANDLE hSystem;
    IF_HANDLE hIface;
    DEV_HANDLE hDevice0;
    DEV_HANDLE hDevice1;

    std::string host;
};

TEST_F(DeviceFixture, ParentIF) {
    IF_HANDLE hParentIf = nullptr;
    EXPECT_EQ(GC_ERR_SUCCESS, DevGetParentIF(hDevice0, &hParentIf));
    EXPECT_EQ(hParentIf, hIface);
}

TEST_F(DeviceFixture, NumStreams) {
    uint32_t numStreams = 0;
    EXPECT_EQ(GC_ERR_SUCCESS, DevGetNumDataStreams(hDevice0, &numStreams));
    EXPECT_EQ(numStreams, 1);
}

TEST_F(DeviceFixture, StreamID) {
    char buffer[100];
    size_t size = sizeof(buffer);
    EXPECT_EQ(GC_ERR_SUCCESS, DevGetDataStreamID(hDevice0, 0, buffer, &size));
    EXPECT_EQ(string(buffer), string("default"));

    size = sizeof(buffer);
    EXPECT_EQ(GC_ERR_SUCCESS, DevGetDataStreamID(hDevice1, 0, buffer, &size));
    EXPECT_EQ(string(buffer), string("default"));
}

TEST_F(DeviceFixture, OpenMultiple) {
    DEVICE_ACCESS_STATUS accessStatus;
    size_t size;
    INFO_DATATYPE type;
    char buffer[100];

    // Check access status
    for(int i=0; i<5; i++) {
        size = sizeof(buffer);
        EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, i, buffer, &size));

        size = sizeof(accessStatus);
        EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceInfo(hIface, buffer, DEVICE_INFO_ACCESS_STATUS, &type, &accessStatus, &size));

        if(i <= 1) {
            EXPECT_EQ(accessStatus, DEVICE_ACCESS_STATUS_BUSY);
        } else {
            EXPECT_EQ(accessStatus, DEVICE_ACCESS_STATUS_READWRITE);
        }
    }

    // Try re-open 0 device
    DEV_HANDLE hDummy;
    size = sizeof(buffer);
    EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, 0, buffer, &size));
    ASSERT_EQ(GC_ERR_RESOURCE_IN_USE, IFOpenDevice(hIface, buffer, DEVICE_ACCESS_READONLY, &hDummy));

    // Open devices 2 to 3
    for(int i = 2; i<= 4; i++) {
        size = sizeof(buffer);
        EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, i, buffer, &size));
        ASSERT_EQ(GC_ERR_SUCCESS, IFOpenDevice(hIface, buffer, DEVICE_ACCESS_READONLY, &hDummy));
        ASSERT_EQ(GC_ERR_SUCCESS, DevClose(hDummy));
    }

    // Close devices
    ASSERT_EQ(GC_ERR_SUCCESS, DevClose(hDevice0));
    hDevice0 = nullptr;
    ASSERT_EQ(GC_ERR_SUCCESS, DevClose(hDevice1));
    hDevice1 = nullptr;

    // Check access status
    for(int i=0; i<5; i++) {
        size = sizeof(buffer);
        EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, i, buffer, &size));

        size = sizeof(accessStatus);
        EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceInfo(hIface, buffer, DEVICE_INFO_ACCESS_STATUS, &type, &accessStatus, &size));
        EXPECT_EQ(accessStatus, DEVICE_ACCESS_STATUS_READWRITE);
    }
}
