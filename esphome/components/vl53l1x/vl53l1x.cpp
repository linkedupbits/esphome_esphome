#include "vl53l1x.h"
#include "esphome/core/log.h"

#include <cinttypes>

/*
 * Most of the code in this integration is based on the VL53L1X library
 * by Pololu (Pololu Corporation), which in turn is based on the VL53L1X
 * API from ST.
 *
 * For more information about licensing, please view the included LICENSE.txt file
 * in the vl53l1x integration directory.
 */

namespace esphome::vl53l1x {

static const char *const TAG = "vl53l1x";

#ifndef VL53L1X_EXCLUDE_OUTPUT_ERRORS_AND_WARNINGS
namespace {

struct DriverError {
  int8_t code;
  const char *occurrence;
};

// Table 7, "Bare driver errors and warnings descriptions", from ST's UM2356 VL53L1X API user manual. Only the
// entries this driver can actually produce are annotated with where they're reported from; the rest are included
// for completeness/fidelity to the table (e.g. for external tooling matching codes logged elsewhere), but this
// driver doesn't implement the SPAD-ref/zone calibration features some of them describe.
constexpr DriverError DRIVER_ERRORS[] = {
    {0, "No error"},                                                               // VL53L1X_ERROR_NONE
    {-1, "Invalid calibration data"},                                              // VL53L1X_ERROR_CALIBRATION_WARNING
    {-4, "Invalid parameter is set in a function"},                                // VL53L1X_ERROR_INVALID_PARAMS
    {-5, "Requested parameter is not supported in the programmed configuration"},  // VL53L1X_ERROR_NOT_SUPPORTED
    {-6, "Interrupt status is incorrect"},                                         // VL53L1X_ERROR_RANGE_ERROR
    {-7, "Ranging is aborted due to timeout"},                                     // VL53L1X_ERROR_TIME_OUT
    {-8, "Requested mode is not supported"},                                       // VL53L1X_ERROR_MODE_NOT_SUPPORTED
    {-10, "Supplied buffer is larger than I2C supports"},          // VL53L1X_ERROR_COMMS_BUFFER_TOO_SMALL
    {-14, "Command is invalid in current mode"},                   // VL53L1X_ERROR_INVALID_COMMAND
    {-16, "An error occurred during reference SPAD calibration"},  // VL53L1X_ERROR_REF_SPAD_INIT
    {-22, "Crosstalk calibration has no successful samples to compute the crosstalk"},
    // VL53L1X_ERROR_XTALK_EXTRACTION_NO_SAMPLE_FAIL
    {-23, "Crosstalk calibration sigma estimate is above the maximal limit allowed"},
    // VL53L1X_ERROR_XTALK_EXTRACTION_SIGMA_LIMIT_FAIL
    {-24, "Offset calibration found no valid ranging"},  // VL53L1X_ERROR_OFFSET_CAL_NO_SAMPLE_FAIL
    {-28, "Fewer than five good SPADs available"},       // VL53L1X_WARNING_REF_SPAD_CHAR_NOT_ENOUGH_SPADS
    {-29, "Final reference rate is greater than the upper reference rate limit"},
    // VL53L1X_WARNING_REF_SPAD_CHAR_RATE_TOO_HIGH
    {-30, "Final reference rate is less than the lower reference rate limit"},
    // VL53L1X_WARNING_REF_SPAD_CHAR_RATE_TOO_LOW
    {-31, "Fewer valid samples than requested"},                   // VL53L1X_WARNING_OFFSET_CAL_MISSING_SAMPLES
    {-32, "Offset calibration range sigma estimate is too high"},  // VL53L1X_WARNING_OFFSET_CAL_SIGMA_TOO_HIGH
    {-33, "Signal rate is greater than a limit and sensor is saturating"},
    // VL53L1X_WARNING_OFFSET_CAL_RATE_TOO_HIGH
    {-34, "Not enough SPADs can be used"},        // VL53L1X_WARNING_OFFSET_CAL_SPAD_COUNT_TOO_LOW
    {-41, "Function called is not implemented"},  // VL53L1X_ERROR_NOT_IMPLEMENTED
};

}  // namespace
#endif  // !VL53L1X_EXCLUDE_OUTPUT_ERRORS_AND_WARNINGS

void VL53L1XComponent::report_error_(int8_t error_code) {
#ifndef VL53L1X_EXCLUDE_OUTPUT_ERRORS_AND_WARNINGS
  if (this->error_sensor_ == nullptr) {
    return;
  }
  for (const auto &entry : DRIVER_ERRORS) {
    if (entry.code == error_code) {
      this->error_sensor_->publish_state(entry.occurrence);
      return;
    }
  }
  this->error_sensor_->publish_state("Unknown error");
#else
  (void) error_code;
#endif
}

std::list<VL53L1XComponent *>
    VL53L1XComponent::vl53_sensors;                        // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
bool VL53L1XComponent::enable_pin_setup_complete = false;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

VL53L1XComponent::VL53L1XComponent() { VL53L1XComponent::vl53_sensors.push_back(this); }

void VL53L1XComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "VL53L1X:");
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
  if (this->enable_pin_ != nullptr) {
    LOG_PIN("  Enable Pin: ", this->enable_pin_);
  }
  ESP_LOGCONFIG(TAG,
                "  Distance Mode: %s\n"
                "  Timing Budget: %" PRIu32 "us\n"
                "  Timeout: %" PRIu32 "%s\n"
                "  ROI: %ux%u",
                VL53L1XComponent::distance_mode_to_string_(this->distance_mode_), this->measurement_timing_budget_us_,
                this->timeout_us_, this->timeout_us_ > 0 ? "us" : " (no timeout)", this->roi_width_, this->roi_height_);
  if (this->roi_center_spad_.has_value()) {
    ESP_LOGCONFIG(TAG, "  ROI Center SPAD: %u", *this->roi_center_spad_);
  }
  ESP_LOGCONFIG(TAG, "  Signal Threshold: %.3fMcps", this->signal_threshold_raw_ / 128.0f);
  ESP_LOGCONFIG(TAG, "  Sigma Threshold: %umm", this->sigma_threshold_mm_);
  if (this->distance_threshold_enabled_) {
    ESP_LOGCONFIG(TAG, "  Distance Threshold: %s [%u, %u]mm, Report No Target: %s",
                  VL53L1XComponent::distance_threshold_window_to_string_(this->distance_threshold_window_),
                  this->distance_threshold_low_mm_, this->distance_threshold_high_mm_,
                  ONOFF(this->distance_threshold_report_no_target_));
  }
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with VL53L1X failed!");
  }
}

