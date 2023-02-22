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

#ifndef VISIONTRANSFER_STANDARDRAMETERIDS_H
#define VISIONTRANSFER_STANDARDRAMETERIDS_H

#include <map>
#include <string>

#include <stdint.h>

#include <visiontransfer/parameterinfo.h>

namespace visiontransfer {
namespace internal {

/**
 * \brief A collection of numeric IDs for all supported parameters by
 * Nerian stereo devices.
 *
 * This class is only used internally. Users should use the class
 * \ref DeviceParameters instead.
 */

class StandardParameterIDs {
public:

    enum ParameterID {
        // Reserved
        UNDEFINED                           = 0x0000,

        // Processing settings
        OPERATION_MODE                      = 0x0100,
        NUMBER_OF_DISPARITIES               = 0x0101, // Not available yet
        DISPARITY_OFFSET                    = 0x0102,
        MAX_NUMBER_OF_IMAGES                = 0x0103,

        // Algorithmic settings
        SGM_P1_EDGE                         = 0x0200,
        SGM_P2_EDGE                         = 0x0201,
        MASK_BORDER_PIXELS_ENABLED          = 0x0202,
        CONSISTENCY_CHECK_ENABLED           = 0x0203,
        CONSISTENCY_CHECK_SENSITIVITY       = 0x0204,
        UNIQUENESS_CHECK_ENABLED            = 0x0205,
        UNIQUENESS_CHECK_SENSITIVITY        = 0x0206,
        TEXTURE_FILTER_ENABLED              = 0x0207,
        TEXTURE_FILTER_SENSITIVITY          = 0x0208,
        GAP_INTERPOLATION_ENABLED           = 0x0209,
        NOISE_REDUCTION_ENABLED             = 0x020a,
        SPECKLE_FILTER_ITERATIONS           = 0x020b,
        SGM_P1_NO_EDGE                      = 0x020c,
        SGM_P2_NO_EDGE                      = 0x020d,
        SGM_EDGE_SENSITIVITY                = 0x020e,
        SUBPIXEL_OPTIMIZATION_ROI_ENABLED   = 0x020f,
        SUBPIXEL_OPTIMIZATION_ROI_X         = 0x0210,
        SUBPIXEL_OPTIMIZATION_ROI_Y         = 0x0211,
        SUBPIXEL_OPTIMIZATION_ROI_WIDTH     = 0x0212,
        SUBPIXEL_OPTIMIZATION_ROI_HEIGHT    = 0x0213,

        // Exposure settings
        AUTO_EXPOSURE_MODE                  = 0x0300,
        AUTO_TARGET_INTENSITY               = 0x0301,
        AUTO_INTENSITY_DELTA                = 0x0302,
        AUTO_TARGET_FRAME                   = 0x0303,
        AUTO_SKIPPED_FRAMES                 = 0x0304,
        AUTO_MAXIMUM_EXPOSURE_TIME          = 0x0305,
        AUTO_MAXIMUM_GAIN                   = 0x0306,
        MANUAL_EXPOSURE_TIME                = 0x0307,
        MANUAL_GAIN                         = 0x0308,
        AUTO_EXPOSURE_ROI_ENABLED           = 0x0309,
        AUTO_EXPOSURE_ROI_X                 = 0x030a,
        AUTO_EXPOSURE_ROI_Y                 = 0x030b,
        AUTO_EXPOSURE_ROI_WIDTH             = 0x030c,
        AUTO_EXPOSURE_ROI_HEIGHT            = 0x030d,

        // Trigger / Pairing
        MAX_FRAME_TIME_DIFFERENCE_MS        = 0x0400,
        TRIGGER_FREQUENCY                   = 0x0401,
        TRIGGER_0_ENABLED                   = 0x0402,
        TRIGGER_0_PULSE_WIDTH               = 0x0403,
        TRIGGER_1_ENABLED                   = 0x0404,
        TRIGGER_1_PULSE_WIDTH               = 0x0405,
        TRIGGER_1_OFFSET                    = 0x0406,
        TRIGGER_0B_PULSE_WIDTH              = 0x0407,
        TRIGGER_0C_PULSE_WIDTH              = 0x0408,
        TRIGGER_0D_PULSE_WIDTH              = 0x0409,
        TRIGGER_1B_PULSE_WIDTH              = 0x040a,
        TRIGGER_1C_PULSE_WIDTH              = 0x040b,
        TRIGGER_1D_PULSE_WIDTH              = 0x040c,
        TRIGGER_0_POLARITY                  = 0x040d,
        TRIGGER_1_POLARITY                  = 0x040e,
        TRIGGER_0E_PULSE_WIDTH              = 0x040f,
        TRIGGER_0F_PULSE_WIDTH              = 0x0410,
        TRIGGER_0G_PULSE_WIDTH              = 0x0411,
        TRIGGER_0H_PULSE_WIDTH              = 0x0412,
        TRIGGER_1E_PULSE_WIDTH              = 0x0413,
        TRIGGER_1F_PULSE_WIDTH              = 0x0414,
        TRIGGER_1G_PULSE_WIDTH              = 0x0415,
        TRIGGER_1H_PULSE_WIDTH              = 0x0416,
        TRIGGER_0_CONSTANT                  = 0x0417,
        TRIGGER_1_CONSTANT                  = 0x0418,
        TRIGGER_INPUT                       = 0x0419,

        // Auto Re-calibration
        AUTO_RECALIBRATION_ENABLED          = 0x0500,
        AUTO_RECALIBRATION_PERMANENT        = 0x0501,

        // System settings
        REBOOT                              = 0x0600,
        PPS_SYNC                            = 0x0601
    };

    enum ParameterFlags {
        // bit flags
        PARAMETER_WRITEABLE                 = 0x0001,
    };

    // String representations for all ParameterIDs. They correspond
    // to a lowercase version, OPERATION_MODE <-> "operation_mode";
    // contents initialized C++11 style over in the source file
    static const std::map<ParameterID, std::string> parameterNameByID;

    // Obtain the ParameterID for a parameter name, or UNDEFINED if invalid
    static ParameterID getParameterIDForName(const std::string& name);

};

}} // namespace

#endif
