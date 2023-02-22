#include <genicam/gentl.h>
#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <functional>
#include "test-common.h"

using namespace std;
using namespace GenTL;

class PortFixture: public ::testing::Test {
public:
   PortFixture(): hSystem(nullptr), hIface(nullptr), hDevice(nullptr) {
   }

   ~PortFixture() {
   }

   virtual void SetUp( ) {
        ASSERT_EQ(GC_ERR_SUCCESS, GCInitLib());
        ASSERT_EQ(GC_ERR_SUCCESS, TLOpen(&hSystem));
        ASSERT_EQ(GC_ERR_SUCCESS, TLOpenInterface(hSystem, "eth", &hIface));
        EXPECT_EQ(GC_ERR_SUCCESS, IFUpdateDeviceList(hIface, nullptr, 1000));

        // open first device
        char buffer[100];
        size_t size = sizeof(buffer);
        EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, 0, buffer, &size));
        ASSERT_EQ(GC_ERR_SUCCESS, IFOpenDevice(hIface, buffer, DEVICE_ACCESS_READONLY, &hDevice));

        ASSERT_EQ(GC_ERR_SUCCESS, DevGetPort(hDevice, &hPort));
   }

   virtual void TearDown( ) {
        ASSERT_EQ(GC_ERR_SUCCESS, DevClose(hDevice));
        ASSERT_EQ(GC_ERR_SUCCESS, IFClose(hIface));
        ASSERT_EQ(GC_ERR_SUCCESS, TLClose(hSystem));
        ASSERT_EQ(GC_ERR_SUCCESS, GCCloseLib());
   }

protected:
    TL_HANDLE hSystem;
    IF_HANDLE hIface;
    DEV_HANDLE hDevice;
    PORT_HANDLE hPort;

    void comparePortWithFunction(PORT_HANDLE port, const std::vector<int32_t>& commands,
            std::function<GC_ERROR(void*, int32_t, INFO_DATATYPE* piType, void* pBuffer, size_t* piSize)> func) {
        for(unsigned int i=0; i<commands.size(); i++) {
            char buffer1[1000], buffer2[1000];

            size_t size = sizeof(buffer1);
            uint64_t address = 0xE0000000 + 0x1000*commands[i];
            memset(buffer1, 0, sizeof(buffer1));
            EXPECT_EQ(GC_ERR_SUCCESS, GCReadPort(port, address, buffer1, &size));

            size = sizeof(buffer2);
            memset(buffer2, 0, sizeof(buffer2));
            INFO_DATATYPE type;
            EXPECT_EQ(GC_ERR_SUCCESS, func(port, commands[i], &type, buffer2, &size));

            EXPECT_EQ(string(buffer1), string(buffer2));
        }
    }

    void testSelector(PORT_HANDLE port, uint32_t infoCommand, const char* val0, const char* val1) {
        // Make sure selector is set to 0
        unsigned int selector = 100;
        size_t size = sizeof(selector);
        EXPECT_EQ(GC_ERR_SUCCESS, GCReadPort(port, 0xD0000000, &selector, &size));
        EXPECT_EQ(0, selector);

        uint64_t idAddress = 0xC0000000 + 0x1000*infoCommand;
        char buffer[100];
        size = sizeof(buffer);
        EXPECT_EQ(GC_ERR_SUCCESS, GCReadPort(port, idAddress, buffer, &size));
        EXPECT_EQ(std::string(val0), string(buffer));

        // Set selector to 1 and compare again
        selector = 1;
        size = sizeof(selector);
        EXPECT_EQ(GC_ERR_SUCCESS, GCWritePort(port, 0xD0000000, &selector, &size));
        EXPECT_EQ(GC_ERR_SUCCESS, GCReadPort(port, 0xD0000000, &selector, &size));
        EXPECT_EQ(1, selector);

        size = sizeof(buffer);
        EXPECT_EQ(GC_ERR_SUCCESS, GCReadPort(port, idAddress, buffer, &size));
        EXPECT_EQ(std::string(val1), string(buffer));
    }
};