const char *VL53L1XComponent::distance_mode_to_string_(DistanceMode mode) {
  switch (mode) {
    case DistanceMode::SHORT:
      return "short";
    case DistanceMode::MEDIUM:
      return "medium";
    case DistanceMode::LONG:
    default:
      return "long";
  }
}

const char *VL53L1XComponent::distance_threshold_window_to_string_(DistanceThresholdWindow window) {
  switch (window) {
    case DistanceThresholdWindow::BELOW:
      return "below";
    case DistanceThresholdWindow::ABOVE:
      return "above";
    case DistanceThresholdWindow::OUTSIDE_WINDOW:
      return "outside window";
    case DistanceThresholdWindow::INSIDE_WINDOW:
    default:
      return "inside window";
  }
}

void VL53L1XComponent::set_timeout_us(uint32_t timeout_us) {
  this->timeout_us_ = timeout_us;
  if (this->setup_complete_) {
    this->publish_timeout_();
  }
}

void VL53L1XComponent::set_distance_mode(DistanceMode mode) {
  if (this->setup_complete_) {
    // updates this->distance_mode_ and reapplies the current timing budget with the new VCSEL periods
    this->set_distance_mode_(mode);
    this->publish_distance_mode_();
  } else {
    this->distance_mode_ = mode;
  }
}

void VL53L1XComponent::set_timing_budget(uint32_t timing_budget_us) {
  if (this->setup_complete_) {
    if (!this->set_measurement_timing_budget_(timing_budget_us)) {
      ESP_LOGW(TAG, "0x%02X - could not apply timing budget of %" PRIu32 "us", this->address_, timing_budget_us);
      return;
    }
    this->measurement_timing_budget_us_ = timing_budget_us;
    this->publish_timing_budget_();
  } else {
    this->measurement_timing_budget_us_ = timing_budget_us;
  }
}

void VL53L1XComponent::publish_distance_mode_() {
  if (this->distance_mode_sensor_ != nullptr) {
    this->distance_mode_sensor_->publish_state(VL53L1XComponent::distance_mode_to_string_(this->distance_mode_));
  }
}

void VL53L1XComponent::publish_timing_budget_() {
  if (this->timing_budget_sensor_ != nullptr) {
    this->timing_budget_sensor_->publish_state(this->measurement_timing_budget_us_ / 1000.0f);
  }
}

void VL53L1XComponent::publish_timeout_() {
  if (this->timeout_sensor_ != nullptr) {
    this->timeout_sensor_->publish_state(this->timeout_us_ / 1000.0f);
  }
}

void VL53L1XComponent::set_offset_mm(int16_t offset_mm) {
  this->offset_mm_ = offset_mm;
  if (this->setup_complete_) {
    this->write_reg16_(REG_ALGO_PART_TO_PART_RANGE_OFFSET_MM, static_cast<uint16_t>(offset_mm * 4));
  }
}

void VL53L1XComponent::set_xtalk_cps(uint16_t xtalk_cps) {
  this->xtalk_cps_ = xtalk_cps;
  if (this->setup_complete_) {
    this->write_reg16_(REG_ALGO_CROSSTALK_COMPENSATION_PLANE_OFFSET_KCPS, xtalk_cps);
  }
}

void VL53L1XComponent::set_roi(uint8_t width, uint8_t height) {
  if (width < 4) {
    width = 4;
  } else if (width > 16) {
    width = 16;
  }
  if (height < 4) {
    height = 4;
  } else if (height > 16) {
    height = 16;
  }
  this->roi_width_ = width;
  this->roi_height_ = height;
  if (this->setup_complete_) {
    this->apply_roi_();
  }
}

void VL53L1XComponent::set_roi_center_spad(uint8_t center_spad) {
  this->roi_center_spad_ = center_spad;
  if (this->setup_complete_) {
    this->apply_roi_();
  }
}

