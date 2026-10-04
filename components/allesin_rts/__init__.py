import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

from ..rfm69_direct import RFM69DirectHub

CODEOWNERS = ["@chris"]
AUTO_LOAD = ["rfm69_direct"]

CONF_RFM69_DIRECT_ID = "rfm69_direct_id"

allesin_rts_ns = cg.esphome_ns.namespace("allesin_rts")
AllesinRTSHub = allesin_rts_ns.class_("AllesinRTSHub", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(AllesinRTSHub),
        cv.GenerateID(CONF_RFM69_DIRECT_ID): cv.use_id(RFM69DirectHub),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    radio_hub = await cg.get_variable(config[CONF_RFM69_DIRECT_ID])
    cg.add(var.set_radio_hub(radio_hub))
