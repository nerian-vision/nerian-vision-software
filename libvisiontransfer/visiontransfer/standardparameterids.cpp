#include <visiontransfer/standardparameterids.h>

namespace visiontransfer {
namespace internal {

/// Return the ID for a string configuration key (reverse lookup), or UNDEFINED if unknown
StandardParameterIDs::ParameterID StandardParameterIDs::getParameterIDForName(const std::string& name)
{
    static std::map<std::string, StandardParameterIDs::ParameterID> lookup;
    if (!lookup.size()) {
        std::map<std::string, StandardParameterIDs::ParameterID> m;
        for (const auto& kv: StandardParameterIDs::parameterNameByID) {
            m[kv.second] = kv.first;
        }
        lookup = m;
    }
    auto it = lookup.find(name);
    if (it==lookup.end()) return StandardParameterIDs::ParameterID::UNDEFINED;
    return it->second;
}

const std::map<StandardParameterIDs::ParameterID, std::string>
StandardParameterIDs::parameterNameByID {
    // Processing settings
        {OPERATION_MODE, "operation_mode"},
        {NUMBER_OF_DISPARITIES, "number_of_disparities"},
        {DISPARITY_OFFSET, "disparity_offset"},
        {MAX_NUMBER_OF_IMAGES, "max_number_of_images"},
    // Algorithmic settings
        {SGM_P1_EDGE, "sgm_p1_edge"},
        {SGM_P2_EDGE, "sgm_p2_edge"},
        {SGM_P1_NO_EDGE, "sgm_p1_no_edge"},
        {SGM_P2_NO_EDGE, "sgm_p2_no_edge"},
        {SGM_EDGE_SENSITIVITY, "sgm_edge_sensitivity"},
        {MASK_BORDER_PIXELS_ENABLED, "mask_border_pixels_enabled"},
        {CONSISTENCY_CHECK_ENABLED, "consistency_check_enabled"},
        {CONSISTENCY_CHECK_SENSITIVITY, "consistency_check_sensitivity"},
        {UNIQUENESS_CHECK_ENABLED, "uniqueness_check_enabled"},
        {UNIQUENESS_CHECK_SENSITIVITY, "uniqueness_check_sensitivity"},
        {TEXTURE_FILTER_ENABLED, "texture_filter_enabled"},
        {TEXTURE_FILTER_SENSITIVITY, "texture_filter_sensitivity"},
        {GAP_INTERPOLATION_ENABLED, "gap_interpolation_enabled"},
        {NOISE_REDUCTION_ENABLED, "noise_reduction_enabled"},
        {SPECKLE_FILTER_ITERATIONS, "speckle_filter_iterations"},
        {SUBPIXEL_OPTIMIZATION_ROI_ENABLED, "subpixel_optimization_roi_enabled"},
        {SUBPIXEL_OPTIMIZATION_ROI_X, "subpixel_optimization_roi_x"},
        {SUBPIXEL_OPTIMIZATION_ROI_Y, "subpixel_optimization_roi_y"},
        {SUBPIXEL_OPTIMIZATION_ROI_WIDTH, "subpixel_optimization_roi_width"},
        {SUBPIXEL_OPTIMIZATION_ROI_HEIGHT, "subpixel_optimization_roi_height"},
    // Exposure settings
        {AUTO_EXPOSURE_MODE, "auto_exposure_mode"},
        {AUTO_TARGET_INTENSITY, "auto_target_intensity"},
        {AUTO_INTENSITY_DELTA, "auto_intensity_delta"},
        {AUTO_TARGET_FRAME, "auto_target_frame"},
        {AUTO_SKIPPED_FRAMES, "auto_skipped_frames"},
        {AUTO_MAXIMUM_EXPOSURE_TIME, "auto_maximum_exposure_time"},
        {AUTO_MAXIMUM_GAIN, "auto_maximum_gain"},
        {MANUAL_EXPOSURE_TIME, "manual_exposure_time"},
        {MANUAL_GAIN, "manual_gain"},
        {AUTO_EXPOSURE_ROI_ENABLED, "auto_exposure_roi_enabled"},
        {AUTO_EXPOSURE_ROI_X, "auto_exposure_roi_x"},
        {AUTO_EXPOSURE_ROI_Y, "auto_exposure_roi_y"},
        {AUTO_EXPOSURE_ROI_WIDTH, "auto_exposure_roi_width"},
        {AUTO_EXPOSURE_ROI_HEIGHT, "auto_exposure_roi_height"},
    // Trigger / Pairing
        {MAX_FRAME_TIME_DIFFERENCE_MS, "max_frame_time_difference_ms"},
        {TRIGGER_FREQUENCY, "trigger_frequency"},
        {TRIGGER_0_ENABLED, "trigger_0_enabled"},
        {TRIGGER_0_PULSE_WIDTH, "trigger_0_pulse_width"},
        {TRIGGER_1_ENABLED, "trigger_1_enabled"},
        {TRIGGER_1_PULSE_WIDTH, "trigger_1_pulse_width"},
        {TRIGGER_1_OFFSET, "trigger_1_offset"},
        {TRIGGER_0B_PULSE_WIDTH, "trigger_0b_pulse_width"},
        {TRIGGER_0C_PULSE_WIDTH, "trigger_0c_pulse_width"},
        {TRIGGER_0D_PULSE_WIDTH, "trigger_0d_pulse_width"},
        {TRIGGER_1B_PULSE_WIDTH, "trigger_1b_pulse_width"},
        {TRIGGER_1C_PULSE_WIDTH, "trigger_1c_pulse_width"},
        {TRIGGER_1D_PULSE_WIDTH, "trigger_1d_pulse_width"},
        {TRIGGER_0_POLARITY, "trigger_0_polarity"},
        {TRIGGER_1_POLARITY, "trigger_1_polarity"},
        {TRIGGER_0E_PULSE_WIDTH, "trigger_0e_pulse_width"},
        {TRIGGER_0F_PULSE_WIDTH, "trigger_0f_pulse_width"},
        {TRIGGER_0G_PULSE_WIDTH, "trigger_0g_pulse_width"},
        {TRIGGER_0H_PULSE_WIDTH, "trigger_0h_pulse_width"},
        {TRIGGER_1E_PULSE_WIDTH, "trigger_1e_pulse_width"},
        {TRIGGER_1F_PULSE_WIDTH, "trigger_1f_pulse_width"},
        {TRIGGER_1G_PULSE_WIDTH, "trigger_1g_pulse_width"},
        {TRIGGER_1H_PULSE_WIDTH, "trigger_1h_pulse_width"},
        {TRIGGER_0_CONSTANT, "trigger_0_constant"},
        {TRIGGER_1_CONSTANT, "trigger_1_constant"},
        {TRIGGER_INPUT, "trigger_input"},
    // Auto Re-calibration
        {AUTO_RECALIBRATION_ENABLED, "auto_recalibration_enabled"},
        {AUTO_RECALIBRATION_PERMANENT, "auto_recalibration_permanent"},
    // System settings
        {REBOOT, "reboot"},
        {PPS_SYNC, "pps_sync"},
    };

}} // namespace


