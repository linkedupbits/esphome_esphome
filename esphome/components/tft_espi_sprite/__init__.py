import esphome.codegen as cg
import esphome.config_validation as cv
import logging
from esphome.components import binary_sensor
from esphome.const import (
    CONF_ID,
)

from esphome.components.tft_espi import (
    TFT_ESPI, 
    tft_espi_ns, 
    CONF_TFT_ESPI_ID
)

CODEOWNERS = ["@linkedupbits"]

_LOGGER = logging.getLogger(__name__)
_LOGGER.info("processing %s...", __name__)

DEPENDENCIES = ["tft_espi"]
AUTO_LOAD = ["tft_espi"]

CONF_ESPI_SPRITES = "sprites"
CONF_POSITION = "position"
CONF_X_POS = "x"
CONF_Y_POS = "y"
CONF_WIDTH = "width"
CONF_HEIGHT = "height"

tft_espi_sprite_ns = cg.esphome_ns.namespace("tft_espi_sprite")
TFT_eSprite_ESPHome = tft_espi_sprite_ns.class_("TFT_eSprite_ESPHome", cg.Component)


CONFIG_SCHEMA = cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TFT_eSprite_ESPHome),
            cv.GenerateID(CONF_TFT_ESPI_ID): cv.use_id(TFT_ESPI),
            cv.Required(CONF_ESPI_SPRITES): cv.ensure_list(
                cv.Schema( {
                    cv.Required(CONF_POSITION): cv.Schema ( {
                        cv.Required(CONF_X_POS): cv.positive_int,
                        cv.Required(CONF_Y_POS): cv.positive_int,
                        cv.Required(CONF_WIDTH): cv.positive_int,
                        cv.Required(CONF_HEIGHT): cv.positive_int,
                    }),
                }),
            ),
        }
#    ,validate_tdisplays3,
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    cg.add_library("TFT_eSPI", None)
    
    espi = await cg.get_variable(config[CONF_TFT_ESPI_ID])
    var = cg.new_Pvariable(config[CONF_ID], espi)
    await cg.register_component(var, config)
