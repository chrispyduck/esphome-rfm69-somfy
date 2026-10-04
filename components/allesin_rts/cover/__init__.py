import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover
from esphome.const import CONF_ID

from .. import AllesinRTSHub, allesin_rts_ns

CONF_ALLESIN_RTS_ID = "allesin_rts_id"

AllesinRTSCover = allesin_rts_ns.class_("AllesinRTSCover", cover.Cover, cg.Component)

CONFIG_SCHEMA = cover.COVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(AllesinRTSCover),
        cv.GenerateID(CONF_ALLESIN_RTS_ID): cv.use_id(AllesinRTSHub),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cover.register_cover(var, config)

    hub = await cg.get_variable(config[CONF_ALLESIN_RTS_ID])
    cg.add(var.set_hub(hub))
