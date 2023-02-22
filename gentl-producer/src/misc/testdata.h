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

#ifndef NERIAN_TESTDATA_H
#define NERIAN_TESTDATA_H

#include "misc/common.h"

#ifdef DELIVER_TEST_DATA
class TestData {
public:
#ifdef _WIN32
    static __declspec(align(32)) unsigned char leftTestData[];
    static __declspec(align(32)) unsigned short disparityTestData[];
#else
    static unsigned char leftTestData[] __attribute__((aligned(32)));
    static unsigned short disparityTestData[] __attribute__((aligned(32)));
#endif
};
#endif

#endif
