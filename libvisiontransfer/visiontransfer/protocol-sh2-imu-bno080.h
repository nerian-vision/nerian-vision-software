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

#ifndef VISIONTRANSFER_PROTOCOL_SH2_IMU_BNO080
#define VISIONTRANSFER_PROTOCOL_SH2_IMU_BNO080

#include <cstdint>

namespace visiontransfer {
namespace internal {

struct SH2Constants {
    static constexpr uint8_t CHANNEL_COMMAND     = 0;
    static constexpr uint8_t CHANNEL_EXECUTABLE  = 1;
    static constexpr uint8_t CHANNEL_CONTROL     = 2;
    static constexpr uint8_t CHANNEL_REPORTS     = 3;
    static constexpr uint8_t CHANNEL_WAKE_REPORTS= 4;
    static constexpr uint8_t CHANNEL_GYRO        = 5;

    static constexpr uint8_t REPORT_COMMAND_RESPONSE          = 0xf1;
    static constexpr uint8_t REPORT_COMMAND_REQUEST           = 0xf2;
    static constexpr uint8_t REPORT_FRS_READ_RESPONSE         = 0xf3;
    static constexpr uint8_t REPORT_FRS_READ_REQUEST          = 0xf4;
    static constexpr uint8_t REPORT_FRS_WRITE_RESPONSE        = 0xf5;
    static constexpr uint8_t REPORT_FRS_WRITE_DATA            = 0xf6;
    static constexpr uint8_t REPORT_FRS_WRITE_REQUEST         = 0xf7;
    static constexpr uint8_t REPORT_PRODUCT_ID_RESPONSE       = 0xf8;
    static constexpr uint8_t REPORT_PRODUCT_ID_REQUEST        = 0xf9;
    static constexpr uint8_t REPORT_TIMESTAMP_REBASE          = 0xfa;
    static constexpr uint8_t REPORT_BASE_TIMESTAMP_REFERENCE  = 0xfb;
    static constexpr uint8_t REPORT_GET_FEATURE_RESPONSE      = 0xfc;
    static constexpr uint8_t REPORT_SET_FEATURE_COMMAND       = 0xfd;
    static constexpr uint8_t REPORT_GET_FEATURE_REQUEST       = 0xfe;

    // Commands supported by report 0xf2 / 0xf1
    static constexpr uint8_t COMMAND_REPORT_ERRORS                      = 0x01;
    static constexpr uint8_t COMMAND_COUNTS                             = 0x02;
    static constexpr uint8_t COMMAND_TARE                               = 0x03;
    static constexpr uint8_t COMMAND_INITIALIZE                         = 0x04;
    static constexpr uint8_t COMMAND_RESERVED_05                        = 0x05;
    static constexpr uint8_t COMMAND_SAVE_DCD                           = 0x06;
    static constexpr uint8_t COMMAND_ME_CALIBRATION                     = 0x07;
    static constexpr uint8_t COMMAND_RESERVED_08                        = 0x08;
    static constexpr uint8_t COMMAND_PERIODIC_DCD_SAVE                  = 0x09;
    static constexpr uint8_t COMMAND_GET_OSCILLATOR_TYPE                = 0x0a;
    static constexpr uint8_t COMMAND_CLEAR_DCD_AND_RESET                = 0x0b;
    static constexpr uint8_t COMMAND_CALIBRATION                        = 0x0c;
    static constexpr uint8_t COMMAND_BOOTLOADER                         = 0x0d;
    static constexpr uint8_t COMMAND_INTERACTIVE_CALIBRATION            = 0x0e;

    // Subcommands, for certain commands only
    //   DCD / ME / Bootloader not considered yet, here
    static constexpr uint8_t SUBCOMMAND_COUNTS__GET_COUNTS              = 0x00;
    static constexpr uint8_t SUBCOMMAND_COUNTS__CLEAR_COUNTS            = 0x01;
    static constexpr uint8_t SUBCOMMAND_TARE__TARE_NOW                  = 0x00;
    static constexpr uint8_t SUBCOMMAND_TARE__PERSIST_TARE              = 0x01;
    static constexpr uint8_t SUBCOMMAND_TARE__SET_REORIENTATION         = 0x02;
    static constexpr uint8_t SUBCOMMAND_CALIBRATION__START_CALIBRATION  = 0x00;
    static constexpr uint8_t SUBCOMMAND_CALIBRATION__FINISH_CALIBRATION = 0x01;

