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

#include <visiontransfer/datachannel-imu-bno080.h>
#include <visiontransfer/protocol-sh2-imu-bno080.h>

namespace visiontransfer {
namespace internal {

ClientSideDataChannelIMUBNO080::ClientSideDataChannelIMUBNO080()
: DataChannel() {
    infoString = "Receiver for the BNO080 IMU sensor";
    // Sane defaults for orientation etc. if values are queried despite lack of sensor
    lastXYZ[0x01 - 1] =       {0, 0, 0,  0, 0, 10};
    lastXYZ[0x02 - 1] =       {0, 0, 0,  0, 0, 0};
    lastXYZ[0x03 - 1] =       {0, 0, 0,  0, 0, 0};
    lastXYZ[0x04 - 1] =       {0, 0, 0,  0, 0, 0};
    lastXYZ[0x05 - 1] =       {0, 0, 0,  0, 0, 0}; // unused, cf. the quaternion below
    lastXYZ[0x06 - 1] =       {0, 0, 0,  0, 0, 10};
    lastScalar[0x0a - 0x0a] = {0, 0, 0,  0};
    lastScalar[0x0b - 0x0a] = {0, 0, 0,  0}; // unused / sensor not present
    lastScalar[0x0d - 0x0a] = {0, 0, 0,  0};
    lastScalar[0x0d - 0x0a] = {0, 0, 0,  0}; // unused / sensor not present
    lastScalar[0x0e - 0x0a] = {0, 0, 0,  0};
    lastRotationQuaternion =  {0, 0, 0,  0.0, 0.0, 0.0, 1.0, 0}; // channel 0x05
}

int ClientSideDataChannelIMUBNO080::handleSensorInputRecord(unsigned char* data, int datalen, uint64_t baseTime) {
    int sensorid = data[0];
    int status =   data[2] & 3;
    int delay =    ((data[2] & 0xfc) << 6) | data[3];
    uint64_t myTime = baseTime + delay;
    switch (sensorid) {
        // these have identical format, 3D vector
        case SH2Constants::SENSOR_ACCELEROMETER:       //0x01
        case SH2Constants::SENSOR_GYROSCOPE:           //0x02
        case SH2Constants::SENSOR_MAGNETOMETER:        //0x03
        case SH2Constants::SENSOR_LINEAR_ACCELERATION: //0x04
        case SH2Constants::SENSOR_GRAVITY:             //0x06
            {
                double x, y, z;
                auto q = sh2GetSensorQPoint(sensorid);
                x = sh2ConvertFixedQ16(sh2GetU16(data+4), q);
                y = sh2ConvertFixedQ16(sh2GetU16(data+6), q);
                z = sh2ConvertFixedQ16(sh2GetU16(data+8), q);
                // sensorid-1 is in range [0..5]
                lastXYZ[sensorid-1] = TimestampedVector((int) (myTime/1000000), (int) (myTime%1000000), status, x, z, -y);
                ringbufXYZ[sensorid-1].pushData(lastXYZ[sensorid-1]);
                break;
            }
        // this one is 4D (quaternion data), plus accuracy field
        case SH2Constants::SENSOR_ROTATION_VECTOR:     //0x05
        case SH2Constants::SENSOR_GAME_ROTATION_VECTOR://0x08
        case SH2Constants::SENSOR_GEOMAGNETIC_ROTATION://0x09
            {
                double x, y, z, w;
                double accuracy = -1.0;
                auto q = sh2GetSensorQPoint(sensorid);
                x = sh2ConvertFixedQ16(sh2GetU16(data+4), q);
                y = sh2ConvertFixedQ16(sh2GetU16(data+6), q);
                z = sh2ConvertFixedQ16(sh2GetU16(data+8), q);
                w = sh2ConvertFixedQ16(sh2GetU16(data+10), q);
                if (sensorid!=SH2Constants::SENSOR_GAME_ROTATION_VECTOR) {
                    // The BNO080 'game rotation vectors' to not provide an accuracy estimate
                    //  (since they do not estimate yaw in a fixed geomagnetic system).
                    accuracy = (double) ((signed short) sh2GetU16(data+12)) / (double) (1 << 12); // accuracy Q point is 12
                }
                lastRotationQuaternion = TimestampedQuaternion((int) (myTime/1000000), (int) (myTime%1000000), status, x, z, -y, w, accuracy);
                ringbufRotationQuaternion.pushData(lastRotationQuaternion);
                break;
            }
        // the misc. sensors are 1D floats (32b or 16b)
        case SH2Constants::SENSOR_PRESSURE:            // 0x0a
        case SH2Constants::SENSOR_AMBIENT_LIGHT:       // 0x0b
            {
                signed short svalue = sh2GetU32(data+4);
                double value = (double) svalue / (double)(1 << sh2GetSensorQPoint(sensorid));
                lastScalar[sensorid - 0x0a] = TimestampedScalar((int) (myTime/1000000), (int) (myTime%1000000), status, value);
                ringbufScalar[sensorid - 0x0a].pushData(lastScalar[sensorid - 0x0a]);
                break;
            }
        case SH2Constants::SENSOR_HUMIDITY:            // 0x0c
        case SH2Constants::SENSOR_PROXIMITY:           // 0x0d
        case SH2Constants::SENSOR_TEMPERATURE:         // 0x0e
            {
                signed short svalue = sh2GetU16(data+4);
                double value = (double) svalue / (double)(1 << sh2GetSensorQPoint(sensorid));
                lastScalar[sensorid - 0x0a] = TimestampedScalar((int) (myTime/1000000), (int) (myTime%1000000), status, value);
                ringbufScalar[sensorid - 0x0a].pushData(lastScalar[sensorid - 0x0a]);
                break;
            }
        default:
            break;
    }
    int recordlen = sh2GetSensorReportLength(sensorid);
    return recordlen;
}

void ClientSideDataChannelIMUBNO080::handleChunk(unsigned char* data, int datalen) {
    if (datalen < 5) return;
    auto cargobase = reinterpret_cast<SH2CargoBase*>(data);
    static uint64_t interruptTime = 0; // will always be reported first, below
    switch (cargobase->getReportType()) {
        case 0xff: { // Our own interrupt-synchronized timestamp
                auto report = reinterpret_cast<SH2CargoBodyScenescanTimestamp*>(data);
                interruptTime = report->getUSecSinceEpoch();
                break;
            }
        case 0xfb: {  // SH-2 Time Base (followed by sensor reports)
                auto report = reinterpret_cast<SH2CargoBodyTimeBase*>(data);
                long basetimeOfs = report->getTimeBase();
                uint64_t localBase = interruptTime - basetimeOfs;
                data += sizeof(SH2CargoBodyTimeBase); datalen -= sizeof(SH2CargoBodyTimeBase);
                // The (variable-length) remainder of this packet are concatenated SH2 sensor input reports.
                // They must be parsed in order since they are of differing sizes, depending on the sensor type.
                int recordlen;
                while (datalen > 0) {
                    recordlen = handleSensorInputRecord(data, datalen, localBase);
                    if (recordlen<1) break; // record type unknown -> size unknown -> cannot proceed
                    data += recordlen; datalen -= recordlen;
                }
                break;
            }
        case 0xfa: // SH-2 Timestamp Rebase
                   // Required for BNO batch reports that span >1.6s.
                   // This is not relevant here, since we set the batch delay to intervals
                   // considerably shorter than that (the server stores those batches
                   // immediately with integrated base timestamps).
        default: {
            }
    }
}

int ClientSideDataChannelIMUBNO080::handleMessage(DataChannelMessage& message, sockaddr_in* sender) {
    unsigned char* data = message.payload;
    int datalen = message.header.payloadSize;
    while (datalen > 0) {
        int elemlen = sh2GetU16(data) & 0x7fff;
        handleChunk(data, elemlen);
        data += elemlen; datalen -= elemlen;
    }
    return 1;
};

}} // namespaces

