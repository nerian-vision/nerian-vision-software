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

#ifndef VISIONTRANSFER_DEVICEPARAMETERS_H
#define VISIONTRANSFER_DEVICEPARAMETERS_H

#include "visiontransfer/common.h"
#include "visiontransfer/deviceinfo.h"
#include "visiontransfer/standardparameterids.h"
#include "visiontransfer/parameterinfo.h"
#if __cplusplus >= 201103L
#include "visiontransfer/parameter.h"
#include "visiontransfer/parameterset.h"
#include <functional>
#endif

#include <map>

namespace visiontransfer {

/**
 * \brief Allows for configuration of the parameters of a Nerian stereo device
 * through a network connection.
 *
 * Parameters are read and written through a TCP connection. Not all
 * parameters that are available in the web interface can be configured
 * through this class.
 *
 * If parameters are changed, they are only valid until the device is
 * rebooted or until a parameter change is performed through the web
 * interface.
 *
 * Since device parameters should be predictable at all times,
 * the functions from this class will internally throw a
 * visiontransfer::TransferException in case of network failure
 * or device reinitialization during parameter access. Please catch
 * this exception if you wish to handle such cases.
 */

class VT_EXPORT DeviceParameters {

private:
    // We (mostly) follow the pimpl idiom here
    class Pimpl;
    Pimpl* pimpl;

public:
    /**
     * \brief Connects to parameter server of a Nerian stereo device by using the
     * device information from device enumeration.
     *
     * \param device Information on the device to which a connection should
     *               be established.
     */
    DeviceParameters(const DeviceInfo& device);

    /**
     * \brief Connects to parameter server of a Nerian stereo device by using a network
     * address.
     *
     * \param address   IP address or host name of the device to which a connection should
     *                  be established.
     * \param service   The port number that should be used as string or
     *                  as textual service name.
     */
    DeviceParameters(const char* address, const char* service = "7683");

    ~DeviceParameters();

    // Processing settings

    /**
     * \brief Operation modes supported by Nerian stereo devices.
     */
    enum OperationMode {
        /// The device passes through the input images without modification.
        PASS_THROUGH = 0,

        /// The devices outputs the rectified input images.
        RECTIFY = 1,

        /// The devices performs stereo matching.
        STEREO_MATCHING = 2
    };

    /**
     * \brief Gets the current operation mode.
     * \return  The current operation mode, which can be PASS_THROUGH,
     *          RECTIFY or STEREO_MATCHING.
     * \see     OperationMode
     */
    OperationMode getOperationMode() {
        return static_cast<OperationMode>(readIntParameter("operation_mode"));
    }

    /**
     * \brief Configures the device to a new operation mode.
     * \param mode  The new operation mode, which can be PASS_THROUGH,
     *              RECTIFY or STEREO_MATCHING.
     * \see         OperationMode
     */
    void setOperationMode(OperationMode mode) {
        writeIntParameter("operation_mode", static_cast<int>(mode));
    }

    /**
     * \brief Gets the current offset of the evaluated disparity range.
     */
    int getDisparityOffset() {
        return readIntParameter("disparity_offset");
    }

    /**
     * \brief Sets the offset of the evaluated disparity range.
     *
     * The offset plus the number of disparities must be smaller or equal to 256.
     */
    void setDisparityOffset(int offset) {
        writeIntParameter("disparity_offset", offset);
    }

    // Algorithmic settings

    /**
     * \brief Gets the SGM penalty P1 for small disparity changes at image edges.
     */
    int getStereoMatchingP1Edge() {
        return readIntParameter("sgm_p1_edge");
    }

    /**
     * \brief Sets the SGM penalty P1 for small disparity changes at image edges.
     *
     * This parameter must be in the range of 0 to 255.
     */
    void setStereoMatchingP1Edge(int p1) {
        writeIntParameter("sgm_p1_edge", p1);
    }

    /**
     * \brief Gets the SGM penalty P1 for small disparity changes outside image edges.
     */
    int getStereoMatchingP1NoEdge() {
        return readIntParameter("sgm_p1_no_edge");
    }

    /**
     * \brief Sets the SGM penalty P1 for small disparity changes outside image edges.
     *
     * This parameter must be in the range of 0 to 255.
     */
    void setStereoMatchingP1NoEdge(int p1) {
        writeIntParameter("sgm_p1_no_edge", p1);
    }

