import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_DISABLED_BY_DEFAULT,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_ARROW_EXPAND_VERTICAL,
    ICON_RULER,
    ICON_TIMER,
    STATE_CLASS_MEASUREMENT,
    UNIT_METER,
    UNIT_MILLIMETER,
    UNIT_MILLISECOND,
)

from .. import VL53L1XComponent

DEPENDENCIES = ["vl53l1x"]

CONF_VL53L1X_ID = "vl53l1x_id"
CONF_TIMING_BUDGET_SENSOR = "timing_budget_sensor"
CONF_TIMEOUT_SENSOR = "timeout_sensor"
CONF_CALIBRATED_OFFSET_SENSOR = "calibrated_offset_sensor"
CONF_CALIBRATED_XTALK_SENSOR = "calibrated_xtalk_sensor"

CONFIG_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_METER,
    icon=ICON_ARROW_EXPAND_VERTICAL,
    accuracy_decimals=2,
    state_class=STATE_CLASS_MEASUREMENT,
).extend(
    {
        cv.GenerateID(CONF_VL53L1X_ID): cv.use_id(VL53L1XComponent),
        cv.Optional(CONF_TIMING_BUDGET_SENSOR): sensor.sensor_schema(
            unit_of_measurement=UNIT_MILLISECOND,
            icon=ICON_TIMER,
            accuracy_decimals=3,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_TIMEOUT_SENSOR): sensor.sensor_schema(
            unit_of_measurement=UNIT_MILLISECOND,
            icon=ICON_TIMER,
            accuracy_decimals=3,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        # Report the result of the last calibrate_offset()/calibrate_xtalk() run (see the vl53l1x.calibrate_offset
        # / vl53l1x.calibrate_xtalk actions). Disabled by default, and left unknown until calibration is invoked.
        cv.Optional(CONF_CALIBRATED_OFFSET_SENSOR): sensor.sensor_schema(
            unit_of_measurement=UNIT_MILLIMETER,
            icon=ICON_RULER,
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ).extend({cv.Optional(CONF_DISABLED_BY_DEFAULT, default=True): cv.boolean}),
        cv.Optional(CONF_CALIBRATED_XTALK_SENSOR): sensor.sensor_schema(
            icon=ICON_RULER,
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ).extend({cv.Optional(CONF_DISABLED_BY_DEFAULT, default=True): cv.boolean}),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_VL53L1X_ID])

    sens = await sensor.new_sensor(config)
    cg.add(parent.set_distance_sensor(sens))

    if timing_budget_sensor_config := config.get(CONF_TIMING_BUDGET_SENSOR):
        sens = await sensor.new_sensor(timing_budget_sensor_config)
        cg.add(parent.set_timing_budget_sensor(sens))

    if timeout_sensor_config := config.get(CONF_TIMEOUT_SENSOR):
        sens = await sensor.new_sensor(timeout_sensor_config)
        cg.add(parent.set_timeout_sensor(sens))

    if calibrated_offset_sensor_config := config.get(CONF_CALIBRATED_OFFSET_SENSOR):
        sens = await sensor.new_sensor(calibrated_offset_sensor_config)
        cg.add(parent.set_calibrated_offset_sensor(sens))

    if calibrated_xtalk_sensor_config := config.get(CONF_CALIBRATED_XTALK_SENSOR):
        sens = await sensor.new_sensor(calibrated_xtalk_sensor_config)
        cg.add(parent.set_calibrated_xtalk_sensor(sens))
