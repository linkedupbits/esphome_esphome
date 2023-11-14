import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
)
from esphome import pins
from esphome.core.entity_helpers import inherit_property_from

CODEOWNERS = ["@linkedupbits"]

from esphome import pins
from esphome.const import (
    CONF_CLK_PIN,
    CONF_ID,
    CONF_MISO_PIN,
    CONF_MOSI_PIN,
    CONF_SPI_ID,
    CONF_CS_PIN,
    CONF_NUMBER,
    CONF_INVERTED,
    KEY_CORE,
    KEY_TARGET_PLATFORM,
    KEY_VARIANT,
    CONF_DATA_RATE,
)

tft_espi_ns = cg.esphome_ns.namespace("tft_espi")
TFT_ESPI = tft_espi_ns.class_("TFT_eSPI",
 cg.PollingComponent
)
TFT_ESPIRef = TFT_ESPI.operator("ref")

def validate_tft_espi(config):
  """
  todo: add meaniful validation
  """
   return config
   
"""
define the schema 
""" 
CONFIG_SCHEMA = cv.Schema(
        {
            cv.Required(CONF_MOSI_PIN): cv.int_,
            cv.Required(CONF_MISO_PIN): cv.int_,
        }
    ,
    validate_tdisplays3,
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
 
    cg.add_library("SPI", None)
    cg.add_library("FS", None)
    cg.add_library("SPIFFS", None)
    cg.add_library("TFT_eSPI", None)

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(Display.operator("ref"), "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))