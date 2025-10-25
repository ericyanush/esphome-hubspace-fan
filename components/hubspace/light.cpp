#include "light.h"
#include "esphome/core/log.h"

namespace esphome {
namespace hubspace {

static const char *const TAG = "hubspace.light";

void HubSpaceLight::setup() {
  ESP_LOGCONFIG(TAG, "Setting up HubSpace Light...");
}

void HubSpaceLight::dump_config() {
  ESP_LOGCONFIG(TAG, "HubSpace Light:");
}

light::LightTraits HubSpaceLight::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
  return traits;
}

void HubSpaceLight::write_state(light::LightState *state) {
  float brightness;
  state->current_values_as_brightness(&brightness);
  
  ESP_LOGD(TAG, "Setting brightness to %.2f", brightness);
  // TODO: Send brightness command to device
}

}  // namespace hubspace
}  // namespace esphome
