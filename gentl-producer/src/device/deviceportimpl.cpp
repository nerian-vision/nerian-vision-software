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

#include "device/deviceportimpl.h"
#include "device/logicaldevice.h"
#include "device/physicaldevice.h"
#include "stream/datastream.h"
#include "misc/infoquery.h"

#include <iostream> // DEBUG
#include <fstream>

using namespace visiontransfer;

namespace GenTL {

#ifdef ENABLE_DEBUGGING
#ifdef _WIN32
    std::fstream debugStreamDevPort("C:\\debug\\gentl-debug-devport-" + std::to_string(time(nullptr)) + ".txt", std::ios::out);
#else
    std::ostream& debugStreamDevPort = std::cout;
#endif
#define DEBUG_DEVPORT(x) debugStreamDevPort << x << std::endl;
#else
#define DEBUG_DEVPORT(x) ;
#endif

DevicePortImpl::DevicePortImpl(LogicalDevice* device)
    :device(device) {
    // Set up child selector defaults
    currentSelectorForBalanceRatio = 1;
    currentSelectorForExposure = 0;
    currentSelectorForGain = 0;
    currentIndexForQMatrix = 0;
}

GC_ERROR DevicePortImpl::readFeature(unsigned int featureId, void* pBuffer, size_t* piSize) {
    INFO_DATATYPE type;
    return device->getInfo(featureId, &type, pBuffer, piSize);
}

GC_ERROR DevicePortImpl::writeSelector(unsigned int selector) {
    if(selector < 0 || selector > 7) {
        return GC_ERR_INVALID_INDEX;
    } else {
        return GC_ERR_SUCCESS;
    }
}

GC_ERROR DevicePortImpl::readChildFeature(unsigned int selector, unsigned int featureId,
        void* pBuffer, size_t* piSize) {

    InfoQuery info(nullptr, pBuffer, piSize);

    std::string id = device->getId();
    const ImageSet& metaData = device->getPhysicalDevice()->getLatestMetaData();
    bool hasIntensityStream = metaData.hasImageType(ImageSet::IMAGE_LEFT) || metaData.hasImageType(ImageSet::IMAGE_COLOR);
    bool hasDisparityStream = metaData.hasImageType(ImageSet::IMAGE_DISPARITY);

    // Possible values of the component selector
    enum ComponentSelector {
        Intensity,
        Disparity,
        Range
    };

    DataStream::StreamType streamType = device->getStream()->getStreamType();

    switch(featureId) {
        // Common device info, not related to component selector
        case 0: // width
            info.setUInt(metaData.getWidth());
            break;
        case 1: // height
            info.setUInt(metaData.getHeight());
            break;
        case 2: // Pixelformat
            info.setUInt(static_cast<unsigned int>(device->getStream()->getPixelFormat(
                device->getPhysicalDevice()->getLatestMetaData())));
            break;
        case 3: // Payload size
            info.setUInt(static_cast<unsigned int>(device->getStream()->getPayloadSize()));
            break;

        // Component selector specific info
        case 4:  // ComponentEnable
            {
                const unsigned int INTENSITY_BIT = 1;
                const unsigned int DISPARITY_BIT = 2;
                const unsigned int RANGE_BIT     = 4;
                bool rangeEnabled = device->getPhysicalDevice()->getComponentEnabledRange();
                if(streamType == DataStream::MULTIPART_STREAM) { // Multipart
                    DEBUG_DEVPORT("Reporting ComponentEnabledReg as " << ((hasIntensityStream?INTENSITY_BIT:0) + ((rangeEnabled && hasDisparityStream) ? RANGE_BIT:0) + (hasDisparityStream?DISPARITY_BIT:0)))
                    info.setUInt((hasIntensityStream?INTENSITY_BIT:0) + ((rangeEnabled && hasDisparityStream) ? RANGE_BIT:0) + (hasDisparityStream?DISPARITY_BIT:0));
                } else if(streamType == DataStream::IMAGE_LEFT_STREAM || streamType == DataStream::IMAGE_RIGHT_STREAM || streamType== DataStream::IMAGE_THIRD_COLOR_STREAM) {
                    info.setUInt(INTENSITY_BIT);
                } else if(streamType == DataStream::DISPARITY_STREAM) {
                    info.setUInt(DISPARITY_BIT);
                } else if(streamType == DataStream::POINTCLOUD_STREAM) {
                    info.setUInt(RANGE_BIT);
                } else {
                    info.setUInt(0);
                }
            }
            break;
        case 5: // Id
            if(streamType != DataStream::MULTIPART_STREAM) {
                info.setUInt(0); // Only one component
            } else {
                switch(selector) {
                    case Intensity:
                        info.setUInt(0);
                        break;
                    case Disparity:
                        info.setUInt(1);
                        break;
                    case Range:
                        info.setUInt(2);
                        break;
                    default:
                        info.setUInt(0);
                }
            }
            break;

        case 6: // Scan3dFocalLengthReg
            {
                const ImageSet& metaData = device->getPhysicalDevice()->getLatestMetaData();
                const float* q = metaData.getQMatrix();
                info.setDouble(q[11]);
            }
            break;

        case 7: // Scan3dBaselineReg
            {
                const ImageSet& metaData = device->getPhysicalDevice()->getLatestMetaData();
                const float* q = metaData.getQMatrix();
                info.setDouble(1.0 / q[14]);
            }
            break;
        case 8: // Scan3dInvalidDataValueReg (now depends on stream type and current selector for multi-part stream)
            {
                float inval_disp = 4095;
                float inval_depth = device->getPhysicalDevice()->getInvalidDepthValue();
                if (streamType == DataStream::DISPARITY_STREAM) {
                    info.setDouble(inval_disp);
                } else if (streamType == DataStream::POINTCLOUD_STREAM) {
                    info.setDouble(inval_depth);
                } else if (streamType == DataStream::MULTIPART_STREAM) {
                    switch(selector) {
                        case Intensity:
                            info.setDouble(-1); // dummy
                            break;
                        case Disparity:
                            info.setDouble(inval_disp);
                            break;
                        case Range:
                            info.setDouble(inval_depth);
                            break;
                        default:
                            info.setDouble(-1);
                    }
                } else {
                    // One of the intensity streams: dummy value (flag (reg 0x1d) is always 0 here)
                    info.setDouble(-1);
                }
            }
            break;
        case 9: // Scan3dPrincipalPointUReg (X offset)
            {
                const ImageSet& metaData = device->getPhysicalDevice()->getLatestMetaData();
                const float* q = metaData.getQMatrix();
                info.setDouble(-q[3]);
            }
            break;
        case 0xa: // Scan3dPrincipalPointVReg (Y offset)
            {
                const ImageSet& metaData = device->getPhysicalDevice()->getLatestMetaData();
                const float* q = metaData.getQMatrix();
                info.setDouble(-q[7]);
            }
            break;
        case 0xb: // Exposure: current manual exposure time
            {
                DEBUG_DEVPORT("Getting current exposure time");
                auto param = device->getPhysicalDevice()->getParameter(currentSelectorForExposure?"manual_exposure_time_color":"manual_exposure_time");
                info.setDouble(param.getCurrent<double>());
            }
            break;
        case 0xc: // Exposure: min manual exposure time
            {
                auto param = device->getPhysicalDevice()->getParameter(currentSelectorForExposure?"manual_exposure_time_color":"manual_exposure_time");
                info.setDouble(param.getMin<double>());
            }
            break;
        case 0xd: // Exposure: max manual exposure time
            {
                auto param = device->getPhysicalDevice()->getParameter(currentSelectorForExposure?"manual_exposure_time_color":"manual_exposure_time");
                info.setDouble(param.getMax<double>());
            }
            break;
        case 0xe: // Exposure: auto exposure mode
            {
                auto param = device->getPhysicalDevice()->getParameter("auto_exposure_mode");
                bool autoExposureActive = (param.getCurrent<int>() < 2); // 0 or 1: auto exposure active; 2 or 3: inactive
                info.setInt(autoExposureActive ? 2 : 0); // 0: Off; 2: Continuous
            }
            break;
        case 0xf: // Gain: current manual gain
            {
                auto param = device->getPhysicalDevice()->getParameter(currentSelectorForGain?"manual_gain_color":"manual_gain");
                info.setDouble(param.getCurrent<double>());
            }
            break;
        case 0x10: // Gain: min manual gain
            {
                auto param = device->getPhysicalDevice()->getParameter(currentSelectorForGain?"manual_gain_color":"manual_gain");
                info.setDouble(param.getMin<double>());
            }
            break;
        case 0x11: // Gain: max manual gain
            {
                auto param = device->getPhysicalDevice()->getParameter(currentSelectorForGain?"manual_gain_color":"manual_gain");
                info.setDouble(param.getMax<double>());
            }
            break;
        case 0x12: // Gain: auto gain mode
            {
                auto param = device->getPhysicalDevice()->getParameter("auto_exposure_mode");
                bool autoGainActive = (param.getCurrent<int>() & 0x01) == 0;
                info.setInt(autoGainActive ? 2 : 0); // 0: Off; 2: Continuous
            }
            break;
        case 0x13: // White balance: selector for manual balance (red or blue)
            {
                info.setInt(currentSelectorForBalanceRatio);
            }
            break;
        case 0x14: // White balance: current manual balance ratio
            {
                DEBUG_DEVPORT("Reading current white balance ratio register, selector is " << ((currentSelectorForBalanceRatio==1)?"1 (Red)":"3 (Blue)"));
                const char* uid = (currentSelectorForBalanceRatio==1)?"white_balance_factor_red":"white_balance_factor_blue";
                auto param = device->getPhysicalDevice()->getParameter(uid);
                info.setDouble(param.getCurrent<double>());
            }
            break;
        case 0x15: // White balance: min manual balance ratio
            {
                info.setDouble(0.0); // For current devices
            }
            break;
        case 0x16: // White balance: max manual balance ratio
            {
                info.setDouble(4.0); // For current devices
            }
            break;
        case 0x17: // White balance: auto balance mode
            {
                auto param = device->getPhysicalDevice()->getParameter("white_balance_mode");
                bool autoBalanceActive = param.getCurrent<int>() != 0;
                info.setInt(autoBalanceActive ? 2 : 0); // 0: Off; 2: Continuous
            }
            break;
        case 0x18: // IntensitySourceReg (which camera to use for multipart 'Intensity' on 3-camera devices)
            {
                info.setInt((int) device->getPhysicalDevice()->getIntensitySource());
                break;
            }
        case 0x19: // ExposureTimeSelector: selector for setting stereo pair vs. color cam exposure
            {
                info.setInt(currentSelectorForExposure);
            }
            break;
        case 0x1a: // GainSelector: selector for setting stereo pair vs. color cam gain
            {
                info.setInt(currentSelectorForGain);
            }
            break;
        case 0x1b: // QMatrixIndexReg: index register for Q matrix access
            {
                info.setInt(currentIndexForQMatrix);
            }
            break;
        case 0x1c: // QMatrixData: data port for Q matrix element access
            {
                const ImageSet& metaData = device->getPhysicalDevice()->getLatestMetaData();
                const float* qMat = metaData.getQMatrix();
                info.setDouble((double) qMat[currentIndexForQMatrix]);
            }
            break;
        case 0x1d: // Scan3dInvalidDataFlagReg (whether a dedicated 'invalid' value is available)
            {
                if (streamType == DataStream::DISPARITY_STREAM) {
                    info.setUInt(1); // available
                } else if (streamType == DataStream::POINTCLOUD_STREAM) {
                    info.setUInt(1);
                } else if (streamType == DataStream::MULTIPART_STREAM) {
                    switch(selector) {
                        case Intensity:
                            info.setUInt(0); // not available for intensity
                            break;
                        case Disparity:
                            info.setUInt(1);
                            break;
                        case Range:
                            info.setUInt(1);
                            break;
                        default:
                            info.setUInt(0);
                    }
                } else {
                    // One of the intensity streams: not available
                    info.setUInt(0);
                }
            }
            break;
        case 0xff: // Nerian device feature map (used to mask the availability of other features via the XML) (DeviceFeatureReg)
            {
                uint32_t featureMap = 0;
                // Bit 0: Availability of third camera
                int numCameras = device->getPhysicalDevice()->getParameter("calib_num_cameras").getCurrent<int>();
                featureMap |= (numCameras>2) ? 1 : 0;
                // Bit 1: Availability of Nerian software white balance
                featureMap |= device->getPhysicalDevice()->hasParameter("white_balance_mode") ? 2 : 0;
                // Feature bitmap complete
                DEBUG_DEVPORT("Device feature bit map: " << featureMap);
                info.setInt(featureMap);
            }
            break;
        default:
            return GC_ERR_INVALID_ADDRESS;
    }

    return info.query();
}

GC_ERROR DevicePortImpl::writeChildFeature(unsigned int selector, unsigned int featureId,
        const void* pBuffer, size_t* piSize) {

    std::string id = device->getId();
    const ImageSet& metaData = device->getPhysicalDevice()->getLatestMetaData();
    bool hasIntensityStream = metaData.hasImageType(ImageSet::IMAGE_LEFT) || metaData.hasImageType(ImageSet::IMAGE_COLOR);
    bool hasDisparityStream = metaData.hasImageType(ImageSet::IMAGE_DISPARITY);

    // Possible values of the component selector
    enum ComponentSelector {
        Intensity,
        Disparity,
        Range
    };

    DataStream::StreamType streamType = device->getStream()->getStreamType();

    switch(featureId) {
        // Component selector specific info
        case 4:  // ComponentEnable
            {
                const unsigned int INTENSITY_BIT = 1;
                const unsigned int DISPARITY_BIT = 2;
                const unsigned int RANGE_BIT     = 4;
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                DEBUG_DEVPORT("New raw value for ComponentEnableReg is " << newVal);
                if (selector == ComponentSelector::Range) {
                    bool enableRange = (newVal & RANGE_BIT) != 0;
                    if (enableRange && !hasDisparityStream) {
                        DEBUG_DEVPORT("NOT AVAILABLE - cannot enable Range when there is no disparity in the data stream");
                        return GC_ERR_NOT_AVAILABLE;
                    } else {
                        DEBUG_DEVPORT("Setting ComponentEnable for Range to " << enableRange);
                        device->getPhysicalDevice()->setComponentEnabledRange(enableRange);
                        return GC_ERR_SUCCESS;
                    }
                } else {
                    DEBUG_DEVPORT("NOT AVAILABLE - TODO - write ComponentEnable for selector " << selector);
                    return GC_ERR_NOT_AVAILABLE;
                }
            }
            break;
        case 0xb: // Exposure: set manual exposure time
            {
                if (*piSize != 8) throw std::runtime_error("Expected a new feature value of size 8");
                double newVal = (reinterpret_cast<const double*>(pBuffer))[0];
                DEBUG_DEVPORT("Set manual exposure time, new val " << newVal);
                device->getPhysicalDevice()->setParameter(currentSelectorForExposure?"manual_exposure_time_color":"manual_exposure_time", newVal);
                return GC_ERR_SUCCESS;
            }
        case 0xe: // Exposure: set auto exposure mode
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                DEBUG_DEVPORT("Set auto exposure mode, new val " << newVal);
                int oldMode = device->getPhysicalDevice()->getParameter("auto_exposure_mode").getCurrent<int>();
                // Update auto exposure flag, preserve auto gain flag
                device->getPhysicalDevice()->setParameter("auto_exposure_mode", (newVal?0:2) | (oldMode & 1));
                return GC_ERR_SUCCESS;
            }
        case 0xf: // Gain: set manual gain
            {
                if (*piSize != 8) throw std::runtime_error("Expected a new feature value of size 8");
                double newVal = (reinterpret_cast<const double*>(pBuffer))[0];
                DEBUG_DEVPORT("Set manual gain, new val " << newVal);
                device->getPhysicalDevice()->setParameter(currentSelectorForGain?"manual_gain_color":"manual_gain", newVal);
                return GC_ERR_SUCCESS;
            }
        case 0x12: // Gain: set auto gain mode
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                DEBUG_DEVPORT("Set auto gain mode, new val " << newVal);
                int oldMode = device->getPhysicalDevice()->getParameter("auto_exposure_mode").getCurrent<int>();
                // Update auto gain flag, preserve auto exposure flag
                device->getPhysicalDevice()->setParameter("auto_exposure_mode", (newVal?0:1) | (oldMode & 2));
                return GC_ERR_SUCCESS;
            }
        case 0x13: // White balance: selector for manual balance channel (red or blue)
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                currentSelectorForBalanceRatio = (newVal==1)?1:3; // Red or Blue
                DEBUG_DEVPORT("White balance ratio selector is now " << currentSelectorForBalanceRatio);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x14: // White balance: current manual balance ratio
            {
                if (*piSize != 8) throw std::runtime_error("Expected a new feature value of size 8");
                double newVal = (reinterpret_cast<const double*>(pBuffer))[0];
                const char* uid = (currentSelectorForBalanceRatio==1)?"white_balance_factor_red":"white_balance_factor_blue";
                device->getPhysicalDevice()->setParameter(uid, newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x17: // White balance: auto balance mode
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                DEBUG_DEVPORT("Set auto white balance mode, new val " << newVal);
                device->getPhysicalDevice()->setParameter("white_balance_mode", newVal ? 1 : 0);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x18: // IntensitySourceReg (which camera to use for multipart 'Intensity' on 3-camera devices)
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                DEBUG_DEVPORT("Set intensity source mode to " << newVal);
                device->getPhysicalDevice()->setIntensitySource((PhysicalDevice::IntensitySource) newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x19: // ExposureTimeSelector: selector for setting stereo pair vs. color cam exposure
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                currentSelectorForExposure = (newVal)?1:0;
                DEBUG_DEVPORT("Exposure selector is now " << currentSelectorForExposure);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x1a: // GainSelector: selector for setting stereo pair vs. color cam gain
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                currentSelectorForGain = (newVal)?1:0;
                DEBUG_DEVPORT("Exposure selector is now " << currentSelectorForGain);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x1b: // QMatrixIndexReg: index register for Q matrix access
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                if ((newVal < 0) || (newVal > 15)) newVal = 0;
                currentIndexForQMatrix = newVal;
                DEBUG_DEVPORT("Index into Q matrix is now" << currentIndexForQMatrix);
                return GC_ERR_SUCCESS;
            }
            break;
        // (0x1c QMatrixData is read-only)
        default:
            DEBUG_DEVPORT("TODO - implement me (DevPortImpl::writeChildFeature)");
    }
    return GC_ERR_NOT_AVAILABLE;
}

}
