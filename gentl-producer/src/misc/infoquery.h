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

#ifndef INFOQUERY_H
#define INFOQUERY_H

#include "misc/common.h"

#include <genicam/gentl.h>
#include <vector>
#include <list>
#include <string>

namespace GenTL {

/*
 * This is a helper class for GenTL functions that allow a query for
 * module information. The class provides functionality for copying
 * values of different data types to the output buffer in a GenTL
 * compatible way.
 */
class InfoQuery {
public:
    // Instantiates a new object which will write its output to the given
    // pointers. If a datatype output is not required, then piType can be
    // a null pointer.
    InfoQuery(GenTL::INFO_DATATYPE* piType, void* pBuffer, size_t* piSize);

    // Methods for outputting different data types.
    void setString(std::string strValue);
    void setBool(bool val);
    void setInt(int intValue);
    void setUInt(unsigned int intValue);
    void setUInt64(uint64_t intValue);
    void setDouble(double doubleValue);
    void setSizeT(size_t sizeValue);
    void setPtr(void* ptrValue);

    // Finishes the information query and deliveres the return code.
    GenTL::GC_ERROR query();

private:
    GenTL::INFO_DATATYPE* piType;
    void* pBuffer;
    size_t* piSize;
    GenTL::GC_ERROR result;

    void setGeneric(const void* value, size_t size, GenTL::INFO_DATATYPE type);
};

}

#endif
