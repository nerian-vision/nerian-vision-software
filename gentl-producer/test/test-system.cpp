#include <genicam/gentl.h>
#include <gtest/gtest.h>
#include <iostream>
#include "test-common.h"

using namespace std;
using namespace GenTL;

TEST(SystemTest, SimpleFunctions) {
    TL_HANDLE hSystem = nullptr;
    ASSERT_EQ(GC_ERR_SUCCESS, GCInitLib());
    ASSERT_EQ(GC_ERR_SUCCESS, TLOpen(&hSystem));

    EXPECT_EQ(GC_ERR_SUCCESS, TLUpdateInterfaceList(hSystem, nullptr, 1000));

    uint32_t numIf = 0;
    EXPECT_EQ(GC_ERR_SUCCESS, TLGetNumInterfaces(hSystem, &numIf));
    EXPECT_EQ(numIf, 1);

    char buffer[100];
    size_t size = sizeof(buffer);
    EXPECT_EQ(GC_ERR_SUCCESS, TLGetInterfaceID(hSystem, 0, buffer, &size));
    EXPECT_EQ(string(buffer), string("eth"));

    ASSERT_EQ(GC_ERR_SUCCESS, TLClose(hSystem));
    ASSERT_EQ(GC_ERR_SUCCESS, GCCloseLib());
}

TEST(SystemTest, InterfaceInfo) {
    TL_HANDLE hSystem = nullptr;
    ASSERT_EQ(GC_ERR_SUCCESS, GCInitLib());
    ASSERT_EQ(GC_ERR_SUCCESS, TLOpen(&hSystem));

    cout << endl << "TLGetInterfaceInfo():" << endl;
    TEST_INFO_BEGIN(INTERFACE_INFO_ID, INTERFACE_INFO_TLTYPE);

    TEST_INFO_TYPE(INTERFACE_INFO_ID, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(INTERFACE_INFO_DISPLAYNAME, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(INTERFACE_INFO_TLTYPE, INFO_DATATYPE_STRING);

    TEST_INFO_CALL(TLGetInterfaceInfo(hSystem, "eth", command, &receivedType, buffer, &size));
    TEST_INFO_END();
    cout << endl;

    ASSERT_EQ(GC_ERR_SUCCESS, TLClose(hSystem));
    ASSERT_EQ(GC_ERR_SUCCESS, GCCloseLib());
}
