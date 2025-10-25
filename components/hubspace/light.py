"""HubSpace Light platform."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_OUTPUT_ID
from . import hubspace_ns, HubSpaceComponent, CONF_HUBSPACE_ID

DEPENDENCIES = ["hubspace"]

HubSpaceLight = hubspace_ns.class_("HubSpaceLight", light.LightOutput, cg.Component)

CONFIG_SCHEMA = light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(HubSpaceLight),
        cv.GenerateID(CONF_HUBSPACE_ID): cv.use_id(HubSpaceComponent),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    """Generate code for the light."""
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)
    
    parent = await cg.get_variable(config[CONF_HUBSPACE_ID])
    cg.add(var.set_parent(parent))
    cg.add(parent.register_light(var))
