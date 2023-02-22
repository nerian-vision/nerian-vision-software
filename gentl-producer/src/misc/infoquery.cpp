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

#include "misc/infoquery.h"
#include <cstring>

namespace GenTL {

InfoQuery::InfoQuery(INFO_DATATYPE* piType, void* pBuffer, size_t* piSize)
    :piType(piType), pBuffer(pBuffer), piSize(piSize), result(GC_ERR_NOT_IMPLEMENTED) {
}

void InfoQuery::setGeneric(const void* value, size_t size, INFO_DATATYPE type) {
    result = GC_ERR_SUCCESS;

    // First check input parameters
    if(piSize == nullptr) {
        result = GC_ERR_INVALID_PARAMETER;
        return;
    } else if(pBuffer != nullptr && *piSize < size) {
        size = *piSize;
        result = GC_ERR_BUFFER_TOO_SMALL;
    }

    // Copy the payload
    if(pBuffer != nullptr) {
        std::memcpy(pBuffer, value, size);
    }

    // Copy type information
    if(piType != nullptr) {
        *piType = type;
    }

    // Copy size information
    *piSize = size;
}

void InfoQuery::setString(std::string strValue) {
    setGeneric(strValue.c_str(), strValue.length() + 1, INFO_DATATYPE_STRING);
}

void InfoQuery::setBool(bool val) {
    unsigned char val8 = static_cast<unsigned char>(val);
    setGeneric(&val8, sizeof(val8), INFO_DATATYPE_BOOL8);
}

void InfoQuery::setInt(int intValue) {
    setGeneric(&intValue, sizeof(intValue), INFO_DATATYPE_INT32);
}

void InfoQuery::setUInt(unsigned int intValue) {
    setGeneric(&intValue, sizeof(intValue), INFO_DATATYPE_UINT32);
}

void InfoQuery::setUInt64(uint64_t intValue) {
    setGeneric(&intValue, sizeof(intValue), INFO_DATATYPE_UINT64);
}

void InfoQuery::setDouble(double doubleValue) {
    setGeneric(&doubleValue, sizeof(doubleValue), INFO_DATATYPE_FLOAT64);
}

void InfoQuery::setSizeT(size_t sizeValue) {
    setGeneric(&sizeValue, sizeof(sizeValue), INFO_DATATYPE_SIZET);
}

void InfoQuery::setPtr(void* ptrValue) {
    setGeneric(&ptrValue, sizeof(ptrValue), INFO_DATATYPE_PTR);
}

GC_ERROR InfoQuery::query() {
    return result;
}

}
