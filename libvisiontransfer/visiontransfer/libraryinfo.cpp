/*******************************************************************************
 * Copyright (c) 2024 Allied Vision Technologies GmbH
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

#include <visiontransfer/libraryinfo.h>

#include <sstream>

namespace visiontransfer {

/* static */
const char* LibraryInfo::getLibraryVersionString() {
    static std::string ver("");
    if (ver=="") {
        std::stringstream ss;
        ss << VISIONTRANSFER_MAJOR_VERSION << "." << VISIONTRANSFER_MINOR_VERSION << "." << VISIONTRANSFER_PATCH_VERSION;
        ver = ss.str();
    }
    return ver.c_str();
}

/* static */
int LibraryInfo::getLibraryVersionMajor() {
    return VISIONTRANSFER_MAJOR_VERSION;
}

/* static */
int LibraryInfo::getLibraryVersionMinor() {
    return VISIONTRANSFER_MINOR_VERSION;
}

/* static */
int LibraryInfo::getLibraryVersionPatch() {
    return VISIONTRANSFER_PATCH_VERSION;
}

} // namespace

