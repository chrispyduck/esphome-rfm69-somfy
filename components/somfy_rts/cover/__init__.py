import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover
from esphome.const import CONF_ID

from .. import SomfyRTSHub, somfy_rts_ns

CONF_SOMFY_RTS_ID = "somfy_rts_id"
CONF_REMOTE_NUMBER = "remote_number"

SomfyRTSCover = somfy_rts_ns.class_("SomfyRTSCover", cover.Cover, cg.Component)

CONFIG_SCHEMA = cover.COVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(SomfyRTSCover),
        cv.GenerateID(CONF_SOMFY_RTS_ID): cv.use_id(SomfyRTSHub),
        cv.Required(CONF_REMOTE_NUMBER): cv.int_range(min=0, max=127),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cover.register_cover(var, config)

    hub = await cg.get_variable(config[CONF_SOMFY_RTS_ID])
    cg.add(var.set_hub(hub))
    cg.add(var.set_remote_number(config[CONF_REMOTE_NUMBER]))
