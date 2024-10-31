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

#include "visiontransfer/internal/logging.h"

using namespace visiontransfer;
using namespace visiontransfer::internal;

namespace visiontransfer {
namespace internal {

Logging& Logging::getInstance() {
    static Logging loggingSingleton;
    return loggingSingleton;
}

bool Logging::isLoggingEnabled() {
#ifdef VISIONTRANSFER_LOGGING_ENABLED
    return true;
#else
    return false;
#endif
}

bool Logging::hasLoggingSinks() {
#ifdef VISIONTRANSFER_LOGGING_ENABLED
    return sinks.size() > 0;
#else
    return false;
#endif
}

bool Logging::addLoggingSink(std::ostream* os, LogLevel minLevel, LogLevel maxLevel, const std::string& channelFilter) {
#ifdef VISIONTRANSFER_LOGGING_ENABLED
    sinks.push_back({os, minLevel, maxLevel, channelFilter});
    return true;
#else
    return false;
#endif
}

bool Logging::removeAllLoggingSinksFor(std::ostream* os) {
#ifdef VISIONTRANSFER_LOGGING_ENABLED
    std::vector<LoggingSink> newSinks;
    for (auto& s: sinks) {
        if (s.stream != os) {
            newSinks.push_back(s);
        }
    }
    sinks = newSinks;
    return true;
#else
    return false;
#endif
}

bool Logging::emit(const std::string& channel, LogLevel logLevel, const std::string& message) {
#ifdef VISIONTRANSFER_LOGGING_ENABLED
    std::unique_lock<std::mutex> lock(mutex);
    for (auto& s: sinks) {
        if ((logLevel >= s.minLevel && logLevel <=s.maxLevel)
                && (s.channelFilter.size()==0 || s.channelFilter==channel)) {
            (*(s.stream)) << message;
            s.stream->flush();
        }
    }
    return true;
#else
    return false;
#endif
}

}} // namespace

