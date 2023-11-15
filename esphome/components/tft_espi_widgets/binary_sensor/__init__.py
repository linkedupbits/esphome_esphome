import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
)

#from ../tft_espi import (
#    CONF_TFT_ESPI_ID
#)

CODEOWNERS = ["@linkedupbits"]

#DEPENDENCIES = ["tft_espi"]
AUTO_LOAD = ["binary_sensor"]

CONF_TFT_eSPI_ESPHome_Button_ID = "TFT_eSPI_Widget_Button"

tft_espi_widgets_ns = cg.esphome_ns.namespace("tft_espi_widgets")
TFT_eSPI_Button = tft_espi_widgets_ns.class_("TFT_eSPI_ESPHome_Button", binary_sensor.BinarySensor)

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(TFT_eSPI_Button).extend(
    {
        cv.GenerateID(CONF_TFT_eSPI_ESPHome_Button_ID), #: cv.use_id(TFT_ESPI),
    }
)

# https://gite.lirmm.fr/doccy/TFT_eSPI_Widgets

async def to_code(config):
    cg.add_library("TFT_eSPI", None)
    cg.add_library("TFT_eSPI_Widgets", None)
    
    #espi = await cg.get_variable(config[CONF_TFT_ESPI_ID])
    #var = cg.new_Pvariable(config[CONF_ID], espi)
    await cg.register_component(var, config)
