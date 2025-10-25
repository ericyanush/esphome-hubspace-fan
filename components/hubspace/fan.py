"""HubSpace Fan platform."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan
from esphome.const import CONF_OUTPUT_ID, CONF_ID
from . import hubspace_ns, HubSpaceComponent, CONF_HUBSPACE_ID

DEPENDENCIES = ["hubspace"]
CONF_HUBSPACE_ID = "hubspace_id"

HubSpaceFan = hubspace_ns.class_("HubSpaceFan", fan.Fan, cg.Component)

CONFIG_SCHEMA = fan.fan_schema(HubSpaceFan).extend(
    {
        cv.GenerateID(CONF_HUBSPACE_ID): cv.use_id(HubSpaceComponent),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    """Generate code for the fan."""
    var = await fan.new_fan(config)
    await cg.register_component(var, config)
    
    parent = await cg.get_variable(config[CONF_HUBSPACE_ID])
    cg.add(var.set_parent(parent))
    cg.add(parent.register_fan(var))
