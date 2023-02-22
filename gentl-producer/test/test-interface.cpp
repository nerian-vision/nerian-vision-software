#include <genicam/gentl.h>
#include <gtest/gtest.h>
#include <iostream>
#include "test-common.h"

using namespace std;
using namespace GenTL;


class InterfaceFixture: public ::testing::Test {
public:
   InterfaceFixture(): hSystem(nullptr), hIface(nullptr) {
   }

   ~InterfaceFixture() {
   }

   virtual void SetUp( ) {
        ASSERT_EQ(GC_ERR_SUCCESS, GCInitLib());
        ASSERT_EQ(GC_ERR_SUCCESS, TLOpen(&hSystem));
        ASSERT_EQ(GC_ERR_SUCCESS, TLOpenInterface(hSystem, "eth", &hIface));
   }

   virtual void TearDown( ) {
        ASSERT_EQ(GC_ERR_SUCCESS, IFClose(hIface));
        ASSERT_EQ(GC_ERR_SUCCESS, TLClose(hSystem));
        ASSERT_EQ(GC_ERR_SUCCESS, GCCloseLib());
   }

protected:
    TL_HANDLE hSystem;
    IF_HANDLE hIface;
};

TEST_F(InterfaceFixture, NumDevices) {
    uint32_t numDevices = 0;
    EXPECT_EQ(GC_ERR_SUCCESS, IFGetNumDevices(hIface, &numDevices));
    EXPECT_GT(numDevices, 0);

    EXPECT_EQ(GC_ERR_SUCCESS, IFUpdateDeviceList(hIface, nullptr, 1000));

    EXPECT_EQ(GC_ERR_SUCCESS, IFGetNumDevices(hIface, &numDevices));
    EXPECT_GT(numDevices, 0);
}

TEST_F(InterfaceFixture, DeviceID) {
    EXPECT_EQ(GC_ERR_SUCCESS, IFUpdateDeviceList(hIface, nullptr, 1000));

    char buffer[100];

    size_t size = sizeof(buffer);
    EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, 0, buffer, &size));
    std::string bufferStr = buffer;
    EXPECT_EQ(bufferStr.substr(bufferStr.length() - 1), string("/"));

    size = sizeof(buffer);
    EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, 1, buffer, &size));
    bufferStr = buffer;
    EXPECT_EQ(bufferStr.substr(bufferStr.length() - 5), string("/left"));

    size = sizeof(buffer);
    EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, 2, buffer, &size));
    bufferStr = buffer;
    EXPECT_EQ(bufferStr.substr(bufferStr.length() - 6), string("/right"));

    size = sizeof(buffer);
    EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, 3, buffer, &size));
    bufferStr = buffer;
    EXPECT_EQ(bufferStr.substr(bufferStr.length() - 10), string("/disparity"));

    size = sizeof(buffer);
    EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, 4, buffer, &size));
    bufferStr = buffer;
    EXPECT_EQ(bufferStr.substr(bufferStr.length() - 11), string("/pointcloud"));
}

TEST_F(InterfaceFixture, ParentTL) {
    TL_HANDLE hParent;
    EXPECT_EQ(GC_ERR_SUCCESS, IFGetParentTL(hIface, &hParent));
    EXPECT_EQ(hParent, hSystem);
}

TEST_F(InterfaceFixture, DeviceInfo) {
    EXPECT_EQ(GC_ERR_SUCCESS, IFUpdateDeviceList(hIface, nullptr, 1000));

    cout << endl << "IFGetDeviceInfo():" << endl;
    TEST_INFO_BEGIN(DEVICE_INFO_ID, DEVICE_INFO_TIMESTAMP_FREQUENCY);

    TEST_INFO_TYPE(DEVICE_INFO_ID, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(DEVICE_INFO_VENDOR, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(DEVICE_INFO_MODEL, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(DEVICE_INFO_TLTYPE, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(DEVICE_INFO_DISPLAYNAME, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(DEVICE_INFO_ACCESS_STATUS, INFO_DATATYPE_INT32);

    TEST_INFO_SKIP(DEVICE_INFO_USER_DEFINED_NAME); // Not implemented
    TEST_INFO_SKIP(DEVICE_INFO_SERIAL_NUMBER); // Not implemented
    TEST_INFO_SKIP(DEVICE_INFO_VERSION); // Not implemented

    TEST_INFO_TYPE(DEVICE_INFO_TIMESTAMP_FREQUENCY, INFO_DATATYPE_UINT64);

    TEST_INFO_CALL(IFGetDeviceInfo(hIface, "udp://0.0.0.0", command, &receivedType, buffer, &size));
    TEST_INFO_END();
    cout << endl;
}

TEST_F(InterfaceFixture, DeviceInfoLastError) {

    INFO_DATATYPE infoType = INFO_DATATYPE_UNKNOWN;
    char buffer[100];
    size_t bufferSize = sizeof(buffer);

    EXPECT_EQ(GC_ERR_NOT_AVAILABLE, IFGetDeviceInfo(hIface, "udp://0.0.0.0", DEVICE_INFO_USER_DEFINED_NAME, &infoType,
        buffer, &bufferSize));

    bufferSize = sizeof(buffer);
    GC_ERROR lastError;
    EXPECT_EQ(GC_ERR_SUCCESS, GCGetLastError(&lastError, buffer, &bufferSize));
    EXPECT_EQ(GC_ERR_NOT_AVAILABLE, lastError);
}