    // What to tare (can be ORed)
    static constexpr uint8_t TARE_AXIS_X                                   = 1;
    static constexpr uint8_t TARE_AXIS_Y                                   = 2;
    static constexpr uint8_t TARE_AXIS_Z                                   = 4;

    // Reference for tare operation
    static constexpr uint8_t TARE_BASE_ROTATION_VECTOR                     = 0;
    static constexpr uint8_t TARE_BASE_GAMING_ROTATION_VECTOR              = 1;
    static constexpr uint8_t TARE_BASE_GEOMAGNETIC_ROTATION_VECTOR         = 2;
    static constexpr uint8_t TARE_BASE_GYRO_INTEGRATED_ROTATION_VECTOR     = 3;
    static constexpr uint8_t TARE_BASE_ARVR_STABILIZED_ROTATION_VECTOR     = 4;
    static constexpr uint8_t TARE_BASE_ARVR_STABILIZED_GAME_ROTATION_VECTOR= 5;

    // Sensor types (= sensor input report ID)
    static constexpr uint8_t SENSOR_ACCELEROMETER                       = 0x01;
    static constexpr uint8_t SENSOR_GYROSCOPE                           = 0x02;
    static constexpr uint8_t SENSOR_MAGNETOMETER                        = 0x03;
    static constexpr uint8_t SENSOR_LINEAR_ACCELERATION                 = 0x04;
    static constexpr uint8_t SENSOR_ROTATION_VECTOR                     = 0x05;
    static constexpr uint8_t SENSOR_GRAVITY                             = 0x06;
    static constexpr uint8_t SENSOR_GYROSCOPE_UNCALIBRATED              = 0x07;
    static constexpr uint8_t SENSOR_GAME_ROTATION_VECTOR                = 0x08;
    static constexpr uint8_t SENSOR_GEOMAGNETIC_ROTATION                = 0x09;
    static constexpr uint8_t SENSOR_PRESSURE                            = 0x0a;
    static constexpr uint8_t SENSOR_AMBIENT_LIGHT                       = 0x0b;
    static constexpr uint8_t SENSOR_HUMIDITY                            = 0x0c;
    static constexpr uint8_t SENSOR_PROXIMITY                           = 0x0d;
    static constexpr uint8_t SENSOR_TEMPERATURE                         = 0x0e;
    static constexpr uint8_t SENSOR_MAGNETOMETER_UNCALIBRATED           = 0x0f;
    static constexpr uint8_t SENSOR_TAP_DETECTOR                        = 0x10;
    static constexpr uint8_t SENSOR_STEP_COUNTER                        = 0x11;
    static constexpr uint8_t SENSOR_SIGNIFICANT_MOTION                  = 0x12;
    static constexpr uint8_t SENSOR_STABILITY_CLASSIFIER                = 0x13;
    static constexpr uint8_t SENSOR_ACCELEROMETER_RAW                   = 0x14;
    static constexpr uint8_t SENSOR_GYROSCOPE_RAW                       = 0x15;
    static constexpr uint8_t SENSOR_MAGNETOMETER_RAW                    = 0x16;
    static constexpr uint8_t SENSOR_STEP_DETECTOR                       = 0x18;
    static constexpr uint8_t SENSOR_SHAKE_DETECTOR                      = 0x19;
    static constexpr uint8_t SENSOR_FLIP_DETECTOR                       = 0x1a;
    static constexpr uint8_t SENSOR_PICKUP_DETECTOR                     = 0x1b;
    static constexpr uint8_t SENSOR_STABILITY_DETECTOR                  = 0x1c;
    static constexpr uint8_t SENSOR_PERSONAL_ACTIVITY_CLASSIFIER        = 0x1e;
    static constexpr uint8_t SENSOR_SLEEP_DETECTOR                      = 0x1f;
    static constexpr uint8_t SENSOR_TILT_DETECTOR                       = 0x20;
    static constexpr uint8_t SENSOR_POCKET_DETECTOR                     = 0x21;
    static constexpr uint8_t SENSOR_CIRCLE_DETECTOR                     = 0x22;
    static constexpr uint8_t SENSOR_HEART_RATE_MONITOR                  = 0x23;
    static constexpr uint8_t SENSOR_ARVR_STABILIZED_ROTATION_VECTOR     = 0x28;
    static constexpr uint8_t SENSOR_ARVR_STABILIZED_GAME_ROTATION_VECTOR= 0x29;
    static constexpr uint8_t SENSOR_GYRO_INTEGRATED_ROTATION_VECTOR     = 0x2a;
    static constexpr uint8_t SENSOR_MOTION_REQUEST                      = 0x2b;

