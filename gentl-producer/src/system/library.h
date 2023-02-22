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

#ifndef NERIAN_LIBRARY_H
#define NERIAN_LIBRARY_H

#include "misc/common.h"
#include <genicam/gentl.h>
#include <string>

namespace GenTL {

/*
 * Class encapsulating all global GenTL library functions
 */
class Library {
public:
    // Methods that match functions form the GenTL interface
    static GC_ERROR initLib();
    static GC_ERROR closeLib();
    static GC_ERROR getInfo(TL_INFO_CMD iInfoCmd, INFO_DATATYPE* piType, void* pBuffer, size_t* piSize);
    static GC_ERROR getLastError(GC_ERROR* piErrorCode, char* sErrorText, size_t* piSize, GC_ERROR lastError);

    // Methods that are used internally
    static std::string errorToString(GC_ERROR error);
};

}

#endif
