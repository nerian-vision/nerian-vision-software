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

#ifndef VISIONTRANSFER_LIBRARYINFO_H
#define VISIONTRANSFER_LIBRARYINFO_H

#include <visiontransfer/common.h>

namespace visiontransfer {

/**
 * \brief Information about the current visiontransfer (API) library.
 */
class VT_EXPORT LibraryInfo {
public:
    /**
     * \brief Return the version string for the visiontransfer library.
     *
     * \return The dotted version string as a static const char*
     *  (which must not be freed).
     */
    static const char* getLibraryVersionString();

    /**
     * \brief Returns the major version of visiontransfer library
     */
    static int getLibraryVersionMajor();

    /**
     * \brief Returns the minor version of visiontransfer library
     */
    static int getLibraryVersionMinor();

    /**
     * \brief Returns the patch version of visiontransfer library
     */
    static int getLibraryVersionPatch();
};

} // namespace


#endif