    /**
     * \brief Gets the SGM penalty P2 for large disparity changes at image edges.
     */
    int getStereoMatchingP2Edge() {
        return readIntParameter("sgm_p2_edge");
    }

    /**
     * \brief Sets the SGM penalty P2 for large disparity changes at image edges.
     *
     * This parameter must be in the range of 0 to 255.
     */
    void setStereoMatchingP2Edge(int p2) {
        writeIntParameter("sgm_p2_edge", p2);
    }

    /**
     * \brief Gets the SGM penalty P2 for large disparity changes at image edges.
     */
    int getStereoMatchingP2NoEdge() {
        return readIntParameter("sgm_p2_no_edge");
    }

    /**
     * \brief Sets the SGM penalty P2 for large disparity changes at image edges.
     *
     * This parameter must be in the range of 0 to 255.
     */
    void setStereoMatchingP2NoEdge(int p2) {
        writeIntParameter("sgm_p2_no_edge", p2);
    }

    /**
     * \brief Gets the edge sensitivity of the SGM algorithm
     */
    int getStereoMatchingEdgeSensitivity() {
        return readIntParameter("sgm_edge_sensitivity");
    }

    /**
     * \brief Sets the edge sensitivity of the SGM algorithm
     *
     * This parameter must be in the range of 0 to 255.
     */
    void setStereoMatchingEdgeSensitivity(int sensitivity) {
        writeIntParameter("sgm_edge_sensitivity", sensitivity);
    }

    /**
     * \brief Returns true if border pixels are removed from the computed
     * disparity map.
     */
    bool getMaskBorderPixelsEnabled() {
        return readBoolParameter("mask_border_pixels_enabled");
    }

    /**
     * \brief Enables or disables the removal of border pixels from the computed
     * disparity map.
     */
    void setMaskBorderPixelsEnabled(bool enabled) {
        writeBoolParameter("mask_border_pixels_enabled", enabled);
    }

    /**
     * \brief Returns true if the consistency check is enabled.
     */
    bool getConsistencyCheckEnabled() {
        return readBoolParameter("consistency_check_enabled");
    }

    /**
     * \brief Enables or disables the consistency check.
     */
    void setConsistencyCheckEnabled(bool enabled) {
        writeBoolParameter("consistency_check_enabled", enabled);
    }

    /**
     * \brief Gets the current sensitivity value for the consistency check.
     */
    int getConsistencyCheckSensitivity() {
        return readIntParameter("consistency_check_sensitivity");
    }

    /**
     * \brief Sets a new sensitivity value for the consistency check.
     *
     * This parameter must be in the range of 0 to 15.
     */
    void setConsistencyCheckSensitivity(int sensitivity) {
        writeIntParameter("consistency_check_sensitivity", sensitivity);
    }

    /**
     * \brief Returns true if the consistency check is enabled.
     */
    bool getUniquenessCheckEnabled() {
        return readBoolParameter("uniqueness_check_enabled");
    }

    /**
     * \brief Enables or disables the uniqueness check.
     */
    void setUniquenessCheckEnabled(bool enabled) {
        writeBoolParameter("uniqueness_check_enabled", enabled);
    }

    /**
     * \brief Gets the current sensitivity value for the uniqueness check.
     */
    int getUniquenessCheckSensitivity() {
        return readIntParameter("uniqueness_check_sensitivity");
    }

    /**
     * \brief Sets a new sensitivity value for the uniqueness check.
     *
     * This parameter must be in the range of 0 to 256.
     */
    void setUniquenessCheckSensitivity(int sensitivity) {
        writeIntParameter("uniqueness_check_sensitivity", sensitivity);
    }

    /**
     * \brief Returns true if the texture filter is enabled.
     */
    bool getTextureFilterEnabled() {
        return readBoolParameter("texture_filter_enabled");
    }

    /**
     * \brief Enables or disables the texture filter.
     */
    void setTextureFilterEnabled(bool enabled) {
        writeBoolParameter("texture_filter_enabled", enabled);
    }

    /**
     * \brief Gets the current sensitivity value for the texture filter.
     */
    int getTextureFilterSensitivity() {
        return readIntParameter("texture_filter_sensitivity");
    }

    /**
     * \brief Sets a new sensitivity value for the texture filter.
     *
     * This parameter must be in the range of 0 to 63.
     */
    void setTextureFilterSensitivity(int sensitivity) {
        writeIntParameter("texture_filter_sensitivity", sensitivity);
    }

