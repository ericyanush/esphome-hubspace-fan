#include "light.h"
#include "esphome/core/log.h"

namespace esphome {
namespace hubspace {

static const char *const TAG = "hubspace.light";

// Color temperature range: 2700K to 6500K
static const float MIN_KELVIN = 2700.0f;
static const float MAX_KELVIN = 6500.0f;

void HubSpaceLight::setup() {
  ESP_LOGCONFIG(TAG, "Setting up HubSpace Light...");
}

void HubSpaceLight::dump_config() {
  ESP_LOGCONFIG(TAG, "HubSpace Light:");
}

light::LightTraits HubSpaceLight::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
  traits.set_min_mireds(esphome::light::color_temperature_kelvin_to_mired(MAX_KELVIN));
  traits.set_max_mireds(esphome::light::color_temperature_kelvin_to_mired(MIN_KELVIN));
  return traits;
}

void HubSpaceLight::write_state(light::LightState *state) {
  if (this->parent_ == nullptr) {
    ESP_LOGE(TAG, "Parent not set!");
    return;
  }

  float brightness, color_temp;
  state->current_values_as_brightness(&brightness);
  state->current_values_as_ct(&color_temp);
  
  // Convert brightness to 0-100 scale
  uint8_t brightness_byte = static_cast<uint8_t>(brightness * 100.0f);
  
  // Convert color temperature (mireds) to kelvin
  float kelvin = esphome::light::color_temperature_mired_to_kelvin(color_temp);
  uint8_t color_code = this->kelvin_to_code(kelvin);
  
  ESP_LOGD(TAG, "Setting brightness to %d%%, color temp to %dK (code: 0x%02X)", 
           brightness_byte, static_cast<int>(kelvin), color_code);
  
  this->parent_->send_brightness(brightness_byte);
  this->parent_->send_color_temp(color_code);
}

uint8_t HubSpaceLight::kelvin_to_code(float kelvin) {
  // Map kelvin to nearest discrete step
  // 2700K = 0x01, 3000K = 0x02, 3500K = 0x03, 4000K = 0x04, 5000K = 0x05, 6500K = 0x06
  if (kelvin <= 2850.0f) return TEMP_2700K;
  if (kelvin <= 3250.0f) return TEMP_3000K;
  if (kelvin <= 3750.0f) return TEMP_3500K;
  if (kelvin <= 4500.0f) return TEMP_4000K;
  if (kelvin <= 5750.0f) return TEMP_5000K;
  return TEMP_6500K;
}

float HubSpaceLight::code_to_kelvin(uint8_t code) {
  switch (code) {
    case TEMP_2700K: return 2700.0f;
    case TEMP_3000K: return 3000.0f;
    case TEMP_3500K: return 3500.0f;
    case TEMP_4000K: return 4000.0f;
    case TEMP_5000K: return 5000.0f;
    case TEMP_6500K: return 6500.0f;
    default: return 3500.0f;  // Default to middle
  }
}

void HubSpaceLight::update_from_slave(uint8_t brightness, uint8_t color_code) {
  auto call = this->make_call();
  
  // Update brightness
  float brightness_float = brightness / 100.0f;
  call.set_brightness(brightness_float);
  
  // Update color temperature
  float kelvin = this->code_to_kelvin(color_code);
  float mireds = esphome::light::color_temperature_kelvin_to_mired(kelvin);
  call.set_color_temperature(mireds);
  
  call.perform();
}

}  // namespace hubspace
}  // namespace esphome
