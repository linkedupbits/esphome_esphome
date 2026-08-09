from esphome import automation, pins
import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import (
    CONF_ADDRESS,
    CONF_ENABLE_PIN,
    CONF_HIGH,
    CONF_ID,
    CONF_LOW,
    CONF_TIMEOUT,
)

CODEOWNERS = ["@linkedupbits"]
DEPENDENCIES = ["i2c"]
MULTI_CONF = True

vl53l1x_ns = cg.esphome_ns.namespace("vl53l1x")
VL53L1XComponent = vl53l1x_ns.class_(
    "VL53L1XComponent", cg.PollingComponent, i2c.I2CDevice
)
CalibrateOffsetAction = vl53l1x_ns.class_("CalibrateOffsetAction", automation.Action)
CalibrateXtalkAction = vl53l1x_ns.class_("CalibrateXtalkAction", automation.Action)

DistanceMode = vl53l1x_ns.enum("DistanceMode", is_class=True)
DISTANCE_MODES = {
    "SHORT": DistanceMode.SHORT,
    "MEDIUM": DistanceMode.MEDIUM,
    "LONG": DistanceMode.LONG,
}

DistanceThresholdWindow = vl53l1x_ns.enum("DistanceThresholdWindow", is_class=True)
DISTANCE_THRESHOLD_WINDOWS = {
    "BELOW": DistanceThresholdWindow.BELOW,
    "ABOVE": DistanceThresholdWindow.ABOVE,
    "OUTSIDE_WINDOW": DistanceThresholdWindow.OUTSIDE_WINDOW,
    "INSIDE_WINDOW": DistanceThresholdWindow.INSIDE_WINDOW,
}

CONF_DISTANCE_MODE = "distance_mode"
CONF_TIMING_BUDGET = "timing_budget"
CONF_OFFSET_MM = "offset_mm"
CONF_XTALK_CPS = "xtalk_cps"
CONF_TARGET_DISTANCE_MM = "target_distance_mm"
CONF_OUTPUT_ERRORS_AND_WARNINGS = "output_errors_and_warnings"
CONF_ROI_WIDTH = "roi_width"
CONF_ROI_HEIGHT = "roi_height"
CONF_ROI_CENTER_SPAD = "roi_center_spad"
CONF_SIGNAL_THRESHOLD = "signal_threshold"
CONF_SIGMA_THRESHOLD = "sigma_threshold"
CONF_DISTANCE_THRESHOLD = "distance_threshold"
CONF_WINDOW = "window"
CONF_REPORT_NO_TARGET = "report_no_target"


def check_keys(obj):
    if obj[CONF_ADDRESS] != 0x29 and CONF_ENABLE_PIN not in obj:
        msg = "Address other then 0x29 requires enable_pin definition to allow sensor\r"
        msg += "re-addressing. Also if you have more then one VL53 device on the same\r"
        msg += "i2c bus, then all VL53 devices must have enable_pin defined."
        raise cv.Invalid(msg)
    return obj


