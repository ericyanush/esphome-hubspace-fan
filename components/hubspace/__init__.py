"""HubSpace component for ESPHome."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor"]
CONF_HUBSPACE_ID = "hubspace_id"

hubspace_ns = cg.esphome_ns.namespace("hubspace")
HubSpaceComponent = hubspace_ns.class_("HubSpaceComponent", cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(HubSpaceComponent),
    }
).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)


async def to_code(config):
    """Generate code for the component."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
