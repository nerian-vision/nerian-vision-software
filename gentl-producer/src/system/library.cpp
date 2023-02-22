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

#ifdef _WIN32
#include <windows.h>
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#else
#include <libgen.h>
#include <dlfcn.h>
#endif

#include <genicam/gentl.h>
#include "system/library.h"
#include "misc/infoquery.h"


namespace GenTL {

GC_ERROR Library::initLib() {
    return GC_ERR_SUCCESS;
}

GC_ERROR Library::closeLib() {
    return GC_ERR_SUCCESS;
}

GC_ERROR Library::getInfo(TL_INFO_CMD iInfoCmd, INFO_DATATYPE* piType, void* pBuffer, size_t* piSize) {
    InfoQuery info(piType, pBuffer, piSize);

#ifdef _WIN32
    char dllPath[100];
    GetModuleFileName((HINSTANCE)&__ImageBase, dllPath, sizeof(dllPath));
#else
    Dl_info dlInfo;
    dladdr((void *)GCGetInfo, &dlInfo);
#endif

    switch(iInfoCmd) {
        case TL_INFO_ID:
            info.setString("nerian-gentl");
            break;
        case TL_INFO_VENDOR:
            info.setString("Nerian Vision GmbH");
            break;
        case TL_INFO_MODEL:
            info.setString("nerian");
            break;
        case TL_INFO_VERSION:
            info.setString("1.0");
            break;
        case TL_INFO_TLTYPE:
            info.setString("Ethernet");
            break;
        case TL_INFO_DISPLAYNAME:
            info.setString("Nerian");
            break;
        case TL_INFO_CHAR_ENCODING:
            info.setInt(TL_CHAR_ENCODING_ASCII);
            break;
        case TL_INFO_GENTL_VER_MAJOR:
            info.setUInt(1);
            break;
        case TL_INFO_GENTL_VER_MINOR:
            info.setUInt(6);
            break;
#ifdef _WIN32
        case TL_INFO_PATHNAME:
            info.setString(dllPath);
            break;
        case TL_INFO_NAME: {
            char filename[100];
            char ext[100];
            _splitpath_s(dllPath, nullptr, 0, nullptr, 0, filename, sizeof(filename), ext, sizeof(ext));
            info.setString(std::string(filename) + ext);
            break;
        }
#else
        case TL_INFO_PATHNAME:
            info.setString(dlInfo.dli_fname);
            break;
        case TL_INFO_NAME:
            info.setString(basename(const_cast<char*>(dlInfo.dli_fname)));
            break;
#endif
        default:
            ; // Nothing to do
    }

    return info.query();
}

std::string Library::errorToString(GC_ERROR error) {
    switch(error) {
        case GC_ERR_SUCCESS:
            return "Operation was successful";
        case GC_ERR_ERROR:
            return "Unspecified runtime error";
        case GC_ERR_NOT_INITIALIZED:
            return "Module or resource not initialized";
        case GC_ERR_RESOURCE_IN_USE:
            return "Requested operation not implemented";
        case GC_ERR_NOT_IMPLEMENTED:
            return "Requested resource is already in use";
        case GC_ERR_ACCESS_DENIED:
            return "Requested operation is not allowed";
        case GC_ERR_INVALID_HANDLE:
            return "Given handle does not support the operation";
        case GC_ERR_INVALID_ID:
            return "ID could not be connected to a resource";
        case GC_ERR_NO_DATA:
            return "The function has no data to work on";
        case GC_ERR_INVALID_PARAMETER:
            return "One of the parameter given was not valid or out of range";
        case GC_ERR_IO:
            return "Communication error has occurred";
        case GC_ERR_TIMEOUT:
            return "Timeout time expired before operation could be completed";
        case GC_ERR_ABORT:
            return "Operation aborted before completion";
        case GC_ERR_INVALID_BUFFER:
            return "Not enough buffers to start acquisition";
        case GC_ERR_NOT_AVAILABLE:
            return "Resource or information is not available";
        case GC_ERR_INVALID_ADDRESS:
            return "Given address is out of range or invalid";
        case GC_ERR_BUFFER_TOO_SMALL:
            return "Provided buffer is too small to receive expected data";
        case GC_ERR_INVALID_INDEX:
            return "Provided index is out of bounds";
        case GC_ERR_PARSING_CHUNK_DATA:
            return "Error parsing a chunk data buffer";
        case GC_ERR_INVALID_VALUE:
            return "A register write function was trying to write an invalid value";
        case GC_ERR_RESOURCE_EXHAUSTED:
            return "Requested resource is exhausted";
        case GC_ERR_OUT_OF_MEMORY:
            return "System has ran out of memory";
        case GC_ERR_BUSY:
            return "Module / entity is busy";
        default:
            return "Unknown error";
    }
}

GC_ERROR Library::getLastError(GC_ERROR* piErrorCode, char* sErrorText, size_t* piSize, GC_ERROR lastError) {
    if(piErrorCode == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    InfoQuery info(nullptr, sErrorText, piSize);
    info.setString(errorToString(lastError));

    *piErrorCode = lastError;
    return info.query();
}

}