    /**
     * \brief Returns true if the texture gap interpolation is enabled.
     */
    bool getGapInterpolationEnabled() {
        return readBoolParameter("gap_interpolation_enabled");
    }

    /**
     * \brief Enables or disables the gap interpolation.
     */
    void setGapInterpolationEnabled(bool enabled) {
        writeBoolParameter("gap_interpolation_enabled", enabled);
    }

    /**
     * \brief Returns true if the noise reduction filter is enabled.
     */
    bool getNoiseReductionEnabled() {
        return readBoolParameter("noise_reduction_enabled");
    }

    /**
     * \brief Enables or disables the noise reduction filter.
     */
    void setNoiseReductionEnabled(bool enabled) {
        writeBoolParameter("noise_reduction_enabled", enabled);
    }

    /**
     * \brief Returns true if the speckle filter is enabled.
     */
    int getSpeckleFilterIterations() {
        return readIntParameter("speckle_filter_iterations");
    }

    /**
     * \brief Enables or disables the speckle filter.
     */
    void setSpeckleFilterIterations(int iter) {
        writeIntParameter("speckle_filter_iterations", iter);
    }

    // Exposure and gain settings

    /**
     * \brief Possible modes of the automatic exposure and gain control.
     */
    enum AutoMode {
        /// Both, exposure and gain are automatically adjusted
        AUTO_EXPOSURE_AND_GAIN = 0,

        /// Only exposure is automatically adjusted, gain is set manually
        AUTO_EXPOSURE_MANUAL_GAIN = 1,

        /// Only gain is automatically adjusted, exposure is set manually
        MANUAL_EXPOSORE_AUTO_GAIN = 2,

        /// Both, exposure and gain are set manually
        MANUAL_EXPOSURE_MANUAL_GAIN = 3
    };

    /**
     * \brief Gets the current mode of the automatic exposure and gain control.
     * \see AutoMode
     */
    AutoMode getAutoMode() {
        return static_cast<AutoMode>(readIntParameter("auto_exposure_mode"));
    }

    /**
     * \brief Sets the current mode of the automatic exposure and gain control.
     * \see AutoMode
     */
    void setAutoMode(AutoMode mode) {
        writeIntParameter("auto_exposure_mode", static_cast<int>(mode));
    }

    /**
     * \brief Gets the target image intensity of the automatic exposure and gain control
     * \return The target intensity.
     *
     * Intensities are measured from 0.0 to 1.0, with 0.0 being the darkest,
     * and 1.0 the brightest possible pixel intensity.
     */
    double getAutoTargetIntensity() {
        return readDoubleParameter("auto_target_intensity");
    }

    /**
     * \brief Sets the target image intensity of the automatic exposure and gain control
     * \param intensity The new target intensity.
     *
     * Intensities are measured from 0.0 to 1.0, with 0.0 being the darkest,
     * and 1.0 the brightest possible pixel intensity.
     */
    void setAutoTargetIntensity(double intensity) {
        writeDoubleParameter("auto_target_intensity", intensity);
    }

    /**
     * \brief Gets the minimum intensity change that is required for adjusting
     * the camera settings.
     *
     * Intensity values are relatively to the target intensity. A value of
     * 0.01 represents a change of 1%.
     */
    double getAutoIntensityDelta() {
        return readDoubleParameter("auto_intensity_delta");
    }

    /**
     * \brief Sets the minimum intensity change that is required for adjusting
     * the camera settings.
     *
     * Intensity values are relatively to the target intensity. A value of
     * 0.01 represents a change of 1%.
     */
    void setAutoIntensityDelta(double delta) {
        writeDoubleParameter("auto_intensity_delta", delta);
    }

    /**
     * \brief Possible options for the target frame selection of the
     * automatic exposure and gain control.
     */
    enum TargetFrame {
        /// Control using only the left frame
        LEFT_FRAME = 0,

        /// Control using only the right frame
        RIGHT_FRAME = 1,

        /// Control using both frames
        BOTH_FRAMES = 2,
    };

    /**
     * \brief Gets the selected target frame for automatic exposure and gain control.
     * \see TargetFrame
     */
    TargetFrame getAutoTargetFrame() {
        return static_cast<TargetFrame>(readIntParameter("auto_target_frame"));
    }

    /**
     * \brief Selects the target frame for automatic exposure and gain control.
     * \see TargetFrame
     */
    void setAutoTargetFrame(TargetFrame target) {
        writeIntParameter("auto_target_frame", static_cast<int>(target));
    }

