/*******************************************************************************
 * Copyright (c) 2023 Allied Vision Technologies GmbH
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

#include "ratelimit.h"
#include <thread>

using namespace std::chrono;

RateLimit::RateLimit(double rate): rate(rate), errorTerm(0),
    lastTime(steady_clock::now()) {
}

void RateLimit::next() {
    double periodUS = 1e6 / rate;

    if(rate > 0) {
        double elapsedUSBeforeSleep = duration_cast<microseconds>(
            steady_clock::now() - lastTime).count() + errorTerm;

        // If the waiting time was too short then we need to sleep
        if(elapsedUSBeforeSleep < periodUS) {
            microseconds duration(static_cast<unsigned int>(periodUS - elapsedUSBeforeSleep));
            std::this_thread::sleep_for(duration);
        }

        double elapsedUSAfterSleep = duration_cast<microseconds>(
            steady_clock::now() - lastTime).count() + errorTerm;

        lastTime = steady_clock::now();

        // Update error term to prevent error accumulation
        errorTerm = elapsedUSAfterSleep - periodUS;
        if(errorTerm > periodUS) {
            errorTerm = periodUS;
        }
    }
}
