import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover
from esphome.const import CONF_ID

from .. import AllesinRTSHub, allesin_rts_ns

CONF_ALLESIN_RTS_ID = "allesin_rts_id"

AllesinRTSCover = allesin_rts_ns.class_("AllesinRTSCover", cover.Cover, cg.Component)

# See the identical comment in components/somfy_rts/cover/__init__.py: ESPHome's
# cover-platform schema helper has changed shape more than once across the
# versions this component has actually been run against - support both instead of
# betting on whichever one happens to be installed.
if hasattr(cover, "cover_schema"):
    _BASE_SCHEMA = cover.cover_schema(AllesinRTSCover)
else:
    _BASE_SCHEMA = cover.COVER_SCHEMA.extend({cv.GenerateID(): cv.declare_id(AllesinRTSCover)})

CONFIG_SCHEMA = _BASE_SCHEMA.extend(
    {
        cv.GenerateID(CONF_ALLESIN_RTS_ID): cv.use_id(AllesinRTSHub),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cover.register_cover(var, config)

    hub = await cg.get_variable(config[CONF_ALLESIN_RTS_ID])
    cg.add(var.set_hub(hub))
