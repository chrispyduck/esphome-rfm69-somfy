import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@chris"]

rfm69_direct_ns = cg.esphome_ns.namespace("rfm69_direct")
RFM69DirectHub = rfm69_direct_ns.class_("RFM69DirectHub", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(RFM69DirectHub),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    # The vendored RFM69 driver needs Arduino's SPI library directly (see
    # components/somfy_rts/__init__.py's identical call for why add_library() is
    # needed instead of just #include <SPI.h>).
    cg.add_library("SPI", None)

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
