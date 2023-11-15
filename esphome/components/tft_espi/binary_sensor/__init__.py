import esphome.codegen as cg
import esphome.config_validation as cv
import logging
from esphome.components import binary_sensor
from esphome.const import (
    CONF_ID,
)

from .. import TFT_ESPI, tft_espi_ns, CONF_TFT_ESPI_ID

CODEOWNERS = ["@linkedupbits"]

_LOGGER = logging.getLogger(__name__)
_LOGGER.info("Test %s...", "a String")

DEPENDENCIES = ["tft_espi"]
# AUTO_LOAD = ["binary_sensor"]

CONF_TFT_eSPI_ESPHome_Button_ID = "TFT_eSPI_Widget_Button"

tft_espi_widgets_ns = cg.esphome_ns.namespace("tft_espi_widgets")
TFT_eSPI_Button = tft_espi_widgets_ns.class_("TFT_eSPI_ESPHome_Button", binary_sensor.BinarySensor)

CONFIG_SCHEMA = cv.All(
    binary_sensor.binary_sensor_schema(TFT_eSPI_Button)
    .extend(cv.COMPONENT_SCHEMA)
    .extend(
        {
            cv.GenerateID(CONF_TFT_eSPI_ESPHome_Button_ID): cv.declare_id(TFT_eSPI_Button),
            cv.GenerateID(CONF_TFT_ESPI_ID): cv.use_id(TFT_ESPI),
        }
    )
)

# https://gite.lirmm.fr/doccy/TFT_eSPI_Widgets

async def to_code(config):
    cg.add_library("TFT_eSPI", None)
    cg.add_library("TFT_eSPI_Widgets", None)
    
    espi = await cg.get_variable(config[CONF_TFT_ESPI_ID])
    var = cg.new_Pvariable(config[], espi)
    await cg.register_component(var, config)
