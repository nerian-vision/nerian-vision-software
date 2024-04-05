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

/*******************************************************************************
 * This header file contains include statements and definitions for simplifying
 * cross platform network development
*******************************************************************************/

#ifndef VISIONTRANSFER_TYPES_H
#define VISIONTRANSFER_TYPES_H

//
// Type definitions that are used by several modules
//

namespace visiontransfer {

#if VISIONTRANSFER_CPLUSPLUS_VERSION >= 201103L
/**
  * \brief Reported connection state for various device channel
  *  connection state callbacks (after successful initial connection)
  */
enum class VT_EXPORT ConnectionState {
    DISCONNECTED = 0,  // A connection has been lost
    CONNECTED = 1      // A connection has been re-established
};
#endif

} // namespace

#endif

