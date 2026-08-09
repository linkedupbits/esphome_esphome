import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_DISABLED_BY_DEFAULT,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_RULER,
)

from .. import VL53L1XComponent

DEPENDENCIES = ["vl53l1x"]

CONF_VL53L1X_ID = "vl53l1x_id"
CONF_ERROR_SENSOR = "error_sensor"

CONFIG_SCHEMA = text_sensor.text_sensor_schema(
    icon=ICON_RULER,
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
).extend(
    {
        cv.GenerateID(CONF_VL53L1X_ID): cv.use_id(VL53L1XComponent),
        # Reports the "Occurrence" text from Table 7 in UM2356 (the VL53L1X API user manual) for the most recent
        # driver error/warning. Only receives updates if the hub's `output_errors_and_warnings` option is enabled
        # -- otherwise the underlying string table isn't compiled in at all, and this stays unknown.
        cv.Optional(CONF_ERROR_SENSOR): text_sensor.text_sensor_schema(
            icon="mdi:alert-circle-outline",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ).extend({cv.Optional(CONF_DISABLED_BY_DEFAULT, default=True): cv.boolean}),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_VL53L1X_ID])

    sens = await text_sensor.new_text_sensor(config)
    cg.add(parent.set_distance_mode_text_sensor(sens))

    if error_sensor_config := config.get(CONF_ERROR_SENSOR):
        sens = await text_sensor.new_text_sensor(error_sensor_config)
        cg.add(parent.set_error_sensor(sens))
