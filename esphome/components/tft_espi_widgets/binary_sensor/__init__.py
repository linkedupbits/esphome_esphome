import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor
from esphome.const import CONF_ID, ICON_EMPTY, UNIT_EMPTY

DEPENDENCIES = ['uart']

empty_uart_sensor_ns = cg.esphome_ns.namespace('tft_espi_widgets')
EmptyUARTSensor = empty_uart_sensor_ns.class_('EmptyUARTSensor', cg.PollingComponent, uart.UARTDevice)

CONFIG_SCHEMA = sensor.sensor_schema(UNIT_EMPTY, ICON_EMPTY, 1).extend({
    cv.GenerateID(): cv.declare_id(EmptyUARTSensor),
}).extend(cv.polling_component_schema('60s')).extend(uart.UART_DEVICE_SCHEMA)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)


"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
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
"""