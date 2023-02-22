#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <string>
#include <vector>

/*
 * Macros for testing info commands
 */

static std::vector<std::string> typeStrings = {
    "INFO_DATATYPE_UNKNOWN",
    "INFO_DATATYPE_STRING",
    "INFO_DATATYPE_STRINGLIST",
    "INFO_DATATYPE_INT16",
    "INFO_DATATYPE_UINT16",
    "INFO_DATATYPE_INT32",
    "INFO_DATATYPE_UINT32",
    "INFO_DATATYPE_INT64",
    "INFO_DATATYPE_UINT64",
    "INFO_DATATYPE_FLOAT64",
    "INFO_DATATYPE_PTR",
    "INFO_DATATYPE_BOOL8",
    "INFO_DATATYPE_SIZET",
    "INFO_DATATYPE_BUFFER",
    "INFO_DATATYPE_PTRDIFF"
};


#define TEST_INFO_BEGIN(minCommand, maxCommand) {\
    unsigned char* buffer = new unsigned char[1000];\
    for(int command = minCommand; command <= maxCommand; command++) { \
        INFO_DATATYPE expectedType = INFO_DATATYPE_UNKNOWN; \
        std::string enumName; \
        switch(command) {

#define TEST_INFO_SKIP(command) \
        case command:\
            continue;

#define TEST_INFO_TYPE(command, type) \
        case command:\
            enumName = #command;\
            expectedType = type;\
            break;

#define TEST_INFO_CALL(f) \
        default:\
            ASSERT_TRUE(false);\
        }\
        size_t size=1000;\
        INFO_DATATYPE receivedType; \
        EXPECT_EQ(GC_ERR_SUCCESS, f);\
        ASSERT_TRUE(size_t(receivedType) < typeStrings.size()); \
        std::cout << enumName << ": " << std::endl \
            << "\ttype:\t" << typeStrings[receivedType] << std::endl\
            << "\tvalue:\t";\
        EXPECT_EQ(receivedType, expectedType);\
        switch(receivedType) {\
            case INFO_DATATYPE_STRING:\
                std::cout << reinterpret_cast<char*>(buffer);\
                break;\
            case INFO_DATATYPE_INT16:\
                std::cout << *reinterpret_cast<int16_t*>(buffer);\
                break;\
            case INFO_DATATYPE_UINT16:\
                std::cout << *reinterpret_cast<uint16_t*>(buffer);\
                break;\
            case INFO_DATATYPE_INT32:\
                std::cout << *reinterpret_cast<int32_t*>(buffer);\
                break;\
            case INFO_DATATYPE_UINT32:\
                std::cout << *reinterpret_cast<uint32_t*>(buffer);\
                break;\
            case INFO_DATATYPE_INT64:\
                std::cout << *reinterpret_cast<int64_t*>(buffer);\
                break;\
            case INFO_DATATYPE_UINT64:\
                std::cout << *reinterpret_cast<uint64_t*>(buffer);\
                break;\
            case INFO_DATATYPE_FLOAT64:\
                std::cout << *reinterpret_cast<double*>(buffer);\
                break;\
            case INFO_DATATYPE_PTR:\
                std::cout << *reinterpret_cast<void**>(buffer);\
                break;\
            case INFO_DATATYPE_BOOL8:\
                std::cout << (int)*reinterpret_cast<bool8_t*>(buffer);\
                break;\
            case INFO_DATATYPE_SIZET:\
                std::cout << *reinterpret_cast<size_t*>(buffer);\
                break;\
        }\
        std::cout << std::endl;

#define TEST_INFO_END() \
            }\
            delete []buffer;\
        }


#endif