    /**
     * \brief Gets the current interval at which the automatic exposure and gain control is run.
     *
     * The return value indicates the number of skipped frames between each
     * adjustment. Typically a value > 0 is desired to give the cameras enough
     * time to react to the new setting.
     */
    int getAutoSkippedFrames() {
        return readIntParameter("auto_skipped_frames");
    }

    /**
     * \brief Sets the current interval at which the automatic exposure and gain control is run.
     *
     * The return value indicates the number of skipped frames between each
     * adjustment. Typically a value > 0 is desired to give the cameras enough
     * time to react to the new setting.
     */
    void setAutoSkippedFrames(int skipped) {
        writeIntParameter("auto_skipped_frames", skipped);
    }

    /**
     * \brief Gets the maximum exposure time that can be selected automatically.
     * \return Maximum exposure time in microseconds.
     */
    double getAutoMaxExposureTime() {
        return readDoubleParameter("auto_maximum_exposure_time");
    }

    /**
     * \brief Sets the maximum exposure time that can be selected automatically.
     * \param time  Maximum exposure time in microseconds.
     */
    void setAutoMaxExposureTime(double time) {
        writeDoubleParameter("auto_maximum_exposure_time", time);
    }

    /**
     * \brief Gets the maximum gain that can be selected automatically.
     * \return Maximum gain in dB.
     */
    double getAutoMaxGain() {
        return readDoubleParameter("auto_maximum_gain");
    }

    /**
     * \brief Gets the maximum gain that can be selected automatically.
     * \param gain  Maximum gain in dB.
     */
    void setAutoMaxGain(double gain) {
        writeDoubleParameter("auto_maximum_gain", gain);
    }

    /**
     * \brief Gets the manually selected exposure time.
     * \return Exposure time in microseconds.
     *
     * This parameter is only relevant if the auto mode is set to
     * MANUAL_EXPOSORE_AUTO_GAIN or MANUAL_EXPOSURE_MANUAL_GAIN.
     *
     * \see setAutoMode
     */
    double getManualExposureTime() {
        return readDoubleParameter("manual_exposure_time");
    }

    /**
     * \brief Sets the manually selected exposure time.
     * \param time  Exposure time in microseconds.
     *
     * This parameter is only relevant if the auto mode is set to
     * MANUAL_EXPOSORE_AUTO_GAIN or MANUAL_EXPOSURE_MANUAL_GAIN.
     *
     * \see setAutoMode
     */
    void setManualExposureTime(double time) {
        writeDoubleParameter("manual_exposure_time", time);
    }

    /**
     * \brief Gets the manually selected gain.
     * \return Gain in dB.
     *
     * This parameter is only relevant if the auto mode is set to
     * AUTO_EXPOSORE_MANUAL_GAIN or MANUAL_EXPOSURE_MANUAL_GAIN.
     *
     * \see setAutoMode
     */
    double getManualGain() {
        return readDoubleParameter("manual_gain");
    }

    /**
     * \brief Sets the manually selected gain.
     * \param gain Gain in dB.
     *
     * This parameter is only relevant if the auto mode is set to
     * AUTO_EXPOSORE_MANUAL_GAIN or MANUAL_EXPOSURE_MANUAL_GAIN.
     *
     * \see setAutoMode
     */
    void setManualGain(double gain) {
        writeDoubleParameter("manual_gain", gain);
    }

    /**
     * \brief Returns true if an ROI for automatic exposure and gain control is enabled.
     */
    bool getAutoROIEnabled() {
        return readBoolParameter("auto_exposure_roi_enabled");
    }

    /**
     * \brief Enables or disables an ROI for automatic exposure and gain control.
     */
    void setAutoROIEnabled(bool enabled) {
        writeBoolParameter("auto_exposure_roi_enabled", enabled);
    }

    /**
     * \brief Gets the configured ROI for automatic exposure and gain control.
     *
     * \param x         Horizontal offset of the ROI from the image center. A value
     *                  of 0 means the ROI is horizontally centered.
     * \param y         Vertical offset of the ROI from the image center. A value
     *                  of 0 means the ROI is vertically centered.
     * \param width     Width of the ROI.
     * \param height    Height of the ROI.
     *
     * The ROI must be enabled with setAutoROIEnabled() before it is considered
     * for exposure or gain control.
     */
    void getAutoROI(int& x, int& y, int& width, int& height) {
        x = readIntParameter("auto_exposure_roi_x");
        y = readIntParameter("auto_exposure_roi_y");
        width = readIntParameter("auto_exposure_roi_width");
        height = readIntParameter("auto_exposure_roi_height");
    }

