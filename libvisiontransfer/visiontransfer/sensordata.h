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

#ifndef VISIONTRANSFER_SENSORDATA_H
#define VISIONTRANSFER_SENSORDATA_H

#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846 
#endif

namespace visiontransfer {

/**
 * \brief Base class for sensor records with timestamp and status (reliability) fields
 */
class SensorRecord {
protected:
    int timestampSec;
    int timestampUSec;
    unsigned char status;
public:
    SensorRecord(int timestampSec, int timestampUSec, unsigned char status): timestampSec(timestampSec), timestampUSec(timestampUSec), status(status) {}
    /**
     * Returns the sensor-reported timestamp for the reading
     */
    void getTimestamp(int& s, int& us) const { s = timestampSec; us = timestampUSec; }
    /**
     * Returns the current sensor calibration status (range 0..3)
     * 0: Sensor unreliable; 1: Accuracy low; 2: ~ medium; 3: ~ high
     */
    unsigned char getStatus() const { return status; }
};

/**
 * \brief Encapsulate a scalar sensor measurement, containing the value, as well as timestamp and status fields
 */
class TimestampedScalar: public SensorRecord {
public:
    double value() const { return valueIntl; }
    TimestampedScalar(int timestampSec, int timestampUSec, unsigned char status, double value): SensorRecord(timestampSec, timestampUSec, status), valueIntl(value) {}
    TimestampedScalar(): SensorRecord(0, 0, 0), valueIntl(0) { }
private:
    double valueIntl;
};


/**
 * \brief Encapsulate a 3D sensor report, containing X, Y, Z, as well as timestamp and status fields
 */
class TimestampedVector: public SensorRecord {
public:
    double x() const { return xIntl; }
    double y() const { return yIntl; }
    double z() const { return zIntl; }
    TimestampedVector(int timestampSec, int timestampUSec, unsigned char status, double x, double y, double z): SensorRecord(timestampSec, timestampUSec, status), xIntl(x), yIntl(y), zIntl(z) {}
    TimestampedVector(): SensorRecord(0, 0, 0), xIntl(0), yIntl(0), zIntl(0) { }
private:
    double xIntl, yIntl, zIntl;
};

/**
 * \brief Encapsulate a 4D (quaternion) sensor report, containing X, Y, Z, W, as well as timestamp and status fields and measurement accuracy
 *
 * Component r is the real part of the quaternion, also called w (ijkr corresponds to xyzw).
 */
class TimestampedQuaternion: public SensorRecord {
public:
    double x() const { return xIntl; }
    double y() const { return yIntl; }
    double z() const { return zIntl; }
    double w() const { return wIntl; }
    /**
     * Convert the quaternion to device roll, pitch, and yaw (radians)
     */
    void getRollPitchYaw(double& roll, double& pitch, double& yaw) {
        // roll
        double sinr_cosp = 2 * (wIntl * xIntl + -zIntl * yIntl);
        double cosr_cosp = 1 - 2 * (xIntl * xIntl + -zIntl * -zIntl);
        roll = std::atan2(sinr_cosp, cosr_cosp);
        // pitch
        double sinp = 2 * (wIntl * -zIntl - yIntl * xIntl);
        pitch = (std::abs(sinp) >= 1) ? ((sinp<0)?-(M_PI/2):(M_PI/2)) : std::asin(sinp);
        // yaw
        double siny_cosp = 2 * (wIntl * yIntl + xIntl * -zIntl);
        double cosy_cosp = 1 - 2 * (-zIntl * -zIntl + yIntl * yIntl);
        yaw = std::atan2(siny_cosp, cosy_cosp);
    }
    /**
     * Returns the internal device-reported angular accuracy (radians)
     */
    double accuracy() const { return accuracyIntl; }
    TimestampedQuaternion(int timestampSec, int timestampUSec, unsigned char status, double x, double y, double z, double w, double accuracy): SensorRecord(timestampSec, timestampUSec, status), xIntl(x), yIntl(y), zIntl(z), wIntl(w), accuracyIntl(accuracy) {}
    TimestampedQuaternion(): SensorRecord(0, 0, 0), xIntl(0), yIntl(0), zIntl(0), wIntl(0), accuracyIntl(0) { }
private:
    double xIntl, yIntl, zIntl, wIntl;
    double accuracyIntl;
};

} // namespace

#endif

