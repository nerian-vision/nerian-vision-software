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

#ifndef VISIONTRANSFER_DATACHANNELSERVICE_H
#define VISIONTRANSFER_DATACHANNELSERVICE_H

#include <vector>
#include <visiontransfer/common.h>
#include <visiontransfer/deviceinfo.h>
#include <visiontransfer/sensordata.h>

using namespace visiontransfer;

namespace visiontransfer {

/**
 * This is the DataChannelServer public API, backwards compatible with C++-98
 *
 * The DataChannelService encapsulates miscellaneous device services,
 * which are not implemented on all Nerian devices. Availability can be
 * queried; access to unavailable elements will normally return default
 * values or silently execute nothing.
 *
 * The imuGet...() and envGet...() functions access an inertial measurement unit
 * with attached environmental sensor, realized with the Hillcrest BNO080
 * and Bosch BME280, respectively, on supported devices.
 * 
 */
class VT_EXPORT DataChannelService {
public:
    class Pimpl;
    /**
     *  Initialize a new background data channel service, connecting to the
     *  specified device. The optional argument pollDelayUSec is used in the
     *  background receive/update loop, the default of 1000us can be overridden
     *  for the sake of efficiency or if minimum latency requirements differ.
     */
    DataChannelService(DeviceInfo deviceInfo, unsigned long pollDelayUSec=1000);
    /**
     *  Initialize a new background data channel service, connecting to the
     *  specified IP address. The optional argument pollDelayUSec is used in the
     *  background receive/update loop, the default of 1000us can be overridden
     *  for the sake of efficiency or if minimum latency requirements differ.
     */
    DataChannelService(const char* ipAddr, unsigned long pollDelayUSec=1000);
    ~DataChannelService();
public:
    /**
     * \brief Return whether the device will provide data from an Inertial Measurement Unit
     */
    bool imuAvailable();

    /**
     * \brief Return the most recent rotation quaternion, relative to gravity and magnetic north
     */
    TimestampedQuaternion imuGetRotationQuaternion();

    /**
     * \brief Return the current contents of the rotation quaternion data buffer, optionally between specified timestamps.
     *
     * This operation consumes an internal ring buffer up to the desired end stamp, data older than the desired window is silently discarded.
     */
    std::vector<TimestampedQuaternion> imuGetRotationQuaternionSeries(int fromSec=0, int fromUSec=0, int untilSec=0x7FFFffffl, int untilUSec=0x7FFFffffl);

    /**
     * \brief Return the most recent calibrated accelerometer reading
     */
    TimestampedVector imuGetAcceleration();

    /**
     * \brief Return the current contents of the calibrated accelerometer data buffer, optionally between specified timestamps.
     *
     * This operation consumes an internal ring buffer up to the desired end stamp, data older than the desired window is silently discarded.
     */
    std::vector<TimestampedVector> imuGetAccelerationSeries(int fromSec=0, int fromUSec=0, int untilSec=0x7FFFffffl, int untilUSec=0x7FFFffffl);

    /**
     * \brief Return the most recent calibrated angular accelerations from the gyroscope
     */
    TimestampedVector imuGetGyroscope();

    /**
     * \brief Return the current contents of the gyroscope data buffer, optionally between specified timestamps.
     *
     * This operation consumes an internal ring buffer up to the desired end stamp, data older than the desired window is silently discarded.
     */
    std::vector<TimestampedVector> imuGetGyroscopeSeries(int fromSec=0, int fromUSec=0, int untilSec=0x7FFFffffl, int untilUSec=0x7FFFffffl);

    /**
     * \brief Return the most recent magnetometer readings
     */
    TimestampedVector imuGetMagnetometer();

    /**
     * \brief Return the current contents of the magnetometer data buffer, optionally between specified timestamps.
     *
     * This operation consumes an internal ring buffer up to the desired end stamp, data older than the desired window is silently discarded.
     */
    std::vector<TimestampedVector> imuGetMagnetometerSeries(int fromSec=0, int fromUSec=0, int untilSec=0x7FFFffffl, int untilUSec=0x7FFFffffl);

    /**
     * \brief Return the most recent linear acceleration, i.e. with gravity factored out
     */
    TimestampedVector imuGetLinearAcceleration();

    /**
     * \brief Return the current contents of the linear acceleration (without gravity) data buffer, optionally between specified timestamps.
     *
     * This operation consumes an internal ring buffer up to the desired end stamp, data older than the desired window is silently discarded.
     */
    std::vector<TimestampedVector> imuGetLinearAccelerationSeries(int fromSec=0, int fromUSec=0, int untilSec=0x7FFFffffl, int untilUSec=0x7FFFffffl);

    /**
     * \brief Return the most recent gravity measurement
     */
    TimestampedVector imuGetGravity();

    /**
     * \brief Return the current contents of the gravity data buffer, optionally between specified timestamps.
     *
     * This operation consumes an internal ring buffer up to the desired end stamp, data older than the desired window is silently discarded.
     */
    std::vector<TimestampedVector> imuGetGravitySeries(int fromSec=0, int fromUSec=0, int untilSec=0x7FFFffffl, int untilUSec=0x7FFFffffl);

private:
    Pimpl* pimpl;
};


} // namespaces

#endif

