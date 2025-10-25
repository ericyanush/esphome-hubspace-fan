"""HubSpace component for ESPHome."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["network"]
AUTO_LOAD = ["sensor"]

hubspace_ns = cg.esphome_ns.namespace("hubspace")
HubSpaceComponent = hubspace_ns.class_("HubSpaceComponent", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(HubSpaceComponent),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    """Generate code for the component."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
