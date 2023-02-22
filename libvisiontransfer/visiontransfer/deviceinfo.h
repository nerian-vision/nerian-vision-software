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

#ifndef VISIONTRANSFER_DEVICEINFO_H
#define VISIONTRANSFER_DEVICEINFO_H

#include <string>

namespace visiontransfer {

/**
 * \brief Representation of the current device status / health.
 *   Useful for addressing issues with peripherals or network transport.
 */
class DeviceStatus {
public:
    DeviceStatus()
        : lastFps(0.0), jumboSize(0), currentCaptureSource(""), validStatus(false) { }
    DeviceStatus(double lastFps, unsigned int jumboSize, const std::string& currentCaptureSource)
        : lastFps(lastFps), jumboSize(jumboSize), currentCaptureSource(currentCaptureSource), validStatus(true) { }
    bool isValid() const { return validStatus; }
    double getLastFps() const { return lastFps; }
    unsigned int getJumboMtu() const { return jumboSize; }
    unsigned int getJumboFramesEnabled() const { return jumboSize > 0; }
    std::string getCurrentCaptureSource() const { return currentCaptureSource; }
private:
    double lastFps; // Most recent FPS report, or 0.0 if N/A
    unsigned int jumboSize; // Jumbo MTU, or 0 if Jumbo mode disabled
    std::string currentCaptureSource; // for targeted instructions
    bool validStatus; // whether the status record contains actual data
};

/**
 * \brief Aggregates information about a discovered device
 */
class DeviceInfo {
public:
    enum DeviceModel {
        SCENESCAN,
        SCENESCAN_PRO,
        SCARLET,
        RUBY
    };

    enum NetworkProtocol {
        PROTOCOL_TCP,
        PROTOCOL_UDP
    };

    /**
     * \brief Constructs an empty object with default information
     */
    DeviceInfo(): ip(""), protocol(PROTOCOL_TCP), fwVersion(""), model(SCENESCAN),
        compatible(false) {
    }

    /**
     * \brief Constructs an object by initializing all members with data
     * from the given parameters
     *
     * \param ip IP address of the discovered device.
     * \param protocol Network protocol of the discovered device.
     * \param fwVersion Firmware version as string.
     * \param model Model of the discovered device
     * \param compatible Indicates if the device is compatible with this
     *        API version.
     */
    DeviceInfo(const char* ip, NetworkProtocol protocol, const char* fwVersion,
         DeviceModel model, bool compatible)
        : ip(ip), protocol(protocol), fwVersion(fwVersion), model(model),
        compatible(compatible) {
    }

    /**
     * \brief Construct DeviceInfo with pre-initialized DeviceStatus field, for received health reports
     */
    DeviceInfo(const char* ip, NetworkProtocol protocol, const char* fwVersion,
         DeviceModel model, bool compatible, const DeviceStatus& status)
        : ip(ip), protocol(protocol), fwVersion(fwVersion), model(model),
        compatible(compatible), status(status){
    }

    /**
     * \brief Gets the IP address of the device.
     * \return Device IP address.
     */
    std::string getIpAddress() const {return ip;}

    /**
     * \brief Gets the network protocol of the device.
     * \return Device network protocol.
     *
     * Possible network protocols are \c PROTOCOL_TCP or \c PROTOCOL_UDP.
     */
    NetworkProtocol getNetworkProtocol() const {return protocol;}

    /**
     * \brief Gets the firmware version of the device.
     * \return Firmware version encoded as string.
     *
     * A firmware version string typically consists of a major, minor
     * and patch version, like for example "1.2.34". For special
     * firmware releases, however, the firmware string might deviate.
     */
    std::string getFirmwareVersion() const {return fwVersion;}

    /**
     * \brief Gets the model identifier of the discovered device.
     * \return The device model.
     *
     * Currently supported models are \c SCENESCAN, \c SCENESCAN_PRO,
     * \c SCARLET and \c RUBY.
     */
    DeviceModel getModel() const {return model;}

    /**
     * \brief Return the status / health as reported by the device
     */
    DeviceStatus getStatus() const { return status; }

    /**
     * \brief Returns true if the device is compatible with this API
     * version
     */
    bool isCompatible() const {return compatible;}

    /**
     * \brief Converts this object to a printable string.
     *
     * All information is concatenated into a readable string, which
     * can for example be printed to a terminal.
     */
    std::string toString() const {
        std::string ret = ip +  "; ";
        switch(model) {
            case SCENESCAN_PRO: ret += "SceneScan Pro"; break;
            case SCENESCAN: ret += "SceneScan"; break;
            case SCARLET: ret += "Scarlet"; break;
            case RUBY: ret += "Ruby"; break;
            default: ret += "Unknown"; break;
        }

        ret += "; " + fwVersion + "; " + (compatible ? "compatible" : "incompatible");
        return ret;
    }

    /**
     * \brief Comparison operator for comparing two DeviceInfo objects.
     */
    bool operator == (const DeviceInfo& other) const {
        return ip == other.ip && protocol == other.protocol && fwVersion == other.fwVersion
            && model == other.model && compatible == other.compatible;
    }

private:
    std::string ip;
    NetworkProtocol protocol;
    std::string fwVersion;
    DeviceModel model;
    bool compatible;
    // Extended device status / health info
    DeviceStatus status;
};

} // namespace

#endif
