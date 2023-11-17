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
AUTO_LOAD = ["tft_espi"]

CONF_POSITION = "position"
CONF_X_POS = "x"
CONF_Y_POS = "y"
CONF_WIDTH = "width"
CONF_HEIGHT = "height"

tft_espi_widgets_ns = cg.esphome_ns.namespace("tft_espi_widgets")
TFT_eSPI_Button = tft_espi_widgets_ns.class_("TFT_eSPI_ESPHome_Button", cg.Component,  binary_sensor.BinarySensor)

CONFIG_SCHEMA = cv.All(
    binary_sensor.binary_sensor_schema(TFT_eSPI_Button)
    .extend(cv.COMPONENT_SCHEMA)
    .extend(
        {
            #cv.GenerateID(CONF_ID): cv.declare_id(TFT_eSPI_Button),
            cv.GenerateID(CONF_TFT_ESPI_ID): cv.use_id(TFT_ESPI),
            cv.Required(CONF_POSITION): cv.Schema ( {
                cv.Required(CONF_X_POS): cv.positive_int,
                cv.Required(CONF_Y_POS): cv.positive_int,
                cv.Required(CONF_WIDTH): cv.positive_int,
                cv.Required(CONF_HEIGHT): cv.positive_int,
            }),
        }
    )
)

# https://gite.lirmm.fr/doccy/TFT_eSPI_Widgets

async def to_code(config):
    cg.add_library("TFT_eSPI", None)
    cg.add_library("TFT_eWidget", None)
    
    espi = await cg.get_variable(config[CONF_TFT_ESPI_ID])
    position_conf = config[CONF_POSITION]
    #var = cg.new_Pvariable(config[CONF_ID], espi, position_conf[CONF_X_POS], position_conf[CONF_Y_POS], position_conf[CONF_WIDTH], position_conf[CONF_HEIGHT])
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)
    cg.add(var.configure(espi, position_conf[CONF_X_POS], position_conf[CONF_Y_POS], position_conf[CONF_WIDTH], position_conf[CONF_HEIGHT]))