    /**
     * \brief Sets the configured ROI for automatic exposure and gain control.
     *
     * \param x         Horizontal offset of the ROI from the image center. A value
     *                  of 0 means the ROI is horizontally centered.
     * \param y         Vertical offset of the ROI from the image center. A value
     *                  of 0 means the ROI is vertically centered.
     * \param width     Width of the ROI.
     * \param height    Height of the ROI.
     *
     * The ROI must be enabled with setAutoROIEnabled() before it is considered
     * for exposure or gain control.
     */
    void setAutoROI(int x, int y, int width, int height) {
        writeIntParameter("auto_exposure_roi_x", x);
        writeIntParameter("auto_exposure_roi_y", y);
        writeIntParameter("auto_exposure_roi_width", width);
        writeIntParameter("auto_exposure_roi_height", height);
    }

    // Trigger and pairing settings

    /**
     * \brief Gets the maximum allowed time difference between two corresponding
     * frames.
     * \return Time difference in milliseconds. A value of -1 corresponds to automatic
     * pairing.
     */
    int getMaxFrameTimeDifference() {
        return readIntParameter("max_frame_time_difference_ms");
    }

    /**
     * \brief Sets the maximum allowed time difference between two corresponding
     * frames.
     * \param diffMs    Time difference in milliseconds. If automatic pairing is desired,
     *      a value of -1 should be set.
     */
    void setMaxFrameTimeDifference(int diffMs) {
        writeIntParameter("max_frame_time_difference_ms", diffMs);
    }

    /**
     * \brief Gets the frequency of the trigger signal.
     * \return Frequency in Hz.
     */
    double getTriggerFrequency() {
        return readDoubleParameter("trigger_frequency");
    }

    /**
     * \brief Sets the frequency of the trigger signal.
     * \param freq Frequency in Hz.
     */
    void setTriggerFrequency(double freq) {
        writeDoubleParameter("trigger_frequency", freq);
    }

    /**
     * \brief Returns true if trigger signal 0 is enabled.
     */
    bool getTrigger0Enabled() {
        return readBoolParameter("trigger_0_enabled");
    }

    /**
     * \brief Enables or disables trigger signal 0.
     */
    void setTrigger0Enabled(bool enabled) {
        writeBoolParameter("trigger_0_enabled", enabled);
    }

    /**
     * \brief Returns the constant value that is output when trigger 0 is disabled
     */
    bool getTrigger0Constant() {
        return readBoolParameter("trigger_0_constant");
    }

    /**
     * \brief Sets the constant value that is output when trigger 0 is disabled
     */
    void setTrigger0Constant(bool on) {
        writeBoolParameter("trigger_0_constant", on);
    }

    /**
     * \brief Returns false if trigger0 polarity is active-high (non-inverted) and
     * false if polarity is active-low (inverted)
     */
    bool getTrigger0Polarity() {
        return readBoolParameter("trigger_0_polarity");
    }

    /**
     * \brief Sets the polarity for trigger0. If invert is false, the polarity
     * is active-high (non-inverted). Otherwise the polarity is active-low (inverted).
     */
    void setTrigger0Polarity(bool invert) {
        writeBoolParameter("trigger_0_polarity", invert);
    }

    /**
     * \brief Returns true if trigger signal 1 is enabled.
     */
    bool getTrigger1Enabled() {
        return readBoolParameter("trigger_1_enabled");
    }

    /**
     * \brief Enables or disables trigger signal 1.
     */
    void setTrigger1Enabled(bool enabled) {
        writeBoolParameter("trigger_1_enabled", enabled);
    }

    /**
     * \brief Returns the constant value that is output when trigger 1 is disabled
     */
    bool getTrigger1Constant() {
        return readBoolParameter("trigger_1_constant");
    }

    /**
     * \brief Sets the constant value that is output when trigger 1 is disabled
     */
    void setTrigger1Constant(bool on) {
        writeBoolParameter("trigger_1_constant", on);
    }

    /**
     * \brief Returns false if trigger1 polarity is active-high (non-inverted) and
     * false if polarity is active-low (inverted)
     */
    bool getTrigger1Polarity() {
        return readBoolParameter("trigger_1_polarity");
    }

