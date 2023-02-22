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

#ifndef NERIAN_COMMON_H
#define NERIAN_COMMON_H

#if __GNUC__ == 4 && __GNUC_MINOR__ < 9
// This is a very ugly workaround for GCC bug 54562. If omitted,
// timeouts are broken.
#include <bits/c++config.h>
#undef _GLIBCXX_USE_CLOCK_MONOTONIC
#endif

// Conditional compilation flags
//#define ENABLE_DEBUGGING
//#define DELIVER_TEST_DATA

#endif
