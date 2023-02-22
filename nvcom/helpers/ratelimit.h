/*******************************************************************************
 * Copyright (c) 2021 Nerian Vision GmbH
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

#ifndef NVSHARED_RATELIMIT_H
#define NVSHARED_RATELIMIT_H

#include <chrono>

// This class is used for limiting a processing rate
// (e.g. frame rate) by sleeping if processing happens
// too fast
class RateLimit {
public:
    RateLimit(double rate = -1);

    // Notifies that processing has moved on to the next
    // data item. If neccessary, a sleep happens here.
    void next();

private:
    double rate;

    // Error term used to prevent error accumulation
    double errorTerm;
    std::chrono::steady_clock::time_point lastTime;
};

#endif