    /**
     * \brief Sets the polarity for trigger1. If invert is false, the polarity
     * is active-high (non-inverted). Otherwise the polarity is active-low (inverted).
     */
    void setTrigger1Polarity(bool invert) {
        writeBoolParameter("trigger_1_polarity", invert);
    }

    /**
     * \brief Gets the pulse width of trigger signal 0.
     * \param pulse     For a cyclic pulse width configuration, this is the index
     *                  of the pulse for which to return the width. Valid values
     *                  are 0 to 7.
     * \return Pulse width in milliseconds.
     */
    double getTrigger0PulseWidth(int pulse=0) {
        switch(pulse) {
            case 0: return readDoubleParameter("trigger_0_pulse_width");
            case 1: return readDoubleParameter("trigger_0b_pulse_width");
            case 2: return readDoubleParameter("trigger_0c_pulse_width");
            case 3: return readDoubleParameter("trigger_0d_pulse_width");
            case 4: return readDoubleParameter("trigger_0e_pulse_width");
            case 5: return readDoubleParameter("trigger_0f_pulse_width");
            case 6: return readDoubleParameter("trigger_0g_pulse_width");
            case 7: return readDoubleParameter("trigger_0h_pulse_width");
            default: return -1;
        }
    }

    /**
     * \brief Sets the pulse width of trigger signal 0.
     * \param width     Pulse width in milliseconds.
     * \param pulse     For a cyclic pulse width configuration, this is the index
     *                  of the pulse for which to set the width. Valid values
     *                  are 0 to 7.
     */
    void setTrigger0PulseWidth(double width, int pulse=0) {
        switch(pulse) {
            case 0: writeDoubleParameter("trigger_0_pulse_width", width);break;
            case 1: writeDoubleParameter("trigger_0b_pulse_width", width);break;
            case 2: writeDoubleParameter("trigger_0c_pulse_width", width);break;
            case 3: writeDoubleParameter("trigger_0d_pulse_width", width);break;
            case 4: writeDoubleParameter("trigger_0e_pulse_width", width);break;
            case 5: writeDoubleParameter("trigger_0f_pulse_width", width);break;
            case 6: writeDoubleParameter("trigger_0g_pulse_width", width);break;
            case 7: writeDoubleParameter("trigger_0h_pulse_width", width);break;
            default: return;
        }
    }

    /**
     * \brief Gets the pulse width of trigger signal 1.
     * \param pulse     For a cyclic pulse width configuration, this is the index
     *                  of the pulse for which to return the width. Valid values
     *                  are 0 to 7.
     * \return Pulse width in milliseconds.
     */
    double getTrigger1PulseWidth(int pulse=0) {
        switch(pulse) {
            case 0: return readDoubleParameter("trigger_1_pulse_width");
            case 1: return readDoubleParameter("trigger_1b_pulse_width");
            case 2: return readDoubleParameter("trigger_1c_pulse_width");
            case 3: return readDoubleParameter("trigger_1d_pulse_width");
            case 4: return readDoubleParameter("trigger_1e_pulse_width");
            case 5: return readDoubleParameter("trigger_1f_pulse_width");
            case 6: return readDoubleParameter("trigger_1g_pulse_width");
            case 7: return readDoubleParameter("trigger_1h_pulse_width");
            default: return -1;
        }
    }

    /**
     * \brief Sets the pulse width of trigger signal 1.
     * \param width     Pulse width in milliseconds.
     * \param pulse     For a cyclic pulse width configuration, this is the index
     *                  of the pulse for which to set the width. Valid values
     *                  are 0 to 7.
     */
    void setTrigger1PulseWidth(double width, int pulse=0) {
        switch(pulse) {
            case 0: writeDoubleParameter("trigger_1_pulse_width", width);break;
            case 1: writeDoubleParameter("trigger_1b_pulse_width", width);break;
            case 2: writeDoubleParameter("trigger_1c_pulse_width", width);break;
            case 3: writeDoubleParameter("trigger_1d_pulse_width", width);break;
            case 4: writeDoubleParameter("trigger_1e_pulse_width", width);break;
            case 5: writeDoubleParameter("trigger_1f_pulse_width", width);break;
            case 6: writeDoubleParameter("trigger_1g_pulse_width", width);break;
            case 7: writeDoubleParameter("trigger_1h_pulse_width", width);break;
            default: return;
        }
    }

    /**
     * \brief Gets the time offset between trigger signal 1 and signal 0.
     * \return Offset in milliseconds.
     */
    double getTrigger1Offset() {
        return readDoubleParameter("trigger_1_offset");
    }

