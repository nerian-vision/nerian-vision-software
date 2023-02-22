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

#ifndef VISIONTRANSFER_SENSORRINGBUFFER_H
#define VISIONTRANSFER_SENSORRINGBUFFER_H

#include <array>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <tuple>

#include <visiontransfer/sensordata.h>

using namespace visiontransfer;

namespace visiontransfer {
namespace internal {

/**
 *  Thread-safe ring buffer for timestamped generic sensor data.
 *   RecordType needs to implement getTimestamp() in order to perform
 *   comparisons in popBetweenTimes() (= obtain data series in interval).
 *
 *   Maximum capacity of the buffer is RINGBUFFER_SIZE-1.
 *   lostSamples() tallies the number of samples silently lost due
 *   to buffer overruns, and is reset by any of the pop...() methods.
 */
template<typename RecordType, int RINGBUFFER_SIZE>
class SensorDataRingBuffer {
    private:
        int read_horizon, write_position, read_next;
        unsigned long lostSamples;
        std::array<RecordType, RINGBUFFER_SIZE> buffer;
        std::recursive_mutex mutex;
    public:
        constexpr unsigned int ringbufferSize() const { return RINGBUFFER_SIZE; }
        SensorDataRingBuffer(): read_horizon(0), write_position(0), read_next(0), lostSamples(0) { }
        constexpr int capacity() const { return ringbufferSize() - 1; }
        int size() const { return (ringbufferSize() + (write_position - read_next)) % ringbufferSize(); }
        int samplesLost() const { return lostSamples; }
        bool isFull() const { return size()==capacity(); }
        bool isEmpty() const { return write_position==read_next; }

        bool advanceWritePosition() {
            write_position = (write_position + 1) % ringbufferSize();
            if (write_position==read_next) {
                // Ring buffer overrun: advance and increment lost samples count
                read_next = (write_position + 1) % ringbufferSize();
                lostSamples++;
            }
            return lostSamples==0;
        }

        bool pushData(const std::vector<RecordType>& data) {
            // A more efficient implementation could be substituted on demand
            std::unique_lock<std::recursive_mutex> lock(mutex);
            for (auto const& d: data) {
                (void) pushData(d);
            }
            return lostSamples==0;
        }

        bool pushData(const RecordType& data) {
            std::unique_lock<std::recursive_mutex> lock(mutex);
            buffer[write_position] = data;
            return advanceWritePosition();
        }

        bool pushData(RecordType&& data) {
            std::unique_lock<std::recursive_mutex> lock(mutex);
            buffer[write_position] = std::move(data);
            return advanceWritePosition();
        }

        // \brief Pop and return the whole ring buffer contents
        std::vector<RecordType> popAllData() {
            std::unique_lock<std::recursive_mutex> lock(mutex);
            lostSamples = 0;
            if (write_position < read_next) {
                // wrapped
                std::vector<RecordType> v(buffer.begin()+read_next, buffer.end());
                v.reserve(v.size() + write_position);
                std::copy(buffer.begin(), buffer.begin() + write_position, std::back_inserter(v));
                read_next = (write_position) % ringbufferSize();
                return v;
            } else {
                std::vector<RecordType> v(buffer.begin()+read_next, buffer.begin()+write_position);
                read_next = (write_position) % ringbufferSize();
                return v;
            }
        }

        /// \brief Pop and return the data between timestamps (or the whole ring buffer contents if not provided)
        std::vector<RecordType> popBetweenTimes(int fromSec = 0, int fromUSec = 0, int untilSec = 0x7fffFFFFl, int untilUSec = 0x7fffFFFFl) {
            std::unique_lock<std::recursive_mutex> lock(mutex);
            lostSamples = 0;
            int tsSec, tsUSec;
            if (write_position == read_next) return std::vector<RecordType>();
            // Find first relevant sample (matching or exceeding the specified start time)
            buffer[read_next].getTimestamp(tsSec, tsUSec);
            while ((tsSec < fromSec) || ((tsSec == fromSec) && (tsUSec < fromUSec))) {
                read_next = (read_next + 1) % ringbufferSize();
                if (write_position == read_next) return std::vector<RecordType>();
            }
            // Find last relevant sample (not exceeding the specified end time)
            int lastidx = read_next;
            int li;
            buffer[lastidx].getTimestamp(tsSec, tsUSec);
            while ((tsSec < untilSec) || ((tsSec == untilSec) && (tsUSec <= untilUSec))) {
                li = (lastidx + 1) % ringbufferSize();
                lastidx = li;
                if (li == write_position) break;
            }
            if (lastidx < read_next) {
                // Wrapped
                std::vector<RecordType> v(buffer.begin()+read_next, buffer.end());
                v.reserve(v.size() + lastidx);
                std::copy(buffer.begin(), buffer.begin() + lastidx, std::back_inserter(v));
                read_next = lastidx;
                return v;
            } else {
                std::vector<RecordType> v(buffer.begin()+read_next, buffer.begin()+lastidx);
                read_next = (lastidx) % ringbufferSize();
                return v;
            }
        }
};

}} // namespace

#endif


