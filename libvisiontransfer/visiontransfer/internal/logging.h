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

#ifndef VISIONTRANSFER_LOGGING_H
#define VISIONTRANSFER_LOGGING_H

#include <visiontransfer/common.h>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <mutex>

namespace visiontransfer {
namespace internal {

class Logging {
public:
    enum LogLevel {
        LL_DEBUG,
        LL_INFO,
        LL_WARN,
        LL_ERROR
    };
    struct LoggingSink {
        std::ostream* stream;
        LogLevel minLevel;
        LogLevel maxLevel;
        std::string channelFilter;
    };
    static Logging& getInstance();
    /**
     * \brief Returns whether the logging interface was enabled in the library at build time.
     *  If not, all log emissions have been elided as well, and all other methods here are no-op.
     */
    bool isLoggingEnabled();
    /**
     * \brief Returns whether any sinks have been installed by the user. This is used inside the
     *  library to quickly skip any unnnecessary lookups and serialization.
     */
    bool hasLoggingSinks();
    /**
     * \brief Add a logging sink (receiver) ostream, optionally filtered to a channel and/or log level.
     *  The user must take care to ensure the lifetime of the ostream object.
     *
     *  Note: This will return false if the library was compiled without logging support.
     */
    bool addLoggingSink(std::ostream* os, LogLevel minLevel=LogLevel::LL_DEBUG, LogLevel maxLevel=LogLevel::LL_ERROR, const std::string& channelFilter="");
    /**
     * \brief Remove all logging sinks that involve the specified ostream.
     *
     *  Note: This will return false if the library was compiled without logging support.
     */
    bool removeAllLoggingSinksFor(std::ostream* os);
    /**
     * \brief Emits a log message. Note: this is primarily used in the library itself.
     *
     *  Note: This will return false if the library was compiled without logging support.
     */
    bool emit(const std::string& channel, LogLevel logLevel, const std::string& message);
private:
    std::vector<LoggingSink> sinks;
    std::mutex mutex;

    // Only accessible via the singleton returned by getInstance()
    Logging();
    Logging(const Logging&) = delete;
    void operator=(const Logging&) = delete;
};

// When building the library, the setting of VISIONTRANSFER_LOGGING_ENABLED
// determines whether or not debug calls are emitted in the library at all.
#ifdef VISIONTRANSFER_LOGGING_ENABLED
#define VISIONTRANSFER_LOG(channel, level, what) { auto& loginst = Logging::getInstance(); if (loginst.hasLoggingSinks()) { std::ostringstream oss; oss << "[visiontransfer] " << channel << ": " << what << '\n'; loginst.emit(channel, level, oss.str()); } }
#else
#define VISIONTRANSFER_LOG(channel, level, what) ;
#endif


}} // namespace

#endif