    /**
     * \brief Sets the time offset between trigger signal 1 and signal 0.
     * \param offset    Offset in milliseconds.
     */
    void setTrigger1Offset(double offset) {
        writeDoubleParameter("trigger_1_offset", offset);
    }

    /**
     * \brief Returns true if the extgernal trigger input is enabled.
     */
    bool getInput() {
        return readBoolParameter("trigger_input");
    }

    /**
     * \brief Enables or disables the external trigger input
     */
    void setTrigger1Offset(bool enabled) {
        writeBoolParameter("trigger_input", enabled);
    }


    // Auto calibration parameters

    /**
     * \brief Returns true if auto re-calibration is enabled.
     */
    bool getAutoRecalibrationEnabled() {
        return readBoolParameter("auto_recalibration_enabled");
    }

    /**
     * \brief Enables or disables auto-recalibration.
     */
    void setAutoRecalibrationEnabled(bool enabled) {
        writeBoolParameter("auto_recalibration_enabled", enabled);
    }

    /**
     * \brief Returns true if persistent storage of auto re-calibration results is enabled.
     */
    bool getSaveAutoRecalibration() {
        return readBoolParameter("auto_recalibration_permanent");
    }

    /**
     * \brief Enables or disables persistent storage of auto re-calibration results.
     */
    void setSaveAutoRecalibration(bool save) {
        writeBoolParameter("auto_recalibration_permanent", save);
    }

    /**
     * \brief Returns true if an ROI for the subpixel optimization algorithm is enabled
     * (otherwise complete frames are used for optimization).
     */
    bool getSubpixelOptimizationROIEnabled() {
        return readBoolParameter("subpixel_optimization_roi_enabled");
    }

    /**
     * \brief Enables or disables an ROI for the subpixel optimization algorithm.
     * (if disabled, complete frames are used for optimization).
     */
    void setSubpixelOptimizationROIEnabled(bool enabled) {
        writeBoolParameter("subpixel_optimization_roi_enabled", enabled);
    }

    /**
     * \brief Gets the configured ROI for the subpixel optimization algorithm.
     *
     * \param x         Horizontal offset of the ROI from the image center. A value
     *                  of 0 means the ROI is horizontally centered.
     * \param y         Vertical offset of the ROI from the image center. A value
     *                  of 0 means the ROI is vertically centered.
     * \param width     Width of the ROI.
     * \param height    Height of the ROI.
     *
     * The ROI must be enabled with setSubpixelOptimizationROIEnabled(), otherwise the
     * optimization algorithm will consider the full images.
     */
    void getSubpixelOptimizationROI(int& x, int& y, int& width, int& height) {
        x = readIntParameter("subpixel_optimization_roi_x");
        y = readIntParameter("subpixel_optimization_roi_y");
        width = readIntParameter("subpixel_optimization_roi_width");
        height = readIntParameter("subpixel_optimization_roi_height");
    }

    /**
     * \brief Sets the configured ROI for the subpixel optimization algorithm.
     *
     * \param x         Horizontal offset of the ROI from the image center. A value
     *                  of 0 means the ROI is horizontally centered.
     * \param y         Vertical offset of the ROI from the image center. A value
     *                  of 0 means the ROI is vertically centered.
     * \param width     Width of the ROI.
     * \param height    Height of the ROI.
     *
     * The ROI must be enabled with setSubpixelOptimizationROIEnabled(), otherwise the
     * optimization algorithm will consider the full images.
     */
    void setSubpixelOptimizationROI(int x, int y, int width, int height) {
        writeIntParameter("subpixel_optimization_roi_x", x);
        writeIntParameter("subpixel_optimization_roi_y", y);
        writeIntParameter("subpixel_optimization_roi_width", width);
        writeIntParameter("subpixel_optimization_roi_height", height);
    }

    /**
     * \brief Remotely triggers a reboot of the device
     */
    void reboot() {
        writeBoolParameter("reboot", true);
    }

    /**
     * \brief Emit a software trigger event to perform a single acquisition. This only has effect when the External Trigger mode is set to Software.
     */
    void triggerNow() {
        writeBoolParameter("trigger_now", true);
    }

