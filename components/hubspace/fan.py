"""HubSpace Fan platform."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan
from esphome.const import CONF_OUTPUT_ID
from . import hubspace_ns, HubSpaceComponent

DEPENDENCIES = ["hubspace"]

HubSpaceFan = hubspace_ns.class_("HubSpaceFan", fan.Fan, cg.Component)

CONFIG_SCHEMA = fan.FAN_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(HubSpaceFan),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    """Generate code for the fan."""
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await fan.register_fan(var, config)
