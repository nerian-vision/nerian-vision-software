#include <genicam/gentl.h>
#include <gtest/gtest.h>
#include <iostream>
#include "test-common.h"

using namespace std;
using namespace GenTL;

TEST(LibraryTest, Initialization) {
    ASSERT_EQ(GC_ERR_SUCCESS, GCInitLib());
    ASSERT_EQ(GC_ERR_SUCCESS, GCCloseLib());
}

TEST(LibraryTest, LibInfo) {
    EXPECT_EQ(GC_ERR_SUCCESS, GCInitLib());

    cout << endl << "GCGetInfo():" << endl;
    TEST_INFO_BEGIN(TL_INFO_ID, TL_INFO_GENTL_VER_MINOR);
    TEST_INFO_TYPE(TL_INFO_ID, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(TL_INFO_VENDOR, INFO_DATATYPE_STRING)
    TEST_INFO_TYPE(TL_INFO_MODEL, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(TL_INFO_VERSION, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(TL_INFO_TLTYPE, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(TL_INFO_NAME, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(TL_INFO_PATHNAME, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(TL_INFO_DISPLAYNAME, INFO_DATATYPE_STRING);
    TEST_INFO_TYPE(TL_INFO_CHAR_ENCODING, INFO_DATATYPE_INT32);
    TEST_INFO_TYPE(TL_INFO_GENTL_VER_MAJOR, INFO_DATATYPE_UINT32);
    TEST_INFO_TYPE(TL_INFO_GENTL_VER_MINOR, INFO_DATATYPE_UINT32);

    TEST_INFO_CALL(GCGetInfo(command, &receivedType, buffer, &size));
    TEST_INFO_END();
    cout << endl;

    EXPECT_EQ(GC_ERR_SUCCESS, GCCloseLib());
}

TEST(LibraryTest, LastError) {
    ASSERT_EQ(GC_ERR_SUCCESS, GCInitLib());
    GC_ERROR error;
    char str[100];
    size_t strSize = sizeof(str);

    EXPECT_EQ(GC_ERR_SUCCESS, GCGetLastError(&error, str, &strSize));
    EXPECT_EQ(GC_ERR_SUCCESS, error);

    std::cout << "Success error string: " << str << endl;

    INFO_DATATYPE type;
    strSize = sizeof(str);
    EXPECT_EQ(GC_ERR_NOT_IMPLEMENTED, GCGetInfo(1000, &type, nullptr, &strSize));

    EXPECT_EQ(GC_ERR_SUCCESS, GCGetLastError(&error, str, &strSize));
    EXPECT_EQ(GC_ERR_NOT_IMPLEMENTED, error);

    EXPECT_EQ(GC_ERR_SUCCESS, GCCloseLib());
}
