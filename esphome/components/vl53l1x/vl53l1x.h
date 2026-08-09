#pragma once

#include <list>

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/optional.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome::vl53l1x {

enum class DistanceMode : uint8_t { SHORT, MEDIUM, LONG };

// Matches the "CrossMode" parameter described in UM2356 section 2.5.5 ("Thresholds").
enum class DistanceThresholdWindow : uint8_t {
  BELOW = 0,           // report when distance < low
  ABOVE = 1,           // report when distance > high
  OUTSIDE_WINDOW = 2,  // report when distance < low or distance > high
  INSIDE_WINDOW = 3,   // report when low < distance < high
};

enum class RangeStatus : uint8_t {
  RANGE_VALID = 0,
  SIGMA_FAIL = 1,
  SIGNAL_FAIL = 2,
  RANGE_VALID_MIN_RANGE_CLIPPED = 3,
  OUT_OF_BOUNDS_FAIL = 4,
  HARDWARE_FAIL = 5,
  RANGE_VALID_NO_WRAP_CHECK_FAIL = 6,
  WRAP_TARGET_FAIL = 7,
  XTALK_SIGNAL_FAIL = 9,
  SYNCHRONIZATION_INT = 10,
  MIN_RANGE_FAIL = 13,
  NONE = 255,
};

class VL53L1XComponent final : public PollingComponent, public i2c::I2CDevice {
 public:
  VL53L1XComponent();

  void setup() override;
  void dump_config() override;
  void update() override;
  void loop() override;

  void set_enable_pin(GPIOPin *enable) { this->enable_pin_ = enable; }

  // These may be called both at configuration time (before setup(), from codegen) and at runtime (e.g. from a
  // lambda): once setup() has completed, they also push the new setting to the sensor and to the corresponding
  // diagnostic sensor, if configured.
  void set_timeout_us(uint32_t timeout_us);
  void set_distance_mode(DistanceMode distance_mode);
  void set_timing_budget(uint32_t timing_budget_us);

  void set_distance_sensor(sensor::Sensor *sens) { this->distance_sensor_ = sens; }
  void set_distance_mode_text_sensor(text_sensor::TextSensor *sens) { this->distance_mode_sensor_ = sens; }
  void set_timing_budget_sensor(sensor::Sensor *sens) { this->timing_budget_sensor_ = sens; }
  void set_timeout_sensor(sensor::Sensor *sens) { this->timeout_sensor_ = sens; }

  // Report the result of calibrate_offset()/calibrate_xtalk() runs. Unlike the diagnostic sensors above, these are
  // never published from setup() with the configured/default value -- they stay unknown until a calibration has
  // actually been run since boot.
  void set_calibrated_offset_sensor(sensor::Sensor *sens) { this->calibrated_offset_sensor_ = sens; }
  void set_calibrated_xtalk_sensor(sensor::Sensor *sens) { this->calibrated_xtalk_sensor_ = sens; }

  // Reports the "Occurrence" text of the most recent driver error/warning, from Table 7 ("Bare driver errors and
  // warnings descriptions") in UM2356. Only ever receives updates if `output_errors_and_warnings` is enabled in
  // config -- the description table is compiled out entirely otherwise, so this stays unknown.
  void set_error_sensor(text_sensor::TextSensor *sens) { this->error_sensor_ = sens; }

  // Fixed calibration values (see calibrate_offset()/calibrate_xtalk() below for how to obtain them). May be
  // called both at configuration time and at runtime; once setup() has completed, they also write straight
  // through to the sensor's registers.
  void set_offset_mm(int16_t offset_mm);
  void set_xtalk_cps(uint16_t xtalk_cps);

  // Region of interest (ROI): narrows the sensor's field of view to a smaller area of the 16x16 SPAD array
  // (UM2356 section 2.5.6), from the minimum 4x4 up to the full 16x16 (the default). May be called both at
  // configuration time and at runtime; once setup() has completed, it also writes straight through to the
  // sensor's registers. Unless an explicit center is set via set_roi_center_spad(), the ROI is centered on the
  // sensor's factory-calibrated optical center -- except for ROIs larger than 10x10, which can't fit around that
  // center without exceeding the array bounds and fall back to the geometric center of the full array (SPAD 199).
  // This mirrors VL53L1X_SetROI() in ST's VL53L1X ULD API.
  void set_roi(uint8_t width, uint8_t height);
  // Advanced: explicitly overrides the ROI center SPAD (0-255) instead of the automatic default described above.
  // Consult ST's SPAD numbering documentation before using this -- the 16x16 array isn't numbered in simple
  // row-major order, so an incorrect value will silently point the ROI at the wrong part of the field of view.
  void set_roi_center_spad(uint8_t center_spad);

  // Limit check settings (UM2356 section 2.5.4): a measurement is only flagged valid if both the signal rate and
  // the range sigma (standard deviation) estimate meet these thresholds -- otherwise RangeStatus is non-zero and
  // the sensor entity publishes NAN. May be called both at configuration time and at runtime; once setup() has
  // completed, they also write straight through to the sensor's registers. Defaults (1.5 Mcps / 90mm) match the
  // sensor's own tuning-parm defaults for low power autonomous mode.
  void set_signal_threshold_mcps(float mcps);
  void set_sigma_threshold_mm(uint16_t mm);

  // Distance threshold detection (UM2356 section 2.5.5): configures the sensor to only report a measurement when
  // it matches the given window relative to low_mm/high_mm (see DistanceThresholdWindow above), offloading
  // presence/proximity filtering to hardware. report_no_target additionally reports when no target was found at
  // all (normally, no report is made in that case). May be called both at configuration time and at runtime; once
  // setup() has completed, it also writes straight through to the sensor's registers. Mirrors
  // VL53L1X_SetDistanceThreshold() in ST's VL53L1X ULD API.
  //
  // Note: while a threshold is configured, an update() cycle whose measurement doesn't match the window will
  // never signal "ready" -- loop() gives up and reports NAN once the configured update_interval has nearly
  // elapsed, rather than blocking indefinitely.
  void set_distance_threshold(uint16_t low_mm, uint16_t high_mm, DistanceThresholdWindow window, bool report_no_target);

  // Blocking offset/crosstalk calibration procedures, per ST's UM2356 VL53L1X API user manual (mirroring
  // VL53L1X_CalibrateOffset()/VL53L1X_CalibrateXtalk() in ST's VL53L1X ULD API). Place a target at a known,
  // precisely-measured distance before calling: a plain white 88%-reflectance target at ~140mm for offset
  // calibration, and a grey ~17-18%-reflectance target beyond 600mm (no other object within 300mm of the sensor's
  // field of view) for crosstalk calibration. Each call blocks for up to 50 measurement cycles. On success, the
  // computed value is applied immediately and logged so it can be copied into the `offset_mm` / `xtalk_cps`
  // config options for permanent use without recalibrating on every boot.
  bool calibrate_offset(uint16_t target_distance_mm);
  bool calibrate_xtalk(uint16_t target_distance_mm);

 protected:
  // Register addresses used by this driver (subset of the VL53L1X register map from ST's UM2356 / STSW-IMG007 API).
  static constexpr uint16_t REG_SOFT_RESET = 0x0000;
  static constexpr uint16_t REG_I2C_DEVICE_ADDRESS = 0x0001;
  static constexpr uint16_t REG_ALGO_CROSSTALK_COMPENSATION_PLANE_OFFSET_KCPS = 0x0016;
  static constexpr uint16_t REG_ALGO_PART_TO_PART_RANGE_OFFSET_MM = 0x001E;
  static constexpr uint16_t REG_MM_CONFIG_INNER_OFFSET_MM = 0x0020;
  static constexpr uint16_t REG_MM_CONFIG_OUTER_OFFSET_MM = 0x0022;
  static constexpr uint16_t REG_DSS_CONFIG_TARGET_TOTAL_RATE_MCPS = 0x0024;
  static constexpr uint16_t REG_PAD_I2C_HV_EXTSUP_CONFIG = 0x002E;
  static constexpr uint16_t REG_GPIO_TIO_HV_STATUS = 0x0031;
  static constexpr uint16_t REG_SIGMA_ESTIMATOR_EFFECTIVE_PULSE_WIDTH_NS = 0x0036;
  static constexpr uint16_t REG_SIGMA_ESTIMATOR_EFFECTIVE_AMBIENT_WIDTH_NS = 0x0037;
  static constexpr uint16_t REG_ALGO_CROSSTALK_COMPENSATION_VALID_HEIGHT_MM = 0x0039;
  static constexpr uint16_t REG_ALGO_RANGE_IGNORE_VALID_HEIGHT_MM = 0x003E;
  static constexpr uint16_t REG_ALGO_RANGE_MIN_CLIP = 0x003F;
  static constexpr uint16_t REG_ALGO_CONSISTENCY_CHECK_TOLERANCE = 0x0040;
  static constexpr uint16_t REG_SYSTEM_INTERRUPT_CONFIG_GPIO = 0x0046;
  static constexpr uint16_t REG_CAL_CONFIG_VCSEL_START = 0x0047;
  static constexpr uint16_t REG_PHASECAL_CONFIG_TIMEOUT_MACROP = 0x004B;
  static constexpr uint16_t REG_PHASECAL_CONFIG_OVERRIDE = 0x004D;
  static constexpr uint16_t REG_DSS_CONFIG_ROI_MODE_CONTROL = 0x004F;
  static constexpr uint16_t REG_SYSTEM_THRESH_RATE_HIGH = 0x0050;
  static constexpr uint16_t REG_SYSTEM_THRESH_RATE_LOW = 0x0052;
  static constexpr uint16_t REG_DSS_CONFIG_MANUAL_EFFECTIVE_SPADS_SELECT = 0x0054;
  static constexpr uint16_t REG_DSS_CONFIG_APERTURE_ATTENUATION = 0x0057;
  static constexpr uint16_t REG_MM_CONFIG_TIMEOUT_MACROP_A = 0x005A;
  static constexpr uint16_t REG_MM_CONFIG_TIMEOUT_MACROP_B = 0x005C;
  static constexpr uint16_t REG_RANGE_CONFIG_TIMEOUT_MACROP_A = 0x005E;
  static constexpr uint16_t REG_RANGE_CONFIG_VCSEL_PERIOD_A = 0x0060;
  static constexpr uint16_t REG_RANGE_CONFIG_TIMEOUT_MACROP_B = 0x0061;
  static constexpr uint16_t REG_RANGE_CONFIG_VCSEL_PERIOD_B = 0x0063;
  static constexpr uint16_t REG_RANGE_CONFIG_SIGMA_THRESH = 0x0064;
  static constexpr uint16_t REG_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT_MCPS = 0x0066;
  static constexpr uint16_t REG_RANGE_CONFIG_VALID_PHASE_HIGH = 0x0069;
  static constexpr uint16_t REG_SYSTEM_GROUPED_PARAMETER_HOLD_0 = 0x0071;
  static constexpr uint16_t REG_SYSTEM_THRESH_HIGH = 0x0072;
  static constexpr uint16_t REG_SYSTEM_THRESH_LOW = 0x0074;
  static constexpr uint16_t REG_SYSTEM_SEED_CONFIG = 0x0077;
  static constexpr uint16_t REG_SD_CONFIG_WOI_SD0 = 0x0078;
  static constexpr uint16_t REG_SD_CONFIG_WOI_SD1 = 0x0079;
  static constexpr uint16_t REG_SD_CONFIG_INITIAL_PHASE_SD0 = 0x007A;
  static constexpr uint16_t REG_SD_CONFIG_INITIAL_PHASE_SD1 = 0x007B;
  static constexpr uint16_t REG_SYSTEM_GROUPED_PARAMETER_HOLD_1 = 0x007C;
  static constexpr uint16_t REG_SD_CONFIG_QUANTIFIER = 0x007E;
  static constexpr uint16_t REG_ROI_CONFIG_USER_ROI_CENTRE_SPAD = 0x007F;
  static constexpr uint16_t REG_ROI_CONFIG_USER_ROI_REQUESTED_GLOBAL_XY_SIZE = 0x0080;
  static constexpr uint16_t REG_SYSTEM_SEQUENCE_CONFIG = 0x0081;
  static constexpr uint16_t REG_SYSTEM_GROUPED_PARAMETER_HOLD = 0x0082;
  static constexpr uint16_t REG_SYSTEM_INTERRUPT_CLEAR = 0x0086;
  static constexpr uint16_t REG_SYSTEM_MODE_START = 0x0087;
  static constexpr uint16_t REG_RESULT_RANGE_STATUS = 0x0089;
  static constexpr uint16_t REG_VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND = 0x0008;
  static constexpr uint16_t REG_VHV_CONFIG_INIT = 0x000B;
  static constexpr uint16_t REG_RESULT_OSC_CALIBRATE_VAL = 0x00DE;
  static constexpr uint16_t REG_PHASECAL_RESULT_VCSEL_START = 0x00D8;
  static constexpr uint16_t REG_FIRMWARE_SYSTEM_STATUS = 0x00E5;
  static constexpr uint16_t REG_IDENTIFICATION_MODEL_ID = 0x010F;
  static constexpr uint16_t REG_OSC_MEASURED_FAST_OSC_FREQUENCY = 0x0006;
  static constexpr uint16_t REG_ROI_CONFIG_MODE_ROI_CENTRE_SPAD = 0x013E;

  static constexpr uint16_t MODEL_ID = 0xEACC;
  // See VL53L1X.h in Pololu's vl53l1x-arduino library for the derivation of this value.
  static constexpr uint32_t TIMING_GUARD_US = 4528;
  static constexpr uint16_t TARGET_RATE = 0x0A00;

  // Number of samples averaged by calibrate_offset()/calibrate_xtalk(), matching ST's VL53L1X_CalibrateOffset()/
  // VL53L1X_CalibrateXtalk() reference implementation.
  static constexpr uint8_t CALIBRATION_SAMPLES = 50;

  void set_distance_mode_(DistanceMode mode);
  uint32_t get_measurement_timing_budget_();
  bool set_measurement_timing_budget_(uint32_t budget_us);
  void apply_roi_();
  void apply_distance_threshold_();
  void setup_manual_calibration_();
  void update_dss_(uint16_t spad_count, uint16_t ambient_count_rate_mcps, uint16_t peak_signal_count_rate_mcps);
  static RangeStatus convert_range_status_(uint8_t raw_status, uint8_t stream_count);
  static const char *distance_mode_to_string_(DistanceMode mode);
  static const char *distance_threshold_window_to_string_(DistanceThresholdWindow window);

  void publish_distance_mode_();
  void publish_timing_budget_();
  void publish_timeout_();

  // Looks up error_code in Table 7 of UM2356 ("Bare driver errors and warnings descriptions") and publishes its
  // "Occurrence" text to error_sensor_, if configured. A no-op unless `output_errors_and_warnings` is enabled in
  // config -- see vl53l1x.cpp, where the description table itself is compiled out when
  // VL53L1X_EXCLUDE_OUTPUT_ERRORS_AND_WARNINGS is defined (the default -- see __init__.py).
  void report_error_(int8_t error_code);

  // Blocking wait for the next continuous-mode measurement (used only by calibrate_offset()/calibrate_xtalk()),
  // returning the raw (uncorrected) range, crosstalk-corrected signal rate, and effective SPAD count, matching
  // VL53L1X_GetDistance()/VL53L1X_GetSignalRate()/VL53L1X_GetSpadNb() in ST's VL53L1X ULD API. Returns false if no
  // measurement became ready before timing out.
  bool read_calibration_sample_(uint16_t &raw_distance_mm, uint16_t &signal_rate_cps, uint16_t &spad_count);

  uint32_t calc_macro_period_(uint8_t vcsel_period);
  static uint16_t decode_timeout_(uint16_t reg_val);
  static uint16_t encode_timeout_(uint32_t timeout_mclks);
  static uint32_t timeout_mclks_to_us_(uint32_t timeout_mclks, uint32_t macro_period_us);
  static uint32_t timeout_us_to_mclks_(uint32_t timeout_us, uint32_t macro_period_us);

  void write_reg_(uint16_t reg, uint8_t value) const { this->write_register16(reg, &value, 1); }
  void write_reg16_(uint16_t reg, uint16_t value) const {
    uint8_t buf[2] = {static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value)};
    this->write_register16(reg, buf, 2);
  }
  uint8_t read_reg_(uint16_t reg) {
    uint8_t value = 0;
    this->read_register16(reg, &value, 1);
    return value;
  }
  uint16_t read_reg16_(uint16_t reg) {
    uint8_t buf[2] = {0, 0};
    this->read_register16(reg, buf, 2);
    return (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
  }

  GPIOPin *enable_pin_{nullptr};
  uint32_t timeout_us_{};
  uint32_t measurement_timing_budget_us_{50000};
  DistanceMode distance_mode_{DistanceMode::LONG};
  int16_t offset_mm_{0};
  uint16_t xtalk_cps_{0};
  uint8_t roi_width_{16};
  uint8_t roi_height_{16};
  optional<uint8_t> roi_center_spad_{};
  uint16_t signal_threshold_raw_{192};  // 9.7 fixed-point Mcps; 192 = 1.5 Mcps
  uint16_t sigma_threshold_mm_{90};
  bool distance_threshold_enabled_{false};
  uint16_t distance_threshold_low_mm_{0};
  uint16_t distance_threshold_high_mm_{0};
  DistanceThresholdWindow distance_threshold_window_{DistanceThresholdWindow::BELOW};
  bool distance_threshold_report_no_target_{false};

  uint16_t fast_osc_frequency_{0};
  bool reading_{false};
  uint32_t reading_start_us_{0};
  bool calibrated_{false};
  bool setup_complete_{false};
  uint8_t saved_vhv_init_{0};
  uint8_t saved_vhv_timeout_{0};

  sensor::Sensor *distance_sensor_{nullptr};
  text_sensor::TextSensor *distance_mode_sensor_{nullptr};
  sensor::Sensor *timing_budget_sensor_{nullptr};
  sensor::Sensor *timeout_sensor_{nullptr};
  sensor::Sensor *calibrated_offset_sensor_{nullptr};
  sensor::Sensor *calibrated_xtalk_sensor_{nullptr};
  text_sensor::TextSensor *error_sensor_{nullptr};

  static std::list<VL53L1XComponent *> vl53_sensors;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
  static bool enable_pin_setup_complete;              // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
};

template<typename... Ts> class CalibrateOffsetAction : public Action<Ts...>, public Parented<VL53L1XComponent> {
 public:
  TEMPLATABLE_VALUE(uint16_t, target_distance_mm)
  void play(const Ts &...x) override { this->parent_->calibrate_offset(this->target_distance_mm_.value(x...)); }
};

template<typename... Ts> class CalibrateXtalkAction : public Action<Ts...>, public Parented<VL53L1XComponent> {
 public:
  TEMPLATABLE_VALUE(uint16_t, target_distance_mm)
  void play(const Ts &...x) override { this->parent_->calibrate_xtalk(this->target_distance_mm_.value(x...)); }
};

}  // namespace esphome::vl53l1x
