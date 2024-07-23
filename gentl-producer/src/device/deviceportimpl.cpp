/*******************************************************************************
 * Copyright (c) 2023 Allied Vision Technologies GmbH
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

#include <algorithm>

#include <iostream> // DEBUG
#include <iomanip>
#include <fstream>
#include <chrono>
#include <thread>
#include <bitset>

using namespace visiontransfer;

namespace GenTL {

#ifdef ENABLE_DEBUGGING
#ifdef _WIN32
    std::fstream debugStreamDevPort("C:\\debug\\gentl-debug-devport-" + std::to_string(time(nullptr)) + ".txt", std::ios::out);
#else
    std::ostream& debugStreamDevPort = std::cout;
#endif
std::chrono::system_clock::time_point debugStreamDevPortInitTime = std::chrono::system_clock::now();
#define DEBUG_DEVPORT_THREAD_ID " (thread " << std::this_thread::get_id() << ") "
#define DEBUG_DEVPORT(x) debugStreamDevPort << std::dec << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - debugStreamDevPortInitTime).count() << ": " << DEBUG_DEVPORT_THREAD_ID << x << std::endl;
#else
#define DEBUG_DEVPORT(x) ;
#endif

DevicePortImpl::DevicePortImpl(LogicalDevice* device)
    :device(device) {
    // Device features
    auto availTrigInputs = device->getPhysicalDevice()->getParameter("trigger_input").getOptions<int>();
    triggerInputAvailable = std::find(availTrigInputs.begin(), availTrigInputs.end(), 1) != availTrigInputs.end(); // '1' is HW trigger option
    // Set up child selector defaults
    currentSelectorForBalanceRatio = 1;
    currentSelectorForExposure = 0;
    currentSelectorForGain = 0;
    currentSelectorForLine = 0;
    currentSelectorForPulseWidth = 0;
    currentIndexForQMatrix = 0;
    preferredTriggerSource = 2; // SW trigger is preselected (available everywhere)
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
        case 0: // SensorWidth / WidthMax
            {
                int fullWidth = device->getPhysicalDevice()->getParameter("calib_image_size_full").at(0);
                info.setUInt(fullWidth);
            }
            break;
        case 1: // SensorHeight / HeightMax
            {
                int fullHeight = device->getPhysicalDevice()->getParameter("calib_image_size_full").at(1);
                info.setUInt(fullHeight);
            }
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
        case 0x1f: // Width
            {
                info.setUInt(metaData.getWidth()); // actual current frame width
            }
            break;
        case 0x20: // Width (min valid value; max is from WidthMax)
            {
                info.setUInt(device->getPhysicalDevice()->getParameter("RT_input_size_width_min").getCurrent<int>());
            }
            break;
        case 0x21: // Width (a-priori increment; valid res determined remotely)
            {
                info.setUInt(device->getPhysicalDevice()->getParameter("RT_input_size_width_inc").getCurrent<int>());
            }
            break;
        case 0x22: // Height
            {
                info.setUInt(metaData.getHeight()); // actual current frame height
            }
            break;
        case 0x23: // Height (min valid value; max is from HeightMax)
            {
                info.setUInt(device->getPhysicalDevice()->getParameter("RT_input_size_height_min").getCurrent<int>());
            }
            break;
        case 0x24: // Height (a-priori increment; valid res determined remotely)
            {
                info.setUInt(device->getPhysicalDevice()->getParameter("RT_input_size_height_inc").getCurrent<int>());
            }
            break;
        case 0x25: // OffsetX
            {
                // Device parameters use coordinates relative from center; translate
                auto dev = device->getPhysicalDevice();
                int ofsRel = dev->getParameter("RT_input_roi_ofs_left_x").getCurrent<int>();
                int maxRel = dev->getParameter("capture_cam0_roi_offset_x").getMax<int>();
                //DEBUG_DEVPORT("widthDiff: " << fullWidth << " - " << curWidth << " = " << widthDiff);
                //DEBUG_DEVPORT("ofsAbs:    " << widthDiff << "/2" << " + " << ofsRel << " = " << (widthDiff/2 + ofsRel));
                info.setUInt(ofsRel + maxRel);
            }
            break;
        case 0x26: // OffsetX (max valid value)
            {
                // Device parameters use coordinates relative from center (limits always symmetrical); translate
                int maxX = 2 * device->getPhysicalDevice()->getParameter("capture_cam0_roi_offset_x").getMax<int>();
                info.setUInt(maxX);
            }
            break;
        case 0x27: // OffsetX (increment)
            {
                int incX = device->getPhysicalDevice()->getParameter("capture_cam0_roi_offset_x").getIncrement<int>();
                info.setUInt(incX);
            }
            break;
        case 0x28: // OffsetY
            {
                // Device parameters use coordinates relative from center; translate
                auto dev = device->getPhysicalDevice();
                int ofsRel = dev->getParameter("RT_input_roi_ofs_left_y").getCurrent<int>();
                int maxRel = dev->getParameter("capture_cam0_roi_offset_y").getMax<int>();
                info.setUInt(ofsRel + maxRel);
            }
            break;
        case 0x29: // OffsetY (max valid value)
            {
                // Device parameters use coordinates relative from center (limits always symmetrical); translate
                int maxY = 2 * device->getPhysicalDevice()->getParameter("capture_cam0_roi_offset_y").getMax<int>();
                info.setUInt(maxY);
            }
            break;
        case 0x2a: // OffsetY (increment)
            {
                int incY = device->getPhysicalDevice()->getParameter("capture_cam0_roi_offset_y").getIncrement<int>();
                info.setUInt(incY);
            }
            break;
        case 0x2b: // AcquisitionFrameRate
            {
                int rate = device->getPhysicalDevice()->getParameter("trigger_frequency").getCurrent<double>();
                info.setDouble(rate);
            }
            break;
        case 0x2c: // AcquisitionFrameRate (max valid value)
            {
                int maxRate = device->getPhysicalDevice()->getParameter("trigger_frequency").getMax<double>();
                info.setDouble(maxRate);
            }
            break;
        case 0x2d: // TriggerMode
            {
                int activeTriggerSource = device->getPhysicalDevice()->getParameter("trigger_input").getCurrent<int>();
                info.setUInt(activeTriggerSource != 0);
            }
            break;
        case 0x2e: // TriggerSource
            {
                int activeTriggerSource = device->getPhysicalDevice()->getParameter("trigger_input").getCurrent<int>();
                info.setUInt(activeTriggerSource ? activeTriggerSource : preferredTriggerSource);
            }
            break;
        case 0x2f: // PatternProjectorBrightness
            {
                auto dev = device->getPhysicalDevice();
                if (dev->hasParameter("projector_brightness")) {
                    double bri = dev->getParameter("projector_brightness").getCurrent<double>();
                    info.setDouble(bri * 100.0);
                } else {
                    info.setDouble(0.0); // should not happen (feature also set as unavailable)
                }
            }
            break;
        case 0x30: // NumberOfDisparities
            {
                int num = device->getPhysicalDevice()->getParameter("number_of_disparities").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x31: // NumberOfDisparities (min)
            {
                int num = device->getPhysicalDevice()->getParameter("number_of_disparities").getMin<int>();
                info.setUInt(num);
            }
            break;
        case 0x32: // NumberOfDisparities (max)
            {
                int num = device->getPhysicalDevice()->getParameter("number_of_disparities").getMax<int>();
                info.setUInt(num);
            }
            break;
        case 0x33: // NumberOfDisparities (increment)
            {
                int num = device->getPhysicalDevice()->getParameter("number_of_disparities").getIncrement<int>();
                info.setUInt(num);
            }
            break;
        case 0x34: // DisparityOffset
            {
                int num = device->getPhysicalDevice()->getParameter("disparity_offset").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x35: // SgmP1NoEdge
            {
                int num = device->getPhysicalDevice()->getParameter("sgm_p1_no_edge").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x36: // SgmP1Edge
            {
                int num = device->getPhysicalDevice()->getParameter("sgm_p1_edge").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x37: // SgmP2NoEdge
            {
                int num = device->getPhysicalDevice()->getParameter("sgm_p2_no_edge").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x38: // SgmP2Edge
            {
                int num = device->getPhysicalDevice()->getParameter("sgm_p2_edge").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x39: // SgmEdgeSensitivity
            {
                int num = device->getPhysicalDevice()->getParameter("sgm_edge_sensitivity").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x3A: // SubpixelOptimizationROIEnabled
            {
                int num = device->getPhysicalDevice()->getParameter("subpixel_optimization_roi_enabled").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x3B: // SubpixelOptimizationROIWidth
            {
                int num = device->getPhysicalDevice()->getParameter("subpixel_optimization_roi_width").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x3C: // SubpixelOptimizationROIHeight
            {
                int num = device->getPhysicalDevice()->getParameter("subpixel_optimization_roi_height").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x3D: // SubpixelOptimizationROIOffsetX
            {
                int num = device->getPhysicalDevice()->getParameter("subpixel_optimization_roi_x").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x3E: // SubpixelOptimizationROIOffsetY
            {
                int num = device->getPhysicalDevice()->getParameter("subpixel_optimization_roi_y").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x3F: // MaskBorderPixelsEnabled
            {
                int num = device->getPhysicalDevice()->getParameter("mask_border_pixels_enabled").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x40: // ConsistencyCheckEnabled
            {
                int num = device->getPhysicalDevice()->getParameter("consistency_check_enabled").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x41: // ConsistencyCheckSensitivity
            {
                int num = device->getPhysicalDevice()->getParameter("consistency_check_sensitivity").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x42: // UniquenessCheckEnabled
            {
                int num = device->getPhysicalDevice()->getParameter("uniqueness_check_enabled").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x43: // UniquenessCheckSensitivity
            {
                int num = device->getPhysicalDevice()->getParameter("uniqueness_check_sensitivity").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x44: // TextureFilterEnabled
            {
                int num = device->getPhysicalDevice()->getParameter("texture_filter_enabled").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x45: // TextureFilterSensitivity
            {
                int num = device->getPhysicalDevice()->getParameter("texture_filter_sensitivity").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x46: // GapInterpolationEnabled
            {
                int num = device->getPhysicalDevice()->getParameter("gap_interpolation_enabled").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x47: // NoiseReductionEnabled
            {
                int num = device->getPhysicalDevice()->getParameter("noise_reduction_enabled").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x48: // SpeckleFilterIterations
            {
                int num = device->getPhysicalDevice()->getParameter("speckle_filter_iterations").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x49: // SpeckleFilterIterations (max valid value)
            {
                int num = device->getPhysicalDevice()->getParameter("speckle_filter_iterations").getMax<int>();
                info.setUInt(num);
            }
            break;
        case 0x4A: // InputPixelFormatsAvailable (selectable pixel formats for L/R cam acquisition)
            {
                // Encode available pixel formats in bit mask for XML to filter
                auto param = device->getPhysicalDevice()->getParameter("capture_pixel_format");
                if (param.hasOptions()) {
                    auto opts = param.getOptions<int>();
                    uint32_t mask;
                    mask =
                          ((std::find(opts.begin(), opts.end(), 0x01080001)!=opts.end())?(1<<0):0)  // Mono8
                        | ((std::find(opts.begin(), opts.end(), 0x01100005)!=opts.end())?(1<<1):0)  // Mono12
                        | ((std::find(opts.begin(), opts.end(), 0x010C0047)!=opts.end())?(1<<2):0)  // Mono12P
                        | ((std::find(opts.begin(), opts.end(), 0x010C0006)!=opts.end())?(1<<3):0)  // Mono12Packed
                        | ((std::find(opts.begin(), opts.end(), 0x02180014)!=opts.end())?(1<<4):0)  // RGB8
                        | ((std::find(opts.begin(), opts.end(), 0x01080008)!=opts.end())?(1<<5):0)  // BayerGR8
                        | ((std::find(opts.begin(), opts.end(), 0x01080009)!=opts.end())?(1<<6):0)  // BayerRG8
                        | ((std::find(opts.begin(), opts.end(), 0x0108000A)!=opts.end())?(1<<7):0)  // BayerGB8
                        | ((std::find(opts.begin(), opts.end(), 0x0108000B)!=opts.end())?(1<<8):0)  // BayerBG8
                    ;
                    info.setUInt(mask);
                } else {
                    // Parameter was not initialized with options yet - disable all for now
                    //  (this cannot happen outside daemon restart)
                    info.setUInt(0);
                }
            }
            break;
        case 0x4B: // InputPixelFormat (active pixel format for L/R cam acquisition)
            {
                int num = device->getPhysicalDevice()->getParameter("capture_pixel_format").getCurrent<int>();
                info.setUInt(num);
            }
            break;
        case 0x4D: // LineSelector
            {
                info.setUInt(currentSelectorForLine);
            }
            break;
        case 0x4E: // LineMode
            {
                // DEPRECATED - unused address - defined read-only via XML
                info.setUInt(0);
            }
            break;
        case 0x4F: // LineInverter
            {
                if (triggerInputAvailable) {
                    // Not SceneScan: LineSelector 0 is the only output pin, named trigger_1_...
                    if (currentSelectorForLine == 0) {
                        int val = device->getPhysicalDevice()->getParameter("trigger_1_polarity").getCurrent<int>();
                        info.setUInt(val ? 1 : 0);
                    } else {
                        // LineSelector 1 is the input pin - we report LineInverter=0 for sake of completeness
                        info.setUInt(0);
                    }
                } else {
                    // SceneScan: LineSelector 0 and 1 are trigger_0_... and trigger_1_..., respectively
                    if (currentSelectorForLine == 0) {
                        int val = device->getPhysicalDevice()->getParameter("trigger_0_polarity").getCurrent<int>();
                        info.setUInt(val ? 1 : 0);
                    } else {
                        int val = device->getPhysicalDevice()->getParameter("trigger_1_polarity").getCurrent<int>();
                        info.setUInt(val ? 1 : 0);
                    }
                }
            }
            break;
        case 0x50: // LineSource: signal source for output pins. FrameTrigger (if 'Enabled' in nvparam), Constant0/1 (if 'Disabled with constant' in nvparam)
            {
                if (triggerInputAvailable) {
                    // Not SceneScan: LineSelector 0 is the only output pin, named trigger_1_...
                    if (currentSelectorForLine == 0) {
                        int enabled = device->getPhysicalDevice()->getParameter("trigger_1_enabled").getCurrent<int>();
                        int constant = device->getPhysicalDevice()->getParameter("trigger_1_constant").getCurrent<int>();
                        info.setUInt(enabled ? 2 : constant); // 0, 1 = the respective value; 2 = FrameTrigger
                    } else {
                        // LineSelector 1 is the input pin - we report LineSource==FrameTrigger for sake of completeness
                        info.setUInt(2);
                    }
                } else {
                    // SceneScan: LineSelector 0 and 1 are trigger_0_... and trigger_1_..., respectively
                    if (currentSelectorForLine == 0) {
                        int enabled = device->getPhysicalDevice()->getParameter("trigger_0_enabled").getCurrent<int>();
                        int constant = device->getPhysicalDevice()->getParameter("trigger_0_constant").getCurrent<int>();
                        info.setUInt(enabled ? 2 : constant); // 0, 1 = the respective value; 2 = FrameTrigger
                    } else {
                        int enabled = device->getPhysicalDevice()->getParameter("trigger_1_enabled").getCurrent<int>();
                        int constant = device->getPhysicalDevice()->getParameter("trigger_1_constant").getCurrent<int>();
                        info.setUInt(enabled ? 2 : constant); // 0, 1 = the respective value; 2 = FrameTrigger
                    }
                }
            }
            break;
        case 0x51: // LinePulseWidthSelector (PulseWidth1..8)
            {
                info.setUInt(currentSelectorForPulseWidth);
            }
            break;
        case 0x52: // LinePulseWidthAlternationMode (Constant / Cycle; Constant restricts LinePulseWidthSelector to 1)
            if (triggerInputAvailable) {
                // Not SceneScan: LineSelector 0 is the only output pin, named trigger_1_...
                if (currentSelectorForLine == 0) {
                    int mode = device->getPhysicalDevice()->getParameter("trigger_1_pulse_width_mode").getCurrent<int>();
                    info.setUInt(mode ? 1 : 0);
                } else {
                    // LineSelector 1 is the input pin - we report Constant mode for sake of completeness
                    info.setUInt(0);
                }
            } else {
                // SceneScan: LineSelector 0 and 1 are trigger_0_... and trigger_1_..., respectively
                if (currentSelectorForLine == 0) {
                    int mode = device->getPhysicalDevice()->getParameter("trigger_0_pulse_width_mode").getCurrent<int>();
                    info.setUInt(mode ? 1 : 0);
                } else {
                    int mode = device->getPhysicalDevice()->getParameter("trigger_1_pulse_width_mode").getCurrent<int>();
                    info.setUInt(mode ? 1 : 0);
                }
            }
            break;
        case 0x53: // LinePulseWidth (trigger_XY_pulse_width, X LineSelector -> 0,1; Y LinePulseWidthSelector -> '','b'..'h')
            {
                int triggerIndex = -1;
                if (triggerInputAvailable) {
                    // Not SceneScan: LineSelector 0 is the only output pin, named trigger_1_...
                    if (currentSelectorForLine == 0) {
                        triggerIndex = 1;
                    } else {
                        // LineSelector 1 is the input pin - we report pulse width 0 for sake of completeness
                        info.setUInt(0);
                    }
                } else {
                    // SceneScan: LineSelector 0 and 1 are trigger_0_... and trigger_1_..., respectively
                    triggerIndex = currentSelectorForLine;
                }
                if (triggerIndex > -1) {
                    int val = 0;
                    if (triggerIndex == 0) {
                        switch (currentSelectorForPulseWidth) {
                            case 0: val = device->getPhysicalDevice()->getParameter("trigger_0_pulse_width").getCurrent<int>(); break;
                            case 1: val = device->getPhysicalDevice()->getParameter("trigger_0b_pulse_width").getCurrent<int>(); break;
                            case 2: val = device->getPhysicalDevice()->getParameter("trigger_0c_pulse_width").getCurrent<int>(); break;
                            case 3: val = device->getPhysicalDevice()->getParameter("trigger_0d_pulse_width").getCurrent<int>(); break;
                            case 4: val = device->getPhysicalDevice()->getParameter("trigger_0e_pulse_width").getCurrent<int>(); break;
                            case 5: val = device->getPhysicalDevice()->getParameter("trigger_0f_pulse_width").getCurrent<int>(); break;
                            case 6: val = device->getPhysicalDevice()->getParameter("trigger_0g_pulse_width").getCurrent<int>(); break;
                            case 7: val = device->getPhysicalDevice()->getParameter("trigger_0h_pulse_width").getCurrent<int>(); break;
                        };
                    } else {
                        switch (currentSelectorForPulseWidth) {
                            case 0: val = device->getPhysicalDevice()->getParameter("trigger_1_pulse_width").getCurrent<int>(); break;
                            case 1: val = device->getPhysicalDevice()->getParameter("trigger_1b_pulse_width").getCurrent<int>(); break;
                            case 2: val = device->getPhysicalDevice()->getParameter("trigger_1c_pulse_width").getCurrent<int>(); break;
                            case 3: val = device->getPhysicalDevice()->getParameter("trigger_1d_pulse_width").getCurrent<int>(); break;
                            case 4: val = device->getPhysicalDevice()->getParameter("trigger_1e_pulse_width").getCurrent<int>(); break;
                            case 5: val = device->getPhysicalDevice()->getParameter("trigger_1f_pulse_width").getCurrent<int>(); break;
                            case 6: val = device->getPhysicalDevice()->getParameter("trigger_1g_pulse_width").getCurrent<int>(); break;
                            case 7: val = device->getPhysicalDevice()->getParameter("trigger_1h_pulse_width").getCurrent<int>(); break;
                        };
                    }
                    info.setUInt(val);
                }
            }
            break;
        case 0x54: // LinePulseWidthMax (max values)
            {
                int triggerIndex = -1;
                if (triggerInputAvailable) {
                    // Not SceneScan: LineSelector 0 is the only output pin, named trigger_1_...
                    if (currentSelectorForLine == 0) {
                        triggerIndex = 1;
                    } else {
                        // LineSelector 1 is the input pin - we report pulse width 0 for sake of completeness
                        info.setUInt(0);
                    }
                } else {
                    // SceneScan: LineSelector 0 and 1 are trigger_0_... and trigger_1_..., respectively
                    triggerIndex = currentSelectorForLine;
                }
                if (triggerIndex > -1) {
                    int val = 0;
                    if (triggerIndex == 0) {
                        switch (currentSelectorForPulseWidth) {
                            case 0: val = device->getPhysicalDevice()->getParameter("trigger_0_pulse_width").getMax<int>(); break;
                            case 1: val = device->getPhysicalDevice()->getParameter("trigger_0b_pulse_width").getMax<int>(); break;
                            case 2: val = device->getPhysicalDevice()->getParameter("trigger_0c_pulse_width").getMax<int>(); break;
                            case 3: val = device->getPhysicalDevice()->getParameter("trigger_0d_pulse_width").getMax<int>(); break;
                            case 4: val = device->getPhysicalDevice()->getParameter("trigger_0e_pulse_width").getMax<int>(); break;
                            case 5: val = device->getPhysicalDevice()->getParameter("trigger_0f_pulse_width").getMax<int>(); break;
                            case 6: val = device->getPhysicalDevice()->getParameter("trigger_0g_pulse_width").getMax<int>(); break;
                            case 7: val = device->getPhysicalDevice()->getParameter("trigger_0h_pulse_width").getMax<int>(); break;
                        };
                    } else {
                        switch (currentSelectorForPulseWidth) {
                            case 0: val = device->getPhysicalDevice()->getParameter("trigger_1_pulse_width").getMax<int>(); break;
                            case 1: val = device->getPhysicalDevice()->getParameter("trigger_1b_pulse_width").getMax<int>(); break;
                            case 2: val = device->getPhysicalDevice()->getParameter("trigger_1c_pulse_width").getMax<int>(); break;
                            case 3: val = device->getPhysicalDevice()->getParameter("trigger_1d_pulse_width").getMax<int>(); break;
                            case 4: val = device->getPhysicalDevice()->getParameter("trigger_1e_pulse_width").getMax<int>(); break;
                            case 5: val = device->getPhysicalDevice()->getParameter("trigger_1f_pulse_width").getMax<int>(); break;
                            case 6: val = device->getPhysicalDevice()->getParameter("trigger_1g_pulse_width").getMax<int>(); break;
                            case 7: val = device->getPhysicalDevice()->getParameter("trigger_1h_pulse_width").getMax<int>(); break;
                        };
                    }
                    info.setUInt(val);
                }
            }
            break;
        case 0x55: // LineOffset (trigger offset in ms)
            if (triggerInputAvailable) {
                // Not SceneScan: LineSelector 0 is the only output pin, named trigger_1_...
                if (currentSelectorForLine == 0) {
                    int val = device->getPhysicalDevice()->getParameter("trigger_1_offset").getCurrent<int>();
                    info.setUInt(val);
                } else {
                    info.setUInt(0);
                }
            } else {
                // SceneScan: only the secondary output has a configurable offset parameter
                if (currentSelectorForLine == 0) {
                    info.setUInt(0);
                } else {
                    int val = device->getPhysicalDevice()->getParameter("trigger_1_offset").getCurrent<int>();
                    info.setUInt(val);
                }
            }
            break;
        case 0x56: // LineOffsetMin
            if (triggerInputAvailable) {
                // Not SceneScan: LineSelector 0 is the only output pin, named trigger_1_...
                if (currentSelectorForLine == 0) {
                    int val = device->getPhysicalDevice()->getParameter("trigger_1_offset").getMin<int>();
                    info.setUInt(val);
                } else {
                    info.setUInt(0);
                }
            } else {
                // SceneScan: only the secondary output has a configurable offset parameter
                if (currentSelectorForLine == 0) {
                    info.setUInt(0);
                } else {
                    int val = device->getPhysicalDevice()->getParameter("trigger_1_offset").getMin<int>();
                    info.setUInt(val);
                }
            }
            break;
        case 0x57: // LineOffsetMax
            if (triggerInputAvailable) {
                // Not SceneScan: LineSelector 0 is the only output pin, named trigger_1_...
                if (currentSelectorForLine == 0) {
                    int val = device->getPhysicalDevice()->getParameter("trigger_1_offset").getMax<int>();
                    info.setUInt(val);
                } else {
                    info.setUInt(0);
                }
            } else {
                // SceneScan: only the secondary output has a configurable offset parameter
                if (currentSelectorForLine == 0) {
                    info.setUInt(0);
                } else {
                    int val = device->getPhysicalDevice()->getParameter("trigger_1_offset").getMax<int>();
                    info.setUInt(val);
                }
            }
            break;
        case 0xff: // Nerian device feature map (used to mask the availability of other features via the XML) (DeviceFeatureReg)
            {
                auto dev = device->getPhysicalDevice();
                uint32_t featureMap = 0;
                // Bit 0: Availability of third camera
                int numCameras = dev->getParameter("calib_num_cameras").getCurrent<int>();
                featureMap |= (numCameras>2) ? 1 : 0;
                // Bit 1: Availability of Nerian software white balance
                featureMap |= dev->hasParameter("white_balance_mode") ? 2 : 0;
                // Bit 2: Availability of hardware trigger input
                featureMap |= triggerInputAvailable ? 4 : 0;
                // Bit 3: Availability of pattern projector
                featureMap |= dev->hasParameter("projector_brightness") ? 8 : 0;
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
    bool hasDisparityStream = metaData.hasImageType(ImageSet::IMAGE_DISPARITY);

    // Possible values of the component selector
    enum ComponentSelector {
        Intensity,
        Disparity,
        Range
    };

    switch(featureId) {
        // Component selector specific info
        case 4:  // ComponentEnable
            {
                //const unsigned int INTENSITY_BIT = 1;
                //const unsigned int DISPARITY_BIT = 2;
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
        case 0x1e: // TriggerSoftwareReg: force emission of software trigger
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                DEBUG_DEVPORT("=== Sending software trigger request ===");
                device->getPhysicalDevice()->sendSoftwareTriggerRequest(); // value is actually a dummy
                return GC_ERR_SUCCESS;
            }
            break;
        // (0x1c QMatrixData is read-only)
        case 0x1f: // Width
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                DEBUG_DEVPORT("=== Requesting new width " << newVal << " ===");
                device->getPhysicalDevice()->setParameter("capture_roi_width", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x22: // Height
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                DEBUG_DEVPORT("=== Requesting new height " << newVal << " ===");
                device->getPhysicalDevice()->setParameter("capture_roi_height", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x25: // OffsetX
            {
                // Device parameters use coordinates relative from center; translate
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                auto dev = device->getPhysicalDevice();
                int maxRel = dev->getParameter("capture_cam0_roi_offset_x").getMax<int>();
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0] - maxRel;
                DEBUG_DEVPORT("=== Requesting new ROI X offset " << newVal << " ===");
                // TODO GenTL: preserve any L-R offset?
                dev->setParameter("capture_cam0_roi_offset_x", newVal);
                dev->setParameter("capture_cam1_roi_offset_x", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x28: // OffsetY
            {
                // Device parameters use coordinates relative from center; translate
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                auto dev = device->getPhysicalDevice();
                int maxRel = dev->getParameter("capture_cam0_roi_offset_y").getMax<int>();
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0] - maxRel;
                DEBUG_DEVPORT("=== Requesting new ROI Y offset " << newVal << " ===");
                // TODO GenTL: preserve any L-R offset?
                dev->setParameter("capture_cam0_roi_offset_y", newVal);
                dev->setParameter("capture_cam1_roi_offset_y", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x2b: // AcquisitionFrameRate
            {
                if (*piSize != 8) throw std::runtime_error("Expected a new feature value of size 8");
                double newVal = (reinterpret_cast<const double*>(pBuffer))[0];
                auto dev = device->getPhysicalDevice();
                dev->setParameter("trigger_frequency", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x2d: // TriggerMode (NB: TriggerSource and TriggerMode overlap in Nerian parameter)
            {
                // DEPRECATED / unused register
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                auto dev = device->getPhysicalDevice();
                dev->setParameter("trigger_input", (newVal==0) ? 0 : preferredTriggerSource);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x2e: // TriggerSource (NB: TriggerSource and TriggerMode overlap in Nerian parameter)
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                preferredTriggerSource = newVal;
                auto dev = device->getPhysicalDevice();
                auto currentVal = dev->getParameter("trigger_input").getCurrent<int>();
                if (currentVal) {
                    // Toggle between SW and ext immediately
                    dev->setParameter("trigger_input", preferredTriggerSource);
                } else {
                    // TriggerMode was off - don't send anything until it is changed to on
                }
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x2f: // PatternProjectorBrightness
            {
                if (*piSize != 8) throw std::runtime_error("Expected a new feature value of size 8");
                double newVal = (reinterpret_cast<const double*>(pBuffer))[0];
                auto dev = device->getPhysicalDevice();
                if (!dev->hasParameter("projector_brightness")) return GC_ERR_NOT_AVAILABLE; // should not happen
                dev->setParameter("projector_brightness", newVal / 100.0);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x30: // NumberOfDisparities
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("number_of_disparities", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x34: // DisparityOffset
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("disparity_offset", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x35: // SgmP1NoEdge
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("sgm_p1_no_edge", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x36: // SgmP1Edge
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("sgm_p1_edge", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x37: // SgmP2NoEdge
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("sgm_p2_no_edge", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x38: // SgmP2Edge
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("sgm_p2_edge", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x39: // SgmEdgeSensitivity
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("sgm_edge_sensitivity", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x3a: // SubpixelOptimizationROIEnabled
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("subpixel_optimization_roi_enabled", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x3b: // SubpixelOptimizationROIWidth
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("subpixel_optimization_roi_width", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x3c: // SubpixelOptimizationROIHeight
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("subpixel_optimization_roi_height", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x3d: // SubpixelOptimizationROIOffsetX
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("subpixel_optimization_roi_x", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x3e: // SubpixelOptimizationROIOffsetY
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("subpixel_optimization_roi_y", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x3f: // MaskBorderPixelsEnabled
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("mask_border_pixels_enabled", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x40: // ConsistencyCheckEnabled
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("consistency_check_enabled", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x41: // ConsistencyCheckSensitivity
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("consistency_check_sensitivity", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x42: // UniquenessCheckEnabled 
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("uniqueness_check_enabled", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x43: // UniquenessCheckSensitivity
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("uniqueness_check_sensitivity", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x44: // TextureFilterEnabled 
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("texture_filter_enabled", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x45: // TextureFilterSensitivity
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("texture_filter_sensitivity", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x46: // GapInterpolationEnabled 
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("gap_interpolation_enabled", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x47: // NoiseReductionEnabled
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("noise_reduction_enabled", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x48: // SpeckleFilterIterations
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("speckle_filter_iterations", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x4B: // InputPixelFormat (active pixel format for L/R cam acquisition)
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                device->getPhysicalDevice()->setParameter("capture_pixel_format", newVal);
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x4D: // LineSelector
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                currentSelectorForLine = newVal;
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x4E: // LineMode (Input / Output: Line0 always Output, Line1 always Output on SceneScan, switchable on others)
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                // DEPRECATED - unused address - defined read-only via XML
                return GC_ERR_NOT_AVAILABLE;
            }
            break;
        case 0x4F: // LineInverter
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                if (triggerInputAvailable) {
                    // Not SceneScan: LineSelector 0 is the only output pin, named trigger_1_...
                    if (currentSelectorForLine == 0) {
                        device->getPhysicalDevice()->setParameter("trigger_1_polarity", newVal?1:0);
                    } else {
                        // LineSelector 1 is the input pin - we cannot change polarity
                        return GC_ERR_NOT_AVAILABLE;
                    }
                } else {
                    // SceneScan: LineSelector 0 and 1 are trigger_0_... and trigger_1_..., respectively
                    device->getPhysicalDevice()->setParameter((currentSelectorForLine==0)?"trigger_0_polarity":"trigger_1_polarity", newVal?1:0);
                }
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x50: // LineSource: signal source for output pins. FrameTrigger (if 'Enabled' in nvparam), Constant0/1 (if 'Disabled with constant' in nvparam)
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                if (triggerInputAvailable) {
                    // Not SceneScan: LineSelector 0 is the only output pin, named trigger_1_...
                    if (currentSelectorForLine == 0) {
                        switch (newVal) {
                            case 0:
                                device->getPhysicalDevice()->setParameter("trigger_1_constant", 0);
                                device->getPhysicalDevice()->setParameter("trigger_1_enabled", 0);
                                break;
                            case 1:
                                device->getPhysicalDevice()->setParameter("trigger_1_constant", 1);
                                device->getPhysicalDevice()->setParameter("trigger_1_enabled", 0);
                                break;
                            case 2:
                                device->getPhysicalDevice()->setParameter("trigger_1_enabled", 1);
                        }
                    } else {
                        // LineSelector 1 is the input pin - we cannot change LineSource
                        return GC_ERR_NOT_AVAILABLE;
                    }
                } else {
                    // SceneScan: LineSelector 0 and 1 are trigger_0_... and trigger_1_..., respectively
                    switch (newVal) {
                        case 0:
                            if (currentSelectorForLine == 0) {
                                device->getPhysicalDevice()->setParameter("trigger_0_constant", 0);
                                device->getPhysicalDevice()->setParameter("trigger_0_enabled", 0);
                            } else {
                                device->getPhysicalDevice()->setParameter("trigger_1_constant", 0);
                                device->getPhysicalDevice()->setParameter("trigger_1_enabled", 0);
                            }
                            break;
                        case 1:
                            if (currentSelectorForLine == 0) {
                                device->getPhysicalDevice()->setParameter("trigger_0_constant", 1);
                                device->getPhysicalDevice()->setParameter("trigger_0_enabled", 0);
                            } else {
                                device->getPhysicalDevice()->setParameter("trigger_1_constant", 1);
                                device->getPhysicalDevice()->setParameter("trigger_1_enabled", 0);
                            }
                            break;
                        case 2:
                        default:
                            if (currentSelectorForLine == 0) {
                                device->getPhysicalDevice()->setParameter("trigger_0_enabled", 1);
                            } else {
                                device->getPhysicalDevice()->setParameter("trigger_1_enabled", 1);
                            }
                    }
                }
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x51: // LinePulseWidthSelector (PulseWidth1..8)
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                currentSelectorForPulseWidth = newVal;
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x52: // LinePulseWidthAlternationMode (Constant / Cycle; Constant restricts LinePulseWidthSelector to 1)
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                if (triggerInputAvailable) {
                    // Not SceneScan: LineSelector 0 is the only output pin, named trigger_1_...
                    if (currentSelectorForLine == 0) {
                        device->getPhysicalDevice()->setParameter("trigger_1_pulse_width_mode", newVal ? 1 : 0);
                    } else {
                        // LineSelector 1 is the input pin - we cannot change pulse width settings
                        return GC_ERR_NOT_AVAILABLE;
                    }
                } else {
                    // SceneScan: LineSelector 0 and 1 are trigger_0_... and trigger_1_..., respectively
                    device->getPhysicalDevice()->setParameter((currentSelectorForLine==0)?"trigger_0_pulse_width_mode":"trigger_1_pulse_width_mode", newVal ? 1 : 0);
                }
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x53: // LinePulseWidth (trigger_XY_pulse_width, X LineSelector -> 0,1; Y LinePulseWidthSelector -> '','b'..'h')
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                int triggerIndex = -1;
                if (triggerInputAvailable) {
                    // Not SceneScan: LineSelector 0 is the only output pin, named trigger_1_...
                    if (currentSelectorForLine == 0) {
                        triggerIndex = 1;
                    } else {
                        // LineSelector 1 is the input pin - we cannot change pulse width settings
                        return GC_ERR_NOT_AVAILABLE;
                    }
                } else {
                    // SceneScan: LineSelector 0 and 1 are trigger_0_... and trigger_1_..., respectively
                    triggerIndex = currentSelectorForLine;
                }

                if (triggerIndex == 0) {
                    switch (currentSelectorForPulseWidth) {
                        case 0: device->getPhysicalDevice()->setParameter("trigger_0_pulse_width", newVal); break;
                        case 1: device->getPhysicalDevice()->setParameter("trigger_0b_pulse_width", newVal); break;
                        case 2: device->getPhysicalDevice()->setParameter("trigger_0c_pulse_width", newVal); break;
                        case 3: device->getPhysicalDevice()->setParameter("trigger_0d_pulse_width", newVal); break;
                        case 4: device->getPhysicalDevice()->setParameter("trigger_0e_pulse_width", newVal); break;
                        case 5: device->getPhysicalDevice()->setParameter("trigger_0f_pulse_width", newVal); break;
                        case 6: device->getPhysicalDevice()->setParameter("trigger_0g_pulse_width", newVal); break;
                        case 7: device->getPhysicalDevice()->setParameter("trigger_0h_pulse_width", newVal); break;
                    };
                } else {
                    switch (currentSelectorForPulseWidth) {
                        case 0: device->getPhysicalDevice()->setParameter("trigger_1_pulse_width", newVal); break;
                        case 1: device->getPhysicalDevice()->setParameter("trigger_1b_pulse_width", newVal); break;
                        case 2: device->getPhysicalDevice()->setParameter("trigger_1c_pulse_width", newVal); break;
                        case 3: device->getPhysicalDevice()->setParameter("trigger_1d_pulse_width", newVal); break;
                        case 4: device->getPhysicalDevice()->setParameter("trigger_1e_pulse_width", newVal); break;
                        case 5: device->getPhysicalDevice()->setParameter("trigger_1f_pulse_width", newVal); break;
                        case 6: device->getPhysicalDevice()->setParameter("trigger_1g_pulse_width", newVal); break;
                        case 7: device->getPhysicalDevice()->setParameter("trigger_1h_pulse_width", newVal); break;
                    };
                }
                return GC_ERR_SUCCESS;
            }
            break;
        case 0x55: // LineOffset (trigger offset in ms)
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                int32_t newVal = (reinterpret_cast<const int32_t*>(pBuffer))[0];
                if (triggerInputAvailable) {
                    // Not SceneScan: LineSelector 0 is the only output pin, named trigger_1_...
                    if (currentSelectorForLine == 0) {
                        device->getPhysicalDevice()->setParameter("trigger_1_offset", newVal);
                    } else {
                        // LineSelector 1 is the input pin
                        return GC_ERR_NOT_AVAILABLE;
                    }
                } else {
                    // SceneScan: only the secondary output has a configurable offset parameter
                    if (currentSelectorForLine == 0) {
                        if (newVal != 0) return GC_ERR_NOT_AVAILABLE;
                    } else {
                        device->getPhysicalDevice()->setParameter("trigger_1_offset", newVal);
                    }
                }
                return GC_ERR_SUCCESS;
            }
            break;
        case 0xF1: // FlushOutputQueue: force flushing of all DataStreams' output queues
            {
                if (*piSize != 4) throw std::runtime_error("Expected a new feature value of size 4");
                DEBUG_DEVPORT("=== FlushOutputQueue: discard queued output for all open DataStreams ===");
                auto phys = device->getPhysicalDevice();
                for(int i=0; i<phys->NUM_LOGICAL_DEVICES; i++) {
                    auto logi = phys->getLogicalDevice(i);
                    if(logi->isOpen()) {
                        DEBUG_DEVPORT("  Invoking flushQueue(ACQ_QUEUE_ALL_TO_INPUT) for: " << logi->getId());
                        auto ds = logi->getStream();
                        ds->flushQueue(ACQ_QUEUE_ALL_TO_INPUT);
                    }
                }
                return GC_ERR_SUCCESS;
            }
            break;
        default:
            DEBUG_DEVPORT("TODO - implement me (DevPortImpl::writeChildFeature)");
    }
    return GC_ERR_NOT_AVAILABLE;
}

}