TEST_F(PortFixture, PortInfo) {
    cout << endl << "GCGetPortInfo():" << endl;

    TEST_INFO_BEGIN(PORT_INFO_ID, PORT_INFO_PORTNAME);
    TEST_INFO_TYPE(PORT_INFO_ID, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(PORT_INFO_VENDOR, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(PORT_INFO_MODEL, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(PORT_INFO_TLTYPE, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(PORT_INFO_MODULE, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(PORT_INFO_LITTLE_ENDIAN, INFO_DATATYPE_BOOL8);
    TEST_INFO_TYPE(PORT_INFO_BIG_ENDIAN, INFO_DATATYPE_BOOL8);
    TEST_INFO_TYPE(PORT_INFO_ACCESS_READ, INFO_DATATYPE_BOOL8);
    TEST_INFO_TYPE(PORT_INFO_ACCESS_WRITE, INFO_DATATYPE_BOOL8);
    TEST_INFO_TYPE(PORT_INFO_ACCESS_NA, INFO_DATATYPE_BOOL8);
    TEST_INFO_TYPE(PORT_INFO_ACCESS_NI, INFO_DATATYPE_BOOL8);
    TEST_INFO_TYPE(PORT_INFO_VERSION, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(PORT_INFO_PORTNAME, INFO_DATATYPE_STRING);

    TEST_INFO_CALL(GCGetPortInfo(hPort, command, &receivedType, buffer, &size));
    TEST_INFO_END();
    cout << endl;
}

TEST_F(PortFixture, PortURL) {
    uint32_t numURLs;
    EXPECT_EQ(GC_ERR_SUCCESS, GCGetNumPortURLs(hPort, &numURLs));
    EXPECT_EQ(numURLs, 1);

    char buffer[100];
    size_t size = sizeof(buffer);
    EXPECT_EQ(GC_ERR_SUCCESS, GCGetPortURL(hPort, buffer, &size));
    cout << "Port URL: " << buffer << endl;
}

TEST_F(PortFixture, PortURLInfo) {
    cout << endl << "GCGetPortURLInfo():" << endl;

    TEST_INFO_BEGIN(URL_INFO_URL, URL_INFO_FILENAME);
    TEST_INFO_TYPE(URL_INFO_URL, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(URL_INFO_SCHEMA_VER_MAJOR, INFO_DATATYPE_INT32);
    TEST_INFO_TYPE(URL_INFO_SCHEMA_VER_MINOR, INFO_DATATYPE_INT32);
    TEST_INFO_TYPE(URL_INFO_FILE_VER_MAJOR, INFO_DATATYPE_INT32);
    TEST_INFO_TYPE(URL_INFO_FILE_VER_MINOR, INFO_DATATYPE_INT32);
    TEST_INFO_TYPE(URL_INFO_FILE_VER_SUBMINOR, INFO_DATATYPE_INT32);
    TEST_INFO_SKIP(URL_INFO_FILE_SHA1_HASH)
    TEST_INFO_TYPE(URL_INFO_FILE_REGISTER_ADDRESS, INFO_DATATYPE_UINT64);
    TEST_INFO_TYPE(URL_INFO_FILE_SIZE, INFO_DATATYPE_UINT64);
    TEST_INFO_TYPE(URL_INFO_SCHEME, INFO_DATATYPE_INT32);
    TEST_INFO_TYPE(URL_INFO_FILENAME, INFO_DATATYPE_STRING);

    TEST_INFO_CALL(GCGetPortURLInfo(hPort, 0, command, &receivedType, buffer, &size));
    TEST_INFO_END();
    cout << endl;
}

TEST_F(PortFixture, Read) {
    char buffer[10000];
    memset(buffer, 0, sizeof(buffer));
    size_t size = sizeof(buffer);
    EXPECT_EQ(GC_ERR_SUCCESS, GCReadPort(hPort, 0xF0000000, buffer, &size));
    EXPECT_EQ(string(buffer).substr(0, 5), string("<?xml"));
    cout << endl << "XML-File: " << size << " bytes" << endl;
}

TEST_F(PortFixture, ReadStacked) {
    char buffer1[10000], buffer2[10000];
    memset(buffer1, 0, sizeof(buffer1));
    memset(buffer2, 0, sizeof(buffer1));

    PORT_REGISTER_STACK_ENTRY stack[2];
    stack[0].Address = 0xF0000000;
    stack[0].pBuffer = buffer1;
    stack[0].Size = sizeof(buffer1);

    stack[1].Address = 0xF0000000;
    stack[1].pBuffer = buffer2;
    stack[1].Size = sizeof(buffer2);

    size_t numEntries = 2;
    EXPECT_EQ(GC_ERR_SUCCESS, GCReadPortStacked(hPort, stack, &numEntries));
    EXPECT_EQ(2, numEntries);
    EXPECT_EQ(0, memcmp(buffer1, buffer2, stack[0].Size));
    EXPECT_EQ(string(buffer1).substr(0, 5), string("<?xml"));
}

TEST_F(PortFixture, ReadSystemPortFeature)  {
    comparePortWithFunction(hSystem, {TL_INFO_ID, TL_INFO_VENDOR, TL_INFO_PATHNAME}, &TLGetInfo);
}

TEST_F(PortFixture, SystemPortSelector)  {
    unsigned int selector = 100;
    size_t size = sizeof(selector);
    EXPECT_EQ(GC_ERR_SUCCESS, GCReadPort(hSystem, 0xD0000000, &selector, &size));
    EXPECT_EQ(0, selector);

    selector = 2;
    EXPECT_EQ(GC_ERR_INVALID_INDEX, GCWritePort(hSystem, 0xD0000000, &selector, &size));
    EXPECT_EQ(GC_ERR_SUCCESS, GCReadPort(hSystem, 0xD0000000, &selector, &size));
    EXPECT_EQ(0, selector);
}

TEST_F(PortFixture, ReadSystemPortChildFeature)  {
    char buffer[100];
    size_t size = sizeof(buffer);
    memset(buffer, 0, sizeof(buffer));
    EXPECT_EQ(GC_ERR_SUCCESS, GCReadPort(hSystem, 0xC0000000, buffer, &size));
    EXPECT_EQ("eth", string(buffer));
}

TEST_F(PortFixture, ReadInterfacePortFeature)  {
    comparePortWithFunction(hIface, {INTERFACE_INFO_ID, INTERFACE_INFO_DISPLAYNAME}, &IFGetInfo);
}

TEST_F(PortFixture, InterfacePortSelector)  {
    // open first device
    char buffer1[100], buffer2[100];
    size_t size = sizeof(buffer1);
    EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, 0, buffer1, &size));
    size = sizeof(buffer2);
    EXPECT_EQ(GC_ERR_SUCCESS, IFGetDeviceID(hIface, 1, buffer2, &size));

    testSelector(hIface, DEVICE_INFO_ID, buffer1, buffer2);

    uint32_t selectorMax;
    size = sizeof(selectorMax);
    EXPECT_EQ(GC_ERR_SUCCESS, GCReadPort(hIface, 0xE03E8000, &selectorMax, &size));
    EXPECT_EQ(selectorMax, 4);
}

TEST_F(PortFixture, ReadDevicePortFeature)  {
    comparePortWithFunction(hDevice, {DEVICE_INFO_ID, DEVICE_INFO_MODEL, DEVICE_INFO_DISPLAYNAME}, &DevGetInfo);
}

TEST_F(PortFixture, DevicePortSelector)  {
    for(uint32_t selector = 0; selector < 3; selector++) {
        uint32_t value = 0;
        size_t size = sizeof(selector);
        EXPECT_EQ(GC_ERR_SUCCESS, GCWritePort(hDevice, 0xD0000000, &selector, &size));
        EXPECT_EQ(GC_ERR_SUCCESS, GCReadPort(hDevice, 0xD0000000, &value, &size));
        EXPECT_EQ(value, selector);

        value = 0;
        EXPECT_EQ(GC_ERR_SUCCESS, GCReadPort(hDevice, 0xC0004000, &value, &size));
        EXPECT_EQ(7, value); // All parts enabled

        value = 0;
        EXPECT_EQ(GC_ERR_SUCCESS, GCReadPort(hDevice, 0xC0005000, &value, &size));
        EXPECT_EQ(selector, value);
    }
}


TEST_F(PortFixture, ReadStreamPortFeature)  {
    DS_HANDLE hStream = nullptr;
    ASSERT_EQ(GC_ERR_SUCCESS, DevOpenDataStream(hDevice, "default", &hStream));
    comparePortWithFunction(hStream, {STREAM_INFO_ID}, &DSGetInfo);
    ASSERT_EQ(GC_ERR_SUCCESS, DSClose(hStream));
}

TEST_F(PortFixture, PartialRead)  {
    char buffer1[100];
    memset(buffer1, 0, sizeof(buffer1));
    size_t size = sizeof(buffer1);

    EXPECT_EQ(GC_ERR_SUCCESS, GCReadPort(hPort, 0xE0000000, buffer1, &size));

    char buffer2[100];
    for(unsigned int i=0; i<sizeof(buffer2); i++) {
        size = 1;
        uint64_t address = 0xE0000000 + i;
        EXPECT_EQ(GC_ERR_SUCCESS, GCReadPort(hPort, address, &buffer2[i], &size));
    }

    EXPECT_EQ(string(buffer1), string(buffer2));
}