void VL53L1XComponent::apply_roi_() {
  // Based on VL53L1X_SetROI() in ST's VL53L1X ULD API.
  uint8_t center_spad;
  if (this->roi_center_spad_.has_value()) {
    center_spad = *this->roi_center_spad_;
  } else {
    center_spad = this->read_reg_(REG_ROI_CONFIG_MODE_ROI_CENTRE_SPAD);
    if (this->roi_width_ > 10 || this->roi_height_ > 10) {
      // The factory-calibrated optical center can't fit an ROI this large without exceeding the array bounds;
      // fall back to the geometric center of the full 16x16 array.
      center_spad = 199;
    }
  }
  this->write_reg_(REG_ROI_CONFIG_USER_ROI_CENTRE_SPAD, center_spad);
  this->write_reg_(REG_ROI_CONFIG_USER_ROI_REQUESTED_GLOBAL_XY_SIZE,
                   static_cast<uint8_t>((this->roi_height_ - 1) << 4 | (this->roi_width_ - 1)));
}

void VL53L1XComponent::set_signal_threshold_mcps(float mcps) {
  // 9.7 fixed-point format, matching RANGE_CONFIG__MIN_COUNT_RATE_RTN_LIMIT_MCPS.
  this->signal_threshold_raw_ = static_cast<uint16_t>(mcps * 128.0f + 0.5f);
  if (this->setup_complete_) {
    this->write_reg16_(REG_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT_MCPS, this->signal_threshold_raw_);
  }
}

void VL53L1XComponent::set_sigma_threshold_mm(uint16_t mm) {
  this->sigma_threshold_mm_ = mm;
  if (this->setup_complete_) {
    // 14.2 fixed-point format, matching RANGE_CONFIG__SIGMA_THRESH.
    this->write_reg16_(REG_RANGE_CONFIG_SIGMA_THRESH, static_cast<uint16_t>(mm << 2));
  }
}

void VL53L1XComponent::set_distance_threshold(uint16_t low_mm, uint16_t high_mm, DistanceThresholdWindow window,
                                              bool report_no_target) {
  this->distance_threshold_enabled_ = true;
  this->distance_threshold_low_mm_ = low_mm;
  this->distance_threshold_high_mm_ = high_mm;
  this->distance_threshold_window_ = window;
  this->distance_threshold_report_no_target_ = report_no_target;
  if (this->setup_complete_) {
    this->apply_distance_threshold_();
  }
}

void VL53L1XComponent::apply_distance_threshold_() {
  if (!this->distance_threshold_enabled_) {
    return;
  }
  // Based on VL53L1X_SetDistanceThreshold() in ST's VL53L1X ULD API.
  uint8_t temp = this->read_reg_(REG_SYSTEM_INTERRUPT_CONFIG_GPIO) & 0x47;
  uint8_t window = static_cast<uint8_t>(this->distance_threshold_window_) & 0x07;
  temp |= window;
  if (this->distance_threshold_report_no_target_) {
    temp |= 0x40;
  }
  this->write_reg_(REG_SYSTEM_INTERRUPT_CONFIG_GPIO, temp);
  this->write_reg16_(REG_SYSTEM_THRESH_HIGH, this->distance_threshold_high_mm_);
  this->write_reg16_(REG_SYSTEM_THRESH_LOW, this->distance_threshold_low_mm_);
}

