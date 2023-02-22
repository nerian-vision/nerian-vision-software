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

// DataChannel protocol definition for the BNO080 IMU sensor

#ifndef VISIONTRANSFER_DATACHANNEL_IMU_BNO080_H
#define VISIONTRANSFER_DATACHANNEL_IMU_BNO080_H

#include <visiontransfer/datachannelservicebase.h>
#include <visiontransfer/sensorringbuffer.h>

#include <vector>

namespace visiontransfer {
namespace internal {

// TODO IMPLEMENT_ME / Not active at the moment:
/**
 * \brief Commands to control the IMU/environmental sensor and its data reporting
 */
class DataChannelIMUBNO080Commands {
public:
    enum Command {
        BNOReserved,
        BNOReset,
        BNOEnableSensors,
        BNORateLimit,
        BNOReports
    };
};

/**
 * \brief Helper functions for IMU control messages
 */
class DataChannelIMUBNO080Util {
    static DataChannelIMUBNO080Commands::Command getCommand(unsigned char* data, int datalen) {
        if (datalen < 2) throw std::runtime_error("Buffer too small");
        return (DataChannelIMUBNO080Commands::Command) ntohs(*((uint16_t*) data));
    }
    static int packResetMessage(unsigned char* data, int datalen) {
        if (datalen < 2) throw std::runtime_error("Buffer too small");
        *((uint16_t*)data) = htons(DataChannelIMUBNO080Commands::BNOReset);
        return 2;
    }
};


/**
 * \brief Encapsulated receiver with ring buffers for IMU / environment sensor data.
 *
 * Public access transparently via DataChannelService
 */
class ClientSideDataChannelIMUBNO080: public DataChannel {
private:
    static constexpr int RINGBUFFER_SIZE = 2048;
public:
    // These are inspected and consumed by the DataChannelService
    SensorDataRingBuffer<TimestampedVector, RINGBUFFER_SIZE> ringbufXYZ[6]; // for sensors 0x01 .. 0x06 (w/o 5)
    TimestampedVector lastXYZ[6]; // cache the most recent value for each channel
    SensorDataRingBuffer<TimestampedQuaternion, RINGBUFFER_SIZE> ringbufRotationQuaternion; // for 0x05, Rot Vec
    TimestampedQuaternion lastRotationQuaternion;
    SensorDataRingBuffer<TimestampedScalar, RINGBUFFER_SIZE> ringbufScalar[5]; // 0x0a .. 0x0e (temp, pressure..)
    TimestampedScalar lastScalar[5];
public:
    ClientSideDataChannelIMUBNO080();
    DataChannel::Type getChannelType() const override { return DataChannel::Types::BNO080; }
    int handleSensorInputRecord(unsigned char* data, int datalen, uint64_t baseTime);
    void handleChunk(unsigned char* data, int datalen);
    int handleMessage(DataChannelMessage& message, sockaddr_in* sender) override;
    bool initialize() override { return true; }
    int startService() override { return 1; }
    int stopService() override { return 1; }
};

}} // namespaces

#endif