    // FRS Configuration Response: Status/Error field
    static constexpr uint8_t FRS_WRITE_RESPONSE_STATUS_WORDS_RECEIVED           = 0x00;
    static constexpr uint8_t FRS_WRITE_RESPONSE_STATUS_UNRECOGNIZED_FRS_TYPE    = 0x01;
    static constexpr uint8_t FRS_WRITE_RESPONSE_STATUS_BUSY                     = 0x02;
    static constexpr uint8_t FRS_WRITE_RESPONSE_STATUS_WRITE_COMPLETED          = 0x03;
    static constexpr uint8_t FRS_WRITE_RESPONSE_STATUS_WRITE_MODE_READY         = 0x04;
    static constexpr uint8_t FRS_WRITE_RESPONSE_STATUS_WRITE_FAILED             = 0x05;
    static constexpr uint8_t FRS_WRITE_RESPONSE_STATUS_UNEXPECTED_DATA          = 0x06;
    static constexpr uint8_t FRS_WRITE_RESPONSE_STATUS_INVALID_LENGTH           = 0x07;
    static constexpr uint8_t FRS_WRITE_RESPONSE_STATUS_RECORD_VALID             = 0x08;
    static constexpr uint8_t FRS_WRITE_RESPONSE_STATUS_RECORD_INVALID           = 0x09;
    static constexpr uint8_t FRS_WRITE_RESPONSE_STATUS_DEVICE_ERROR__DEPRECATED = 0x0A;
    static constexpr uint8_t FRS_WRITE_RESPONSE_STATUS_READ_ONLY_RECORD         = 0x0B;
    static constexpr uint8_t FRS_WRITE_RESPONSE_STATUS_CANNOT_WRITE_MEMORY_FULL = 0x0C;
};

inline uint64_t sh2GetU64(const unsigned char* d) {
   return static_cast<uint64_t>(d[0]) | (static_cast<uint64_t>(d[1]) << 8)
       | (static_cast<uint64_t>(d[2]) << 16) | (static_cast<uint64_t>(d[3]) << 24)
       | (static_cast<uint64_t>(d[4]) << 32) | (static_cast<uint64_t>(d[5]) << 40)
       | (static_cast<uint64_t>(d[6]) << 48) | (static_cast<uint64_t>(d[7]) << 56);
}
inline uint32_t sh2GetU32(const unsigned char* d) {
    return static_cast<uint64_t>(d[0]) | (static_cast<uint64_t>(d[1]) << 8) |
        (static_cast<uint64_t>(d[2]) << 16) | (static_cast<uint64_t>(d[3]) << 24);
}
inline uint16_t sh2GetU16(const unsigned char* d) {
   return static_cast<uint16_t>(d[0]) | (static_cast<uint16_t>(d[1]) << 8);
}
inline uint8_t sh2GetU8(const unsigned char* d) {
   return d[0];
}
inline double sh2ConvertFixedQ16(uint16_t v, unsigned char q) {
    return (double) ((int16_t) v) / (double)(1 << q);
}

inline int sh2GetSensorReportLength(unsigned int sensorReportID) {
    switch(sensorReportID) {
        case SH2Constants::SENSOR_ACCELEROMETER:                        return 10;  //ID 0x01
        case SH2Constants::SENSOR_GYROSCOPE:                            return 10;  //ID 0x02
        case SH2Constants::SENSOR_MAGNETOMETER:                         return 10;  //ID 0x03
        case SH2Constants::SENSOR_LINEAR_ACCELERATION:                  return 10;  //ID 0x04
        case SH2Constants::SENSOR_ROTATION_VECTOR:                      return 14;  //ID 0x05
        case SH2Constants::SENSOR_GRAVITY:                              return 10;  //ID 0x06
        case SH2Constants::SENSOR_GYROSCOPE_UNCALIBRATED:               return 16;  //ID 0x07
        case SH2Constants::SENSOR_GAME_ROTATION_VECTOR:                 return 12;  //ID 0x08
        case SH2Constants::SENSOR_GEOMAGNETIC_ROTATION:                 return 14;  //ID 0x09
        case SH2Constants::SENSOR_PRESSURE:                             return 8;   //ID 0x0a
        case SH2Constants::SENSOR_AMBIENT_LIGHT:                        return 8;   //ID 0x0b
        case SH2Constants::SENSOR_HUMIDITY:                             return 6;   //ID 0x0c
        case SH2Constants::SENSOR_PROXIMITY:                            return 6;   //ID 0x0d
        case SH2Constants::SENSOR_TEMPERATURE:                          return 6;   //ID 0x0e
        case SH2Constants::SENSOR_MAGNETOMETER_UNCALIBRATED:            return 16;  //ID 0x0f
        case SH2Constants::SENSOR_TAP_DETECTOR:                         return 5;   //ID 0x10
        case SH2Constants::SENSOR_STEP_COUNTER:                         return 12;  //ID 0x11
        case SH2Constants::SENSOR_SIGNIFICANT_MOTION:                   return 6;   //ID 0x12
        case SH2Constants::SENSOR_STABILITY_CLASSIFIER:                 return 6;   //ID 0x13
        case SH2Constants::SENSOR_ACCELEROMETER_RAW:                    return 16;  //ID 0x14
        case SH2Constants::SENSOR_GYROSCOPE_RAW:                        return 16;  //ID 0x15
        case SH2Constants::SENSOR_MAGNETOMETER_RAW:                     return 16;  //ID 0x16
        case SH2Constants::SENSOR_STEP_DETECTOR:                        return 8;   //ID 0x18
        case SH2Constants::SENSOR_SHAKE_DETECTOR:                       return 6;   //ID 0x19
        case SH2Constants::SENSOR_FLIP_DETECTOR:                        return 6;   //ID 0x1a
        case SH2Constants::SENSOR_PICKUP_DETECTOR:                      return 6;   //ID 0x1b
        case SH2Constants::SENSOR_STABILITY_DETECTOR:                   return 6;   //ID 0x1c
        case SH2Constants::SENSOR_PERSONAL_ACTIVITY_CLASSIFIER:         return 16;  //ID 0x1e
        case SH2Constants::SENSOR_SLEEP_DETECTOR:                       return 6;   //ID 0x1f
        case SH2Constants::SENSOR_TILT_DETECTOR:                        return 6;   //ID 0x20
        case SH2Constants::SENSOR_POCKET_DETECTOR:                      return 6;   //ID 0x21
        case SH2Constants::SENSOR_CIRCLE_DETECTOR:                      return 6;   //ID 0x22
        case SH2Constants::SENSOR_HEART_RATE_MONITOR:                   return 6;   //ID 0x23
        case SH2Constants::SENSOR_ARVR_STABILIZED_ROTATION_VECTOR:      return 14;  //ID 0x28
        case SH2Constants::SENSOR_ARVR_STABILIZED_GAME_ROTATION_VECTOR: return 12;  //ID 0x29
        case SH2Constants::SENSOR_GYRO_INTEGRATED_ROTATION_VECTOR:      return 14;  //ID 0x2a
        case SH2Constants::SENSOR_MOTION_REQUEST:                       return 6;   //ID 0x2b
        default: return -1;
    }
}

// The Q point for fixed-point values, i.e. the base 2 exponent for division
//  for consistency reasons also 0 for N/A / undefined
inline int sh2GetSensorQPoint(unsigned int sensorReportID) {
    switch(sensorReportID) {
        case SH2Constants::SENSOR_ACCELEROMETER:                        return 8;   //ID 0x01
        case SH2Constants::SENSOR_GYROSCOPE:                            return 9;   //ID 0x02
        case SH2Constants::SENSOR_MAGNETOMETER:                         return 4;   //ID 0x03
        case SH2Constants::SENSOR_LINEAR_ACCELERATION:                  return 8;   //ID 0x04
        case SH2Constants::SENSOR_ROTATION_VECTOR:                      return 14;  //ID 0x05 // but 12 for accuracy fields
        case SH2Constants::SENSOR_GRAVITY:                              return 8;   //ID 0x06
        case SH2Constants::SENSOR_GYROSCOPE_UNCALIBRATED:               return 9;   //ID 0x07
        case SH2Constants::SENSOR_GAME_ROTATION_VECTOR:                 return 14;  //ID 0x08
        case SH2Constants::SENSOR_GEOMAGNETIC_ROTATION:                 return 14;  //ID 0x09
        case SH2Constants::SENSOR_PRESSURE:                             return 20;  //ID 0x0a
        case SH2Constants::SENSOR_AMBIENT_LIGHT:                        return 8;   //ID 0x0b
        case SH2Constants::SENSOR_HUMIDITY:                             return 8;   //ID 0x0c
        case SH2Constants::SENSOR_PROXIMITY:                            return 4;   //ID 0x0d
        case SH2Constants::SENSOR_TEMPERATURE:                          return 7;   //ID 0x0e
        case SH2Constants::SENSOR_MAGNETOMETER_UNCALIBRATED:            return 4;   //ID 0x0f
        case SH2Constants::SENSOR_TAP_DETECTOR:                         return 0;   //ID 0x10
        case SH2Constants::SENSOR_STEP_COUNTER:                         return 0;   //ID 0x11
        case SH2Constants::SENSOR_SIGNIFICANT_MOTION:                   return 0;   //ID 0x12
        case SH2Constants::SENSOR_STABILITY_CLASSIFIER:                 return 0;   //ID 0x13
        case SH2Constants::SENSOR_ACCELEROMETER_RAW:                    return 0;   //ID 0x14
        case SH2Constants::SENSOR_GYROSCOPE_RAW:                        return 0;   //ID 0x15
        case SH2Constants::SENSOR_MAGNETOMETER_RAW:                     return 0;   //ID 0x16
        case SH2Constants::SENSOR_STEP_DETECTOR:                        return 0;   //ID 0x18
        case SH2Constants::SENSOR_SHAKE_DETECTOR:                       return 0;   //ID 0x19
        case SH2Constants::SENSOR_FLIP_DETECTOR:                        return 0;   //ID 0x1a
        case SH2Constants::SENSOR_PICKUP_DETECTOR:                      return 0;   //ID 0x1b
        case SH2Constants::SENSOR_STABILITY_DETECTOR:                   return 0;   //ID 0x1c
        case SH2Constants::SENSOR_PERSONAL_ACTIVITY_CLASSIFIER:         return 0;   //ID 0x1e
        case SH2Constants::SENSOR_SLEEP_DETECTOR:                       return 0;   //ID 0x1f
        case SH2Constants::SENSOR_TILT_DETECTOR:                        return 0;   //ID 0x20
        case SH2Constants::SENSOR_POCKET_DETECTOR:                      return 0;   //ID 0x21
        case SH2Constants::SENSOR_CIRCLE_DETECTOR:                      return 0;   //ID 0x22
        case SH2Constants::SENSOR_HEART_RATE_MONITOR:                   return 0;   //ID 0x23
        case SH2Constants::SENSOR_ARVR_STABILIZED_ROTATION_VECTOR:      return 14;  //ID 0x28
        case SH2Constants::SENSOR_ARVR_STABILIZED_GAME_ROTATION_VECTOR: return 14;  //ID 0x29
        case SH2Constants::SENSOR_GYRO_INTEGRATED_ROTATION_VECTOR:      return 14;  //ID 0x2a // but 10 for angular velocity
        case SH2Constants::SENSOR_MOTION_REQUEST:                       return 0;   //ID 0x2b
        default: return 0;
    }
}

inline const char* sh2GetSensorName(unsigned int sensorReportID) {
    switch(sensorReportID) {
        case SH2Constants::SENSOR_ACCELEROMETER:                        return "Accelerometer";
        case SH2Constants::SENSOR_GYROSCOPE:                            return "Gyroscope";
        case SH2Constants::SENSOR_MAGNETOMETER:                         return "Magnetometer";
        case SH2Constants::SENSOR_LINEAR_ACCELERATION:                  return "Linear Acceleration";
        case SH2Constants::SENSOR_ROTATION_VECTOR:                      return "Rotation Vector";
        case SH2Constants::SENSOR_GRAVITY:                              return "Gravity";
        case SH2Constants::SENSOR_GYROSCOPE_UNCALIBRATED:               return "Gyroscope Uncalibrated";
        case SH2Constants::SENSOR_GAME_ROTATION_VECTOR:                 return "Game Rotation Vector";
        case SH2Constants::SENSOR_GEOMAGNETIC_ROTATION:                 return "Geomagnetic Rotation";
        case SH2Constants::SENSOR_PRESSURE:                             return "Pressure";
        case SH2Constants::SENSOR_AMBIENT_LIGHT:                        return "Ambient Light";
        case SH2Constants::SENSOR_HUMIDITY:                             return "Humidity";
        case SH2Constants::SENSOR_PROXIMITY:                            return "Proximity";
        case SH2Constants::SENSOR_TEMPERATURE:                          return "Temperature";
        case SH2Constants::SENSOR_MAGNETOMETER_UNCALIBRATED:            return "Magnetometer Uncalibrated";
        case SH2Constants::SENSOR_TAP_DETECTOR:                         return "Tap Detector";
        case SH2Constants::SENSOR_STEP_COUNTER:                         return "Step Counter";
        case SH2Constants::SENSOR_SIGNIFICANT_MOTION:                   return "Significant Motion";
        case SH2Constants::SENSOR_STABILITY_CLASSIFIER:                 return "Stability Classifier";
        case SH2Constants::SENSOR_ACCELEROMETER_RAW:                    return "Accelerometer Raw";
        case SH2Constants::SENSOR_GYROSCOPE_RAW:                        return "Gyroscope Raw";
        case SH2Constants::SENSOR_MAGNETOMETER_RAW:                     return "Magnetometer Raw";
        case SH2Constants::SENSOR_STEP_DETECTOR:                        return "Step Detector";
        case SH2Constants::SENSOR_SHAKE_DETECTOR:                       return "Shake Detector";
        case SH2Constants::SENSOR_FLIP_DETECTOR:                        return "Flip Detector";
        case SH2Constants::SENSOR_PICKUP_DETECTOR:                      return "Pickup Detector";
        case SH2Constants::SENSOR_STABILITY_DETECTOR:                   return "Stability Detector";
        case SH2Constants::SENSOR_PERSONAL_ACTIVITY_CLASSIFIER:         return "Personal Activity Classifier";
        case SH2Constants::SENSOR_SLEEP_DETECTOR:                       return "Sleep Detector";
        case SH2Constants::SENSOR_TILT_DETECTOR:                        return "Tilt Detector";
        case SH2Constants::SENSOR_POCKET_DETECTOR:                      return "Pocket Detector";
        case SH2Constants::SENSOR_CIRCLE_DETECTOR:                      return "Circle Detector";
        case SH2Constants::SENSOR_HEART_RATE_MONITOR:                   return "Heart Rate Monitor";
        case SH2Constants::SENSOR_ARVR_STABILIZED_ROTATION_VECTOR:      return "ARVR-Stabilized Rotation Vector";
        case SH2Constants::SENSOR_ARVR_STABILIZED_GAME_ROTATION_VECTOR: return "ARVR-Stabilized Game Rotation Vector";
        case SH2Constants::SENSOR_GYRO_INTEGRATED_ROTATION_VECTOR:      return "Gyro-Integrated Rotation Vector";
        case SH2Constants::SENSOR_MOTION_REQUEST:                       return "Motion Request";
        default: return "UNKNOWN";
    }
}

// Convenience function to return the appropriate unit string, if applicable
inline const char* sh2GetSensorUnit(unsigned int sensorReportID) {
    switch(sensorReportID) {
        case SH2Constants::SENSOR_ACCELEROMETER:                                        //ID 0x01
        case SH2Constants::SENSOR_LINEAR_ACCELERATION:                                  //ID 0x04
        case SH2Constants::SENSOR_GRAVITY:                              return "m/s²";  //ID 0x06
        case SH2Constants::SENSOR_GYROSCOPE:                                            //ID 0x02
        case SH2Constants::SENSOR_GYROSCOPE_UNCALIBRATED:               return "rad/s"; //ID 0x07
        case SH2Constants::SENSOR_MAGNETOMETER:                                         //ID 0x03
        case SH2Constants::SENSOR_MAGNETOMETER_UNCALIBRATED:            return "μT";    //ID 0x0f
        case SH2Constants::SENSOR_PRESSURE:                             return "hPa";   //ID 0x0a
        case SH2Constants::SENSOR_AMBIENT_LIGHT:                        return "lx";    //ID 0x0b "cd/m²"
        case SH2Constants::SENSOR_HUMIDITY:                             return "%";     //ID 0x0c
        case SH2Constants::SENSOR_PROXIMITY:                            return "cm";    //ID 0x0d
        case SH2Constants::SENSOR_TEMPERATURE:                          return "°C";    //ID 0x0e
        default: return "";
    }
}

inline const char* sh2GetCommandName(unsigned int cmdID) {
    static const char* cmdNames[] = {"Reserved", "Errors", "Counter", "Tare", "Initialize",
                "Reserved", "Save DCD", "ME Calibration", "Reserved", "Periodic DCD Save", "Get Oscillator Type",
                "Clear DCD and Reset", "Calibration", "Bootloader", "Interactive Calibration"};
    if (cmdID < sizeof(cmdNames)) return cmdNames[cmdID];
    else return "Unknown";
}

#pragma pack(push,1) // Packed struct definitions from SH-2, co-opted for transfer

// Common prefix for all SH-2 cargos. SHTP headers irrelevant and not represented.
class SH2CargoBase {
private:
    uint8_t  cargoLength[2];
    uint8_t  channel;
    uint8_t  sequenceNumber;
    uint8_t  reportType;
public:
    inline uint16_t getCargoLength() const { return sh2GetU16(cargoLength) & 0x7fff; } // mask out subtransfer bit
    inline uint8_t getChannel() const { return channel; }
    inline uint8_t getSequenceNumber() const { return sequenceNumber; }
    inline uint8_t getReportType() const { return reportType; }
};

// Our own custom extension for sending the raw interrupt timestamp (report 0xFF, never reported by SH2)
class SH2CargoBodyScenescanTimestamp {
private:
    SH2CargoBase base;
    uint8_t  usecSinceEpoch[8]; // 64-bit microsecond count
public:
    inline uint64_t getUSecSinceEpoch() const { return (uint64_t) sh2GetU64(usecSinceEpoch); }
};

// A Timestamp Rebase (0xFA), reporting additional sensor delay offset since last Timebase
class SH2CargoBodyTimestampRebase {
private:
    SH2CargoBase base;
    uint8_t  rebaseTime[4];
public:
    inline long getRebaseTime() const { return (int32_t) sh2GetU32(rebaseTime); }
};

// A Time Base report with a batch transfer (0xFB)
//   It may be followed by any amount of sensor reports, below.
//   Refer to the base.getCargoLength() value and the known
//   record sizes for parsing them.
class SH2CargoBodyTimeBase {
private:
    SH2CargoBase base;
    uint8_t  timeBase_100uSec[4];
public:
    inline long getTimeBase() const { return 100l * sh2GetU32(timeBase_100uSec); }
};

// Common base prefix for all sensor reports
class SH2SensorReportBase {
private:
    uint8_t  sensorID;
    uint8_t  sequenceNumber;
    uint8_t  statusAndDelayMSB;
    uint8_t  delayLSB;
public:
    inline unsigned int getStatus() const { return statusAndDelayMSB & 0x03; }
    inline unsigned int getDelay() const { return ((statusAndDelayMSB & 0xfc) << 6) | delayLSB; }
};

// 10-byte reports with individual Q scaling for non-raw 3D sensors
class SH2SensorReportAccelerometer {
private:
    SH2SensorReportBase base;
    uint8_t  xAxis[2];
    uint8_t  yAxis[2];
    uint8_t  zAxis[2];
public:
    inline double getX() const { return sh2ConvertFixedQ16(sh2GetU16(xAxis), 8); } // Accel Q: shift 8 bits
    inline double getY() const { return sh2ConvertFixedQ16(sh2GetU16(yAxis), 8); }
    inline double getZ() const { return sh2ConvertFixedQ16(sh2GetU16(zAxis), 8); }
};
class SH2SensorReportMagnetometer {
private:
    SH2SensorReportBase base;
    uint8_t  xAxis[2];
    uint8_t  yAxis[2];
    uint8_t  zAxis[2];
public:
    inline double getX() const { return sh2ConvertFixedQ16(sh2GetU16(xAxis), 4); } // Magn Q: shift 4 bits
    inline double getY() const { return sh2ConvertFixedQ16(sh2GetU16(yAxis), 4); }
    inline double getZ() const { return sh2ConvertFixedQ16(sh2GetU16(zAxis), 4); }
};
class SH2SensorReportGyroscope {
private:
    SH2SensorReportBase base;
    uint8_t  xAxis[2];
    uint8_t  yAxis[2];
    uint8_t  zAxis[2];
public:
    inline double getX() const { return sh2ConvertFixedQ16(sh2GetU16(xAxis), 9); } // Gyro Q: shift 9 bits
    inline double getY() const { return sh2ConvertFixedQ16(sh2GetU16(yAxis), 9); }
    inline double getZ() const { return sh2ConvertFixedQ16(sh2GetU16(zAxis), 9); }
};

// 14-byte orientation (quaternion) data. i,j,k,real are also known as x,y,z,w
class SH2SensorReportOrientation {
private:
    SH2SensorReportBase base;
    uint8_t  quatI[2];
    uint8_t  quatJ[2];
    uint8_t  quatK[2];
    uint8_t  quatReal[2];
    uint8_t  accuracy[2];
public:
    inline double getI() const { return sh2ConvertFixedQ16(sh2GetU16(quatI), 14); } // Quaternion data: shift 14 bits
    inline double getJ() const { return sh2ConvertFixedQ16(sh2GetU16(quatJ), 14); }
    inline double getK() const { return sh2ConvertFixedQ16(sh2GetU16(quatK), 14); }
    inline double getReal() const { return sh2ConvertFixedQ16(sh2GetU16(quatReal), 14); }
    inline double getAccuracy() const { return sh2ConvertFixedQ16(sh2GetU16(accuracy), 12); } // Accuracy: shift 12
};

// 6-byte 1D sensor (pressure, ambient light, humidity, proximity, temperature,
// 16-byte data for *raw* accelerometer, gyro, magnetometer
class SH2SensorReportRawAGM {
private:
    SH2SensorReportBase base;
    uint8_t  xAxisRaw[2];
    uint8_t  yAxisRaw[2];
    uint8_t  zAxisRaw[2];
    uint8_t  temperature_forGyro[2];
    uint8_t  timestamp[4];
};

#pragma pack(pop)  // End of common sensor data / transport packed struct definitions

}} // namespaces

#endif