bool VL53L1XComponent::read_calibration_sample_(uint16_t &raw_distance_mm, uint16_t &signal_rate_cps,
                                                uint16_t &spad_count) {
  // Generous per-sample timeout: twice the configured timing budget, plus margin.
  uint32_t sample_timeout_us = this->measurement_timing_budget_us_ * 2 + 100000;
  uint32_t start_us = micros();
  while ((this->read_reg_(REG_GPIO_TIO_HV_STATUS) & 0x01) != 0) {
    if (micros() - start_us > sample_timeout_us) {
      return false;
    }
    yield();
  }

  uint8_t buf[17];
  this->read_register16(REG_RESULT_RANGE_STATUS, buf, sizeof(buf));

  uint16_t dss_actual_effective_spads_sd0 = (static_cast<uint16_t>(buf[3]) << 8) | buf[4];
  uint16_t peak_signal_count_rate_crosstalk_corrected_mcps_sd0 = (static_cast<uint16_t>(buf[15]) << 8) | buf[16];

  spad_count = dss_actual_effective_spads_sd0 >> 8;
  raw_distance_mm = (static_cast<uint16_t>(buf[13]) << 8) | buf[14];
  signal_rate_cps = peak_signal_count_rate_crosstalk_corrected_mcps_sd0 * 8;

  this->write_reg_(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
  return true;
}

bool VL53L1XComponent::calibrate_offset(uint16_t target_distance_mm) {
  if (!this->setup_complete_) {
    ESP_LOGW(TAG, "0x%02X - cannot calibrate offset before setup has completed", this->address_);
    return false;
  }
  if (this->reading_) {
    ESP_LOGW(TAG, "0x%02X - cannot calibrate offset while a measurement is in progress", this->address_);
    return false;
  }

  this->write_reg16_(REG_ALGO_PART_TO_PART_RANGE_OFFSET_MM, 0);
  this->write_reg16_(REG_MM_CONFIG_INNER_OFFSET_MM, 0);
  this->write_reg16_(REG_MM_CONFIG_OUTER_OFFSET_MM, 0);

  this->write_reg_(REG_SYSTEM_MODE_START, 0x40);  // start continuous back-to-back ranging

  int32_t distance_sum = 0;
  bool ok = true;
  for (uint8_t i = 0; i < CALIBRATION_SAMPLES; i++) {
    uint16_t raw_distance_mm;
    uint16_t signal_rate_cps;
    uint16_t spad_count;
    if (!this->read_calibration_sample_(raw_distance_mm, signal_rate_cps, spad_count)) {
      ok = false;
      break;
    }
    distance_sum += raw_distance_mm;
  }

  this->write_reg_(REG_SYSTEM_MODE_START, 0x00);  // stop ranging

  if (!ok) {
    ESP_LOGE(TAG, "0x%02X - offset calibration failed: sensor did not respond", this->address_);
    this->report_error_(-7);  // VL53L1X_ERROR_TIME_OUT
    return false;
  }

  auto avg_distance_mm = static_cast<int16_t>(distance_sum / CALIBRATION_SAMPLES);
  auto offset_mm = static_cast<int16_t>(static_cast<int16_t>(target_distance_mm) - avg_distance_mm);
  ESP_LOGI(TAG, "0x%02X - offset calibration complete: offset_mm: %d (measured %dmm at %umm target)", this->address_,
           offset_mm, avg_distance_mm, target_distance_mm);
  this->set_offset_mm(offset_mm);
  if (this->calibrated_offset_sensor_ != nullptr) {
    this->calibrated_offset_sensor_->publish_state(offset_mm);
  }
  return true;
}

bool VL53L1XComponent::calibrate_xtalk(uint16_t target_distance_mm) {
  if (!this->setup_complete_) {
    ESP_LOGW(TAG, "0x%02X - cannot calibrate crosstalk before setup has completed", this->address_);
    return false;
  }
  if (this->reading_) {
    ESP_LOGW(TAG, "0x%02X - cannot calibrate crosstalk while a measurement is in progress", this->address_);
    return false;
  }

  this->write_reg16_(REG_ALGO_CROSSTALK_COMPENSATION_PLANE_OFFSET_KCPS, 0);

  this->write_reg_(REG_SYSTEM_MODE_START, 0x40);  // start continuous back-to-back ranging

  uint32_t distance_sum = 0;
  uint32_t signal_sum = 0;
  uint32_t spad_sum = 0;
  bool ok = true;
  for (uint8_t i = 0; i < CALIBRATION_SAMPLES; i++) {
    uint16_t raw_distance_mm;
    uint16_t signal_rate_cps;
    uint16_t spad_count;
    if (!this->read_calibration_sample_(raw_distance_mm, signal_rate_cps, spad_count)) {
      ok = false;
      break;
    }
    distance_sum += raw_distance_mm;
    signal_sum += signal_rate_cps;
    spad_sum += spad_count;
  }

  this->write_reg_(REG_SYSTEM_MODE_START, 0x00);  // stop ranging

  if (!ok) {
    ESP_LOGE(TAG, "0x%02X - crosstalk calibration failed: sensor did not respond", this->address_);
    this->report_error_(-7);  // VL53L1X_ERROR_TIME_OUT
    return false;
  }
  if (spad_sum == 0) {
    ESP_LOGE(TAG, "0x%02X - crosstalk calibration failed: no successful samples", this->address_);
    this->report_error_(-22);  // VL53L1X_ERROR_XTALK_EXTRACTION_NO_SAMPLE_FAIL
    return false;
  }

  float avg_distance_mm = static_cast<float>(distance_sum) / CALIBRATION_SAMPLES;
  float avg_signal_cps = static_cast<float>(signal_sum) / CALIBRATION_SAMPLES;
  float avg_spad_count = static_cast<float>(spad_sum) / CALIBRATION_SAMPLES;

  // Based on VL53L1X_CalibrateXtalk() in ST's VL53L1X ULD API.
  float xtalk_f = 512.0f * (avg_signal_cps * (1.0f - (avg_distance_mm / target_distance_mm))) / avg_spad_count;
  uint16_t xtalk_cps = xtalk_f > 0 ? static_cast<uint16_t>(xtalk_f) : static_cast<uint16_t>(0);

  ESP_LOGI(TAG, "0x%02X - crosstalk calibration complete: xtalk_cps: %u", this->address_, xtalk_cps);
  this->set_xtalk_cps(xtalk_cps);
  if (this->calibrated_xtalk_sensor_ != nullptr) {
    this->calibrated_xtalk_sensor_->publish_state(xtalk_cps);
  }
  return true;
}

void VL53L1XComponent::setup() {
  if (!VL53L1XComponent::enable_pin_setup_complete) {
    for (auto &vl53_sensor : vl53_sensors) {
      if (vl53_sensor->enable_pin_ != nullptr) {
        // Set enable pin as OUTPUT and disable the enable pin to force vl53 to HW Standby mode
        vl53_sensor->enable_pin_->setup();
        vl53_sensor->enable_pin_->digital_write(false);
      }
    }
    VL53L1XComponent::enable_pin_setup_complete = true;
  }

  if (this->enable_pin_ != nullptr) {
    // Enable the enable pin to cause FW boot (to get back to 0x29 default address)
    this->enable_pin_->digital_write(true);
    delayMicroseconds(100);
  }

  // Save the i2c address we want and force it to use the default 0x29
  // until we finish setup, then re-address to final desired address.
  uint8_t final_address = this->address_;
  this->set_i2c_address(0x29);

  uint16_t model_id = this->read_reg16_(REG_IDENTIFICATION_MODEL_ID);
  if (model_id != MODEL_ID) {
    ESP_LOGE(TAG, "0x%02X - unexpected model id 0x%04X", final_address, model_id);
    this->mark_failed();
    return;
  }

  // VL53L1_software_reset()
  this->write_reg_(REG_SOFT_RESET, 0x00);
  delayMicroseconds(100);
  this->write_reg_(REG_SOFT_RESET, 0x01);
  delay(1);

  // VL53L1_poll_for_boot_completion()
  uint32_t timeout_start_us = micros();
  while ((this->read_reg_(REG_FIRMWARE_SYSTEM_STATUS) & 0x01) == 0) {
    if (this->timeout_us_ > 0 && (micros() - timeout_start_us > this->timeout_us_)) {
      ESP_LOGE(TAG, "0x%02X - setup timeout", final_address);
      this->report_error_(-7);  // VL53L1X_ERROR_TIME_OUT
      this->mark_failed();
      return;
    }
    yield();
  }

  // VL53L1_DataInit(): switch to 2V8 I/O mode and stash oscillator calibration for later timing calculations
  this->write_reg_(REG_PAD_I2C_HV_EXTSUP_CONFIG, this->read_reg_(REG_PAD_I2C_HV_EXTSUP_CONFIG) | 0x01);
  this->fast_osc_frequency_ = this->read_reg16_(REG_OSC_MEASURED_FAST_OSC_FREQUENCY);

  // VL53L1_StaticInit(): values labeled "tuning parm default" are from vl53l1_tuning_parm_defaults.h in ST's API.
  // static config
  this->write_reg16_(REG_DSS_CONFIG_TARGET_TOTAL_RATE_MCPS, TARGET_RATE);
  this->write_reg_(REG_GPIO_TIO_HV_STATUS, 0x02);
  this->write_reg_(REG_SIGMA_ESTIMATOR_EFFECTIVE_PULSE_WIDTH_NS, 8);
  this->write_reg_(REG_SIGMA_ESTIMATOR_EFFECTIVE_AMBIENT_WIDTH_NS, 16);
  this->write_reg_(REG_ALGO_CROSSTALK_COMPENSATION_VALID_HEIGHT_MM, 0x01);
  this->write_reg_(REG_ALGO_RANGE_IGNORE_VALID_HEIGHT_MM, 0xFF);
  this->write_reg_(REG_ALGO_RANGE_MIN_CLIP, 0);
  this->write_reg_(REG_ALGO_CONSISTENCY_CHECK_TOLERANCE, 2);

  // general config
  this->write_reg16_(REG_SYSTEM_THRESH_RATE_HIGH, 0x0000);
  this->write_reg16_(REG_SYSTEM_THRESH_RATE_LOW, 0x0000);
  this->write_reg_(REG_DSS_CONFIG_APERTURE_ATTENUATION, 0x38);

  // timing config - most of this is overwritten below by distance mode / timing budget configuration
  this->write_reg16_(REG_RANGE_CONFIG_SIGMA_THRESH, static_cast<uint16_t>(this->sigma_threshold_mm_ << 2));
  this->write_reg16_(REG_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT_MCPS, this->signal_threshold_raw_);

  // dynamic config
  this->write_reg_(REG_SYSTEM_GROUPED_PARAMETER_HOLD_0, 0x01);
  this->write_reg_(REG_SYSTEM_GROUPED_PARAMETER_HOLD_1, 0x01);
  this->write_reg_(REG_SD_CONFIG_QUANTIFIER, 2);

  // GPH is 0 after reset, but writing GPH0 and GPH1 above sets GPH to 1; things don't work properly unless it is
  // set back to 0 here.
  this->write_reg_(REG_SYSTEM_GROUPED_PARAMETER_HOLD, 0x00);
  this->write_reg_(REG_SYSTEM_SEED_CONFIG, 1);

  // low power autonomous mode
  this->write_reg_(REG_SYSTEM_SEQUENCE_CONFIG, 0x8B);  // VHV, PHASECAL, DSS1, RANGE
  this->write_reg16_(REG_DSS_CONFIG_MANUAL_EFFECTIVE_SPADS_SELECT, 200 << 8);
  this->write_reg_(REG_DSS_CONFIG_ROI_MODE_CONTROL, 2);  // REQUESTED_EFFECTIVE_SPADS

  this->set_distance_mode_(this->distance_mode_);
  if (!this->set_measurement_timing_budget_(this->measurement_timing_budget_us_)) {
    ESP_LOGW(TAG, "0x%02X - could not apply timing budget of %" PRIu32 "us", final_address,
             this->measurement_timing_budget_us_);
  }

  // the API triggers this change once a measurement is started; assumes MM1 and MM2 are disabled
  this->write_reg16_(REG_ALGO_PART_TO_PART_RANGE_OFFSET_MM, this->read_reg16_(REG_MM_CONFIG_OUTER_OFFSET_MM) * 4);

  // Apply any fixed calibration values from config (see calibrate_offset()/calibrate_xtalk()), overriding the
  // zeroed defaults set above.
  if (this->offset_mm_ != 0) {
    this->write_reg16_(REG_ALGO_PART_TO_PART_RANGE_OFFSET_MM, static_cast<uint16_t>(this->offset_mm_ * 4));
  }
  if (this->xtalk_cps_ != 0) {
    this->write_reg16_(REG_ALGO_CROSSTALK_COMPENSATION_PLANE_OFFSET_KCPS, this->xtalk_cps_);
  }
  this->apply_roi_();
  this->apply_distance_threshold_();

  // Set the sensor to the desired final address
  this->write_reg_(REG_I2C_DEVICE_ADDRESS, final_address & 0x7F);
  this->set_i2c_address(final_address);

  this->setup_complete_ = true;
  this->publish_distance_mode_();
  this->publish_timing_budget_();
  this->publish_timeout_();
}

void VL53L1XComponent::update() {
  if (this->reading_) {
    if (this->distance_sensor_ != nullptr) {
      this->distance_sensor_->publish_state(NAN);
    }
    this->status_momentary_warning("update", 5000);
    ESP_LOGW(TAG, "0x%02X - update called before prior reading complete", this->address_);
    return;
  }

  // start a single-shot ranging measurement
  this->write_reg_(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
  this->write_reg_(REG_SYSTEM_MODE_START, 0x10);  // mode_range__single_shot
  this->reading_ = true;
  this->reading_start_us_ = micros();
}

void VL53L1XComponent::loop() {
  if (!this->reading_) {
    return;
  }

  // GPIO interrupt is active low: a new sample is ready once bit 0 of GPIO__TIO_HV_STATUS clears. While a
  // distance threshold is configured (see set_distance_threshold()), a measurement that doesn't match the
  // configured window never asserts ready at all -- give up once the next update() is nearly due, rather than
  // blocking indefinitely.
  if (this->read_reg_(REG_GPIO_TIO_HV_STATUS) & 0x01) {
    if (this->distance_threshold_enabled_ && micros() - this->reading_start_us_ > this->get_update_interval() * 1000) {
      ESP_LOGD(TAG, "0x%02X - no qualifying measurement before next update", this->address_);
      this->reading_ = false;
      if (this->distance_sensor_ != nullptr) {
        this->distance_sensor_->publish_state(NAN);
      }
    }
    return;
  }
  this->reading_ = false;

  uint8_t buf[17];
  this->read_register16(REG_RESULT_RANGE_STATUS, buf, sizeof(buf));

  uint8_t raw_range_status = buf[0];
  uint8_t stream_count = buf[2];
  uint16_t dss_actual_effective_spads_sd0 = (static_cast<uint16_t>(buf[3]) << 8) | buf[4];
  uint16_t ambient_count_rate_mcps_sd0 = (static_cast<uint16_t>(buf[7]) << 8) | buf[8];
  uint16_t final_crosstalk_corrected_range_mm_sd0 = (static_cast<uint16_t>(buf[13]) << 8) | buf[14];
  uint16_t peak_signal_count_rate_crosstalk_corrected_mcps_sd0 = (static_cast<uint16_t>(buf[15]) << 8) | buf[16];
  RangeStatus range_status = VL53L1XComponent::convert_range_status_(raw_range_status, stream_count);

  // "Setup ranges after the first one in low power auto mode by turning off FW calibration steps and programming
  // static values" - based on VL53L1_low_power_auto_setup_manual_calibration()
  if (!this->calibrated_) {
    this->setup_manual_calibration_();
    this->calibrated_ = true;
  }
  this->update_dss_(dss_actual_effective_spads_sd0, ambient_count_rate_mcps_sd0,
                    peak_signal_count_rate_crosstalk_corrected_mcps_sd0);

  this->write_reg_(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);

  // "apply correction gain": scales the raw range by 2011/2048 (~98%), a tuning parm default from ST's API
  uint32_t range_mm = (static_cast<uint32_t>(final_crosstalk_corrected_range_mm_sd0) * 2011 + 0x0400) / 0x0800;

  if (range_status != RangeStatus::RANGE_VALID) {
    ESP_LOGD(TAG, "0x%02X - distance measurement not valid (status=%u)", this->address_,
             static_cast<uint8_t>(range_status));
    if (this->distance_sensor_ != nullptr) {
      this->distance_sensor_->publish_state(NAN);
    }
    return;
  }

  float range_m = range_mm / 1e3f;
  ESP_LOGD(TAG, "0x%02X - Got distance %.3f m", this->address_, range_m);
  if (this->distance_sensor_ != nullptr) {
    this->distance_sensor_->publish_state(range_m);
  }
}

void VL53L1XComponent::set_distance_mode_(DistanceMode mode) {
  // save existing timing budget so it can be reapplied with the new VCSEL periods
  uint32_t budget_us = this->get_measurement_timing_budget_();

  switch (mode) {
    case DistanceMode::SHORT:
      this->write_reg_(REG_RANGE_CONFIG_VCSEL_PERIOD_A, 0x07);
      this->write_reg_(REG_RANGE_CONFIG_VCSEL_PERIOD_B, 0x05);
      this->write_reg_(REG_RANGE_CONFIG_VALID_PHASE_HIGH, 0x38);
      this->write_reg_(REG_SD_CONFIG_WOI_SD0, 0x07);
      this->write_reg_(REG_SD_CONFIG_WOI_SD1, 0x05);
      this->write_reg_(REG_SD_CONFIG_INITIAL_PHASE_SD0, 6);
      this->write_reg_(REG_SD_CONFIG_INITIAL_PHASE_SD1, 6);
      break;
    case DistanceMode::MEDIUM:
      this->write_reg_(REG_RANGE_CONFIG_VCSEL_PERIOD_A, 0x0B);
      this->write_reg_(REG_RANGE_CONFIG_VCSEL_PERIOD_B, 0x09);
      this->write_reg_(REG_RANGE_CONFIG_VALID_PHASE_HIGH, 0x78);
      this->write_reg_(REG_SD_CONFIG_WOI_SD0, 0x0B);
      this->write_reg_(REG_SD_CONFIG_WOI_SD1, 0x09);
      this->write_reg_(REG_SD_CONFIG_INITIAL_PHASE_SD0, 10);
      this->write_reg_(REG_SD_CONFIG_INITIAL_PHASE_SD1, 10);
      break;
    case DistanceMode::LONG:
    default:
      this->write_reg_(REG_RANGE_CONFIG_VCSEL_PERIOD_A, 0x0F);
      this->write_reg_(REG_RANGE_CONFIG_VCSEL_PERIOD_B, 0x0D);
      this->write_reg_(REG_RANGE_CONFIG_VALID_PHASE_HIGH, 0xB8);
      this->write_reg_(REG_SD_CONFIG_WOI_SD0, 0x0F);
      this->write_reg_(REG_SD_CONFIG_WOI_SD1, 0x0D);
      this->write_reg_(REG_SD_CONFIG_INITIAL_PHASE_SD0, 14);
      this->write_reg_(REG_SD_CONFIG_INITIAL_PHASE_SD1, 14);
      break;
  }

  this->set_measurement_timing_budget_(budget_us);
  this->distance_mode_ = mode;
}

uint32_t VL53L1XComponent::get_measurement_timing_budget_() {
  // assumes these sequence steps are enabled: VHV, PHASECAL, DSS1, RANGE
  uint32_t macro_period_us = this->calc_macro_period_(this->read_reg_(REG_RANGE_CONFIG_VCSEL_PERIOD_A));
  uint32_t range_config_timeout_us = VL53L1XComponent::timeout_mclks_to_us_(
      VL53L1XComponent::decode_timeout_(this->read_reg16_(REG_RANGE_CONFIG_TIMEOUT_MACROP_A)), macro_period_us);
  return 2 * range_config_timeout_us + TIMING_GUARD_US;
}

bool VL53L1XComponent::set_measurement_timing_budget_(uint32_t budget_us) {
  // assumes low power autonomous preset mode
  if (budget_us <= TIMING_GUARD_US) {
    return false;
  }
  uint32_t range_config_timeout_us = budget_us - TIMING_GUARD_US;
  if (range_config_timeout_us > 1100000) {  // FDA_MAX_TIMING_BUDGET_US * 2
    return false;
  }
  range_config_timeout_us /= 2;

  // "Update Macro Period for Range A VCSEL Period"
  uint32_t macro_period_us = this->calc_macro_period_(this->read_reg_(REG_RANGE_CONFIG_VCSEL_PERIOD_A));

  // "Update Phase timeout - uses Timing A". 1000 is the tuning parm default
  // (TIMED_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT).
  uint32_t phasecal_timeout_mclks = VL53L1XComponent::timeout_us_to_mclks_(1000, macro_period_us);
  if (phasecal_timeout_mclks > 0xFF) {
    phasecal_timeout_mclks = 0xFF;
  }
  this->write_reg_(REG_PHASECAL_CONFIG_TIMEOUT_MACROP, phasecal_timeout_mclks);

  // "Update MM Timing A timeout". Low power auto mode disables the MM sequence steps, so the value written here
  // probably doesn't matter, but the API still assigns it.
  this->write_reg16_(REG_MM_CONFIG_TIMEOUT_MACROP_A,
                     VL53L1XComponent::encode_timeout_(VL53L1XComponent::timeout_us_to_mclks_(1, macro_period_us)));

  // "Update Range Timing A timeout"
  this->write_reg16_(REG_RANGE_CONFIG_TIMEOUT_MACROP_A,
                     VL53L1XComponent::encode_timeout_(
                         VL53L1XComponent::timeout_us_to_mclks_(range_config_timeout_us, macro_period_us)));

  // "Update Macro Period for Range B VCSEL Period"
  macro_period_us = this->calc_macro_period_(this->read_reg_(REG_RANGE_CONFIG_VCSEL_PERIOD_B));

  // "Update MM Timing B timeout"
  this->write_reg16_(REG_MM_CONFIG_TIMEOUT_MACROP_B,
                     VL53L1XComponent::encode_timeout_(VL53L1XComponent::timeout_us_to_mclks_(1, macro_period_us)));

  // "Update Range Timing B timeout"
  this->write_reg16_(REG_RANGE_CONFIG_TIMEOUT_MACROP_B,
                     VL53L1XComponent::encode_timeout_(
                         VL53L1XComponent::timeout_us_to_mclks_(range_config_timeout_us, macro_period_us)));

  return true;
}

void VL53L1XComponent::setup_manual_calibration_() {
  // "save original vhv configs"
  this->saved_vhv_init_ = this->read_reg_(REG_VHV_CONFIG_INIT);
  this->saved_vhv_timeout_ = this->read_reg_(REG_VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND);

  // "disable VHV init"
  this->write_reg_(REG_VHV_CONFIG_INIT, this->saved_vhv_init_ & 0x7F);

  // "set loop bound to tuning param" (LOWPOWERAUTO_VHV_LOOP_BOUND_DEFAULT)
  this->write_reg_(REG_VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND, (this->saved_vhv_timeout_ & 0x03) + (3 << 2));

  // "override phasecal"
  this->write_reg_(REG_PHASECAL_CONFIG_OVERRIDE, 0x01);
  this->write_reg_(REG_CAL_CONFIG_VCSEL_START, this->read_reg_(REG_PHASECAL_RESULT_VCSEL_START));
}

void VL53L1XComponent::update_dss_(uint16_t spad_count, uint16_t ambient_count_rate_mcps,
                                   uint16_t peak_signal_count_rate_mcps) {
  // based on VL53L1_low_power_auto_update_DSS()
  if (spad_count != 0) {
    uint32_t total_rate_per_spad = static_cast<uint32_t>(peak_signal_count_rate_mcps) + ambient_count_rate_mcps;
    if (total_rate_per_spad > 0xFFFF) {
      total_rate_per_spad = 0xFFFF;
    }
    total_rate_per_spad <<= 16;
    total_rate_per_spad /= spad_count;

    if (total_rate_per_spad != 0) {
      uint32_t required_spads = (static_cast<uint32_t>(TARGET_RATE) << 16) / total_rate_per_spad;
      if (required_spads > 0xFFFF) {
        required_spads = 0xFFFF;
      }
      this->write_reg16_(REG_DSS_CONFIG_MANUAL_EFFECTIVE_SPADS_SELECT, required_spads);
      return;
    }
  }

  // Something above would have resulted in a divide by zero; gracefully set the target to the mid point instead.
  this->write_reg16_(REG_DSS_CONFIG_MANUAL_EFFECTIVE_SPADS_SELECT, 0x8000);
}

RangeStatus VL53L1XComponent::convert_range_status_(uint8_t raw_status, uint8_t stream_count) {
  // Based on ConvertStatusLite() in ST's API: maps the raw RESULT_RANGE_STATUS register value to one of the
  // human-meaningful statuses in the RangeStatus enum above.
  switch (raw_status) {
    case 1:   // VCSELCONTINUITYTESTFAILURE
    case 2:   // VCSELWATCHDOGTESTFAILURE
    case 3:   // NOVHVVALUEFOUND
    case 17:  // MULTCLIPFAIL
      return RangeStatus::HARDWARE_FAIL;
    case 4:  // MSRCNOTARGET
      return RangeStatus::SIGNAL_FAIL;
    case 5:  // RANGEPHASECHECK
      return RangeStatus::OUT_OF_BOUNDS_FAIL;
    case 6:  // SIGMATHRESHOLDCHECK
      return RangeStatus::SIGMA_FAIL;
    case 7:  // PHASECONSISTENCY
      return RangeStatus::WRAP_TARGET_FAIL;
    case 8:  // MINCLIP
      return RangeStatus::RANGE_VALID_MIN_RANGE_CLIPPED;
    case 9:  // RANGECOMPLETE
      return stream_count == 0 ? RangeStatus::RANGE_VALID_NO_WRAP_CHECK_FAIL : RangeStatus::RANGE_VALID;
    case 12:  // RANGEIGNORETHRESHOLD
      return RangeStatus::XTALK_SIGNAL_FAIL;
    case 13:  // USERROICLIP
      return RangeStatus::MIN_RANGE_FAIL;
    case 18:  // GPHSTREAMCOUNT0READY
      return RangeStatus::SYNCHRONIZATION_INT;
    default:
      return RangeStatus::NONE;
  }
}

uint16_t VL53L1XComponent::decode_timeout_(uint16_t reg_val) {
  // format: "(LSByte * 2^MSByte) + 1"
  return ((reg_val & 0xFF) << (reg_val >> 8)) + 1;
}

uint16_t VL53L1XComponent::encode_timeout_(uint32_t timeout_mclks) {
  // format: "(LSByte * 2^MSByte) + 1"
  if (timeout_mclks == 0) {
    return 0;
  }

  uint32_t ls_byte = timeout_mclks - 1;
  uint16_t ms_byte = 0;
  while ((ls_byte & 0xFFFFFF00) > 0) {
    ls_byte >>= 1;
    ms_byte++;
  }

  return (ms_byte << 8) | (ls_byte & 0xFF);
}

uint32_t VL53L1XComponent::timeout_mclks_to_us_(uint32_t timeout_mclks, uint32_t macro_period_us) {
  return (static_cast<uint64_t>(timeout_mclks) * macro_period_us + 0x800) >> 12;
}

uint32_t VL53L1XComponent::timeout_us_to_mclks_(uint32_t timeout_us, uint32_t macro_period_us) {
  return ((timeout_us << 12) + (macro_period_us >> 1)) / macro_period_us;
}

uint32_t VL53L1XComponent::calc_macro_period_(uint8_t vcsel_period) {
  // fast osc frequency in 4.12 format; PLL period in 0.24 format
  uint32_t pll_period_us = (static_cast<uint32_t>(1) << 30) / this->fast_osc_frequency_;
  uint8_t vcsel_period_pclks = (vcsel_period + 1) << 1;

  // VL53L1_MACRO_PERIOD_VCSEL_PERIODS = 2304
  uint32_t macro_period_us = static_cast<uint32_t>(2304) * pll_period_us;
  macro_period_us >>= 6;
  macro_period_us *= vcsel_period_pclks;
  macro_period_us >>= 6;
  return macro_period_us;
}

}  // namespace esphome::vl53l1x
