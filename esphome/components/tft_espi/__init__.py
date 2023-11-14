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
#    ,validate_tdisplays3,
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):

    cg.add_build_flag("-DUSER_SETUP_LOADED=1")
    # todo: Add driver selection to schema
    cg.add_build_flag("-DST7796_DRIVER=1")
    # todo: add dimensions to schema
    cg.add_build_flag("-DTFT_WIDTH=320")
    cg.add_build_flag("-DTFT_HEIGHT=480")
    cg.add_build_flag("-DCONFIG_TFT_MISO=12")
    cg.add_build_flag("-DTFT_MISO=12")
    cg.add_build_flag("-DTFT_MOSI=13")
    cg.add_build_flag("-DTFT_SCLK=14")
    cg.add_build_flag("-DTFT_CS=15")
    cg.add_build_flag("-DTFT_DC=2")
    cg.add_build_flag("-DTFT_RST=-1")
    
    
    cg.add_build_flag("-DLOAD_GLCD=1")
    cg.add_build_flag("-DLOAD_FONT2=1")
    cg.add_build_flag("-DLOAD_FONT4=1")
    cg.add_build_flag("-DLOAD_FONT6=1")
    cg.add_build_flag("-DLOAD_FONT7=1")
    cg.add_build_flag("-DLOAD_FONT8=1")
    cg.add_build_flag("-DLOAD_GFXFF=1")
    cg.add_build_flag("-DSMOOTH_FONT=1")    

    # If you don't care about control of the backlight you can uncomment the two lines below")
    cg.add_build_flag("-DTFT_BL=27")
    cg.add_build_flag("-DTFT_BACKLIGHT_ON=HIGH") # LOW

    cg.add_build_flag("-DSPI_FREQUENCY=65000000")
    cg.add_build_flag("-DSPI_READ_FREQUENCY=20000000")
    cg.add_build_flag("-DSPI_TOUCH_FREQUENCY=2500000")
  
    cg.add_build_flag("-DTOUCH_CS=33")

    cg.add_library("SPI", None)
    cg.add_library("FS", None)
    cg.add_library("SPIFFS", None)
    cg.add_library("TFT_eSPI", None)
    cg.add_library("LittleFS", None)

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(Display.operator("ref"), "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))