def check_timeout(value):
    value = cv.positive_time_period_microseconds(value)
    if value.total_seconds > 60:
        raise cv.Invalid("Maximum timeout can not be greater then 60 seconds")
    return value


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(VL53L1XComponent),
            cv.Optional(CONF_ENABLE_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_TIMEOUT, default="0us"): check_timeout,
            cv.Optional(CONF_DISTANCE_MODE, default="LONG"): cv.enum(
                DISTANCE_MODES, upper=True
            ),
            cv.Optional(CONF_TIMING_BUDGET, default="50ms"): cv.All(
                cv.positive_time_period_microseconds,
                cv.Range(
                    min=cv.TimePeriod(microseconds=20000),
                    max=cv.TimePeriod(microseconds=1000000),
                ),
            ),
            # Fixed calibration values obtained from a prior vl53l1x.calibrate_offset / vl53l1x.calibrate_xtalk
            # run (see UM2356, the VL53L1X API user manual). Applied on every boot so the sensor doesn't need to
            # be recalibrated each time.
            cv.Optional(CONF_OFFSET_MM, default=0): cv.int_range(min=-4000, max=4000),
            cv.Optional(CONF_XTALK_CPS, default=0): cv.int_range(min=0, max=65535),
            # Compiles in the driver error/warning description table (see Table 7, "Bare driver errors and
            # warnings descriptions", in UM2356) so it can be reported via the `error_sensor` text sensor. Left
            # disabled by default since the table costs a small amount of flash that most users don't need.
            cv.Optional(CONF_OUTPUT_ERRORS_AND_WARNINGS, default=False): cv.boolean,
            # Region of interest (ROI): narrows the field of view to a smaller area of the 16x16 SPAD array (see
            # UM2356 section 2.5.6), from the minimum 4x4 up to the full 16x16 (the default, i.e. no narrowing).
            cv.Optional(CONF_ROI_WIDTH, default=16): cv.int_range(min=4, max=16),
            cv.Optional(CONF_ROI_HEIGHT, default=16): cv.int_range(min=4, max=16),
            # Advanced: explicitly overrides the ROI center SPAD instead of the automatic default (the sensor's
            # factory-calibrated optical center, or the geometric center of the array for ROIs larger than 10x10).
            # Consult ST's SPAD numbering documentation before setting this.
            cv.Optional(CONF_ROI_CENTER_SPAD): cv.int_range(min=0, max=255),
            # Limit check settings (UM2356 section 2.5.4): a measurement is only considered valid if it meets both
            # of these. Defaults match the sensor's own tuning-parm defaults for low power autonomous mode.
            cv.Optional(CONF_SIGNAL_THRESHOLD, default=1.5): cv.float_range(
                min=0.0, max=511.0
            ),
            cv.Optional(CONF_SIGMA_THRESHOLD, default=90): cv.int_range(
                min=0, max=16383
            ),
            # Distance threshold detection (UM2356 section 2.5.5): configures the sensor to only report a
            # measurement matching this window, offloading presence/proximity filtering to hardware. Disabled
            # (standard ranging) unless this block is present.
            cv.Optional(CONF_DISTANCE_THRESHOLD): cv.Schema(
                {
                    cv.Required(CONF_LOW): cv.int_range(min=0, max=65535),
                    cv.Required(CONF_HIGH): cv.int_range(min=0, max=65535),
                    cv.Optional(CONF_WINDOW, default="INSIDE_WINDOW"): cv.enum(
                        DISTANCE_THRESHOLD_WINDOWS, upper=True
                    ),
                    cv.Optional(CONF_REPORT_NO_TARGET, default=False): cv.boolean,
                }
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x29)),
    check_keys,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_timeout_us(config[CONF_TIMEOUT]))
    cg.add(var.set_distance_mode(config[CONF_DISTANCE_MODE]))
    cg.add(var.set_timing_budget(config[CONF_TIMING_BUDGET]))
    cg.add(var.set_offset_mm(config[CONF_OFFSET_MM]))
    cg.add(var.set_xtalk_cps(config[CONF_XTALK_CPS]))
    cg.add(var.set_roi(config[CONF_ROI_WIDTH], config[CONF_ROI_HEIGHT]))

    if (roi_center_spad := config.get(CONF_ROI_CENTER_SPAD)) is not None:
        cg.add(var.set_roi_center_spad(roi_center_spad))

    cg.add(var.set_signal_threshold_mcps(config[CONF_SIGNAL_THRESHOLD]))
    cg.add(var.set_sigma_threshold_mm(config[CONF_SIGMA_THRESHOLD]))

    if distance_threshold_config := config.get(CONF_DISTANCE_THRESHOLD):
        cg.add(
            var.set_distance_threshold(
                distance_threshold_config[CONF_LOW],
                distance_threshold_config[CONF_HIGH],
                distance_threshold_config[CONF_WINDOW],
                distance_threshold_config[CONF_REPORT_NO_TARGET],
            )
        )

    # Inverted (exclude, not include) so the code is present by default -- and therefore covered by clang-tidy and
    # other static analysis -- unless a build specifically opts out by leaving `output_errors_and_warnings` unset.
    if not config[CONF_OUTPUT_ERRORS_AND_WARNINGS]:
        cg.add_define("VL53L1X_EXCLUDE_OUTPUT_ERRORS_AND_WARNINGS")

    if enable_pin_config := config.get(CONF_ENABLE_PIN):
        enable_pin = await cg.gpio_pin_expression(enable_pin_config)
        cg.add(var.set_enable_pin(enable_pin))


VL53L1X_CALIBRATE_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(VL53L1XComponent),
        cv.Required(CONF_TARGET_DISTANCE_MM): cv.templatable(
            cv.int_range(min=1, max=4000)
        ),
    }
)


@automation.register_action(
    "vl53l1x.calibrate_offset",
    CalibrateOffsetAction,
    VL53L1X_CALIBRATE_SCHEMA,
    synchronous=True,
)
@automation.register_action(
    "vl53l1x.calibrate_xtalk",
    CalibrateXtalkAction,
    VL53L1X_CALIBRATE_SCHEMA,
    synchronous=True,
)
async def vl53l1x_calibrate_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    target_distance_mm = await cg.templatable(
        config[CONF_TARGET_DISTANCE_MM], args, cg.uint16
    )
    cg.add(var.set_target_distance_mm(target_distance_mm))
    return var