    /**
     * \brief Enumerates all simple parameters as reported by the device [DEPRECATED]
     * @deprecated since 10.0
     *
     * \return A map associating available parameter names with visiontransfer::ParameterInfo entries
     *
     * \note This function, as well as ParameterInfo, are deprecated and slated to be removed; please use getParameterSet() instead. This function omits all parameters that are not scalar numbers.
     */
    DEPRECATED("Use getParameterSet() instead")
    std::map<std::string, ParameterInfo> getAllParameters();

    /**
     * \brief Set a parameter by name. ParameterException for invalid names.
     * @deprecated since 10.0
     *
     * \note This function is deprecated and slated to be removed; please use setParameter() instead. This function only supports parameters that are scalar numbers.
     */
    template<typename T>
    DEPRECATED("Use setParameter() instead")
    void setNamedParameter(const std::string& name, T value);

    /**
     * \brief Set a parameter by name. ParameterException for invalid names or values.
     */
    template<typename T>
    void setParameter(const std::string& name, T value);

    /**
     * \brief Get a parameter by name, specifying the return type (int, double or bool). ParameterException for invalid names. [DEPRECATED]
     * @deprecated since 10.0
     *
     * \note This function is deprecated and slated to be removed; please use getParameter(name).getCurrent<T>() instead. This function only supports parameters that are scalar numbers.
     */
    template<typename T>
    DEPRECATED("Use getParameter() instead")
    T getNamedParameter(const std::string& name);

#if __cplusplus >= 201103L

    /**
     * \brief Tests whether a specific named parameter is available for this device.
     */
    bool hasParameter(const std::string& name);

    /**
     *  \brief Returns a Parameter object for the named device parameter. ParameterException for invalid or inaccessible parameter names.
     *
     * The returned object is a detached copy of the internal parameter at invocation time; it is not updated when the device sends a new value.
     * Likewise, any modifications must be requested using setParameter or the various parameter-specific setters.
     *
     * \note This function is available for C++11 and newer.
     */
    visiontransfer::param::Parameter getParameter(const std::string& name);

    /**
     * \brief Returns all API-accessible parameters as reported by the device.
     *
     * \return ParameterSet, which extends a std::map<std::string, visiontransfer::param::Parameter>
     *
     * Returned map entries are detached copies of the internal parameters at invocation time; they are not updated when the device sends new values.
     * Likewise, any modifications must be requested using setParameter or the various parameter-specific setters.
     *
     * \note This function is available for C++11 and newer.
     */
    visiontransfer::param::ParameterSet getParameterSet();

    /**
     * \brief Sets the optional user parameter update callback. This is then called for all asynchronous value or metadata changes that the server sends.
     *
     * The callback is called with the parameter UID, which can be used with getParameter(uid) to obtain the data.
     *
     * Caution:
     * The callback is called by the background receiver thread. Please queue the data suitably for consumption and perform costly operations outside this thread!
     * You must not perform parameter write operations directly inside the callback (or the receiver thread)!
     *
     * Note:
     * In the event that the parameter server connection was lost and is then re-established by the background thread, a callback will be invoked for all existing parameters.
     */
    void setParameterUpdateCallback(std::function<void(const std::string& uid)> callback);

    /**
     * \brief A (thread-local) parameter transaction lock for queued writes.
     *
     * Obtain a lock using transactionLock() if you want to set several, possibly dependent,
     * parameters in one go. You can't go wrong by using this for any parameter setting operation.
     * This ensures coordinated setting and uniform calculation of dependent parameters.
     * All parameter write operations (setParameter etc.) are transparently queued until the
     * TransactionLock object leaves scope, and then written as a batch in its destructor.
     */
    class VT_EXPORT TransactionLock {
        private:
            Pimpl* pimpl;
        public:
            TransactionLock(Pimpl* pimpl);
            ~TransactionLock();
    };
    friend class TransactionLock;

    /// Obtain a scoped TransactionLock for the current thread
    std::unique_ptr<TransactionLock> transactionLock();

#endif

private:
    // This class cannot be copied
    DeviceParameters(const DeviceParameters& other);
    DeviceParameters& operator=(const DeviceParameters& other);

    // Generic functions for reading parameters
    int readIntParameter(const char* id);
    double readDoubleParameter(const char* id);
    bool readBoolParameter(const char* id);

    // Generic functions for writing parameters
    void writeIntParameter(const char* id, int value);
    void writeDoubleParameter(const char* id, double value);
    void writeBoolParameter(const char* id, bool value);

};

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// For source compatibility
DEPRECATED("Use DeviceParameters instead.")
typedef DeviceParameters SceneScanParameters;
#endif

} // namespace

#endif
