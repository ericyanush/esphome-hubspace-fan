#include "light.h"
#include "esphome/core/log.h"

namespace esphome {
namespace hubspace {

static const char *const TAG = "hubspace.light";

// Color temperature range: 2700K to 6500K
static const float MIN_KELVIN = 2700.0f;
static const float MAX_KELVIN = 6500.0f;
static const float MIRED_CONVERSION = 1000000.0f;

void HubSpaceLight::setup() {
  ESP_LOGCONFIG(TAG, "Setting up HubSpace Light...");
}

void HubSpaceLight::dump_config() {
  ESP_LOGCONFIG(TAG, "HubSpace Light:");
}

light::LightTraits HubSpaceLight::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
  traits.set_min_mireds(MIRED_CONVERSION / MAX_KELVIN);
  traits.set_max_mireds(MIRED_CONVERSION / MIN_KELVIN);
  return traits;
}

void HubSpaceLight::setup_state(light::LightState *state) {
  state_ = state;
}

void HubSpaceLight::write_state(light::LightState *state) {
  if (this->parent_ == nullptr) {
    ESP_LOGE(TAG, "Parent not set!");
    return;
  }
  
  float brightness = state->current_values.get_brightness();
  bool is_on = state->current_values.is_on();
  float kelvin = state->current_values.get_color_temperature_kelvin();
  
  // Convert brightness to 0-100 scale
  uint8_t brightness_byte = static_cast<uint8_t>(brightness * 100.0f);
  
  ColorTemp color_code = this->kelvin_to_code(kelvin);
  
  ESP_LOGD(TAG, "Setting brightness to %d%%, color temp to %dK (code: 0x%02X)", 
           brightness_byte, static_cast<int>(kelvin), color_code);
  
  // Track expected changes to prevent update loops
  this->pending_change_.has_brightness_change = true;
  this->pending_change_.expected_brightness = is_on ? brightness_byte : 0;
  this->pending_change_.has_color_change = true;
  this->pending_change_.expected_color_temp = static_cast<ColorTemp>(color_code);
  
  this->parent_->send_brightness(is_on ? brightness_byte : 0);
  this->parent_->send_color_temp(color_code);
}

ColorTemp HubSpaceLight::kelvin_to_code(float kelvin) {
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

void HubSpaceLight::update_from_slave(LightStatus status) {
  // Check if this update matches our pending changes
  bool brightness_matches = !this->pending_change_.has_brightness_change || 
                           (status.brightness == this->pending_change_.expected_brightness);
  bool color_matches = !this->pending_change_.has_color_change || 
                      (status.color_temp == this->pending_change_.expected_color_temp);
  
  // Clear pending changes if slave has caught up
  if (brightness_matches && this->pending_change_.has_brightness_change) {
    ESP_LOGV(TAG, "Slave confirmed brightness change: %d%%", status.brightness);
    this->pending_change_.has_brightness_change = false;
  }
  
  if (color_matches && this->pending_change_.has_color_change) {
    ESP_LOGV(TAG, "Slave confirmed color temperature change: 0x%02X", status.color_temp);
    this->pending_change_.has_color_change = false;
  }
  
  // Only process updates if no local changes are pending or if the update doesn't conflict
  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGV(TAG, "Ignoring update from slave while local change is in progress");
    return;
  }

  bool status_on = status.brightness > 0;
  float status_brightness = status.brightness / 100.0f;

  // Update brightness/state only if not waiting for our change to be reflected
  if (brightness_matches) {
    if (status_on != this->state_->current_values.get_state()) {
      ESP_LOGV(TAG, "Updating light state from slave: brightness=%d%%",
               status.brightness);
      auto call = this->state_->make_call();
      if (status_on) {
        call.set_brightness(status_brightness);
      }
      call.set_state(status_brightness > 0.0f);
      call.set_transition_length(0);  // Instant update
      call.perform();
    } else if (status_on && this->state_->current_values.get_brightness() != status_brightness) {
      ESP_LOGV(TAG, "Updating light brightness from slave: brightness=%d%%",
               status.brightness);
      auto call = this->state_->make_call();
      call.set_brightness(status_brightness);
      call.set_transition_length(0);  // Instant update
      call.perform();
    }
  } else {
    ESP_LOGV(TAG, "Ignoring brightness update from slave (waiting for expected: %d%%, got: %d%%)",
             this->pending_change_.expected_brightness, status.brightness);
  }

  // Update color temperature only if not waiting for our change to be reflected
  if (color_matches) {
    ColorTemp current_temp = this->kelvin_to_code(this->state_->current_values.get_color_temperature_kelvin());
    if (current_temp != status.color_temp) {
      ESP_LOGV(TAG, "Updating light color temperature from slave: color_temp_code=0x%02X",
               status.color_temp);
      auto call = this->state_->make_call();
      float kelvin = this->code_to_kelvin(status.color_temp);
      float mireds = MIRED_CONVERSION / kelvin;
      call.set_color_temperature(mireds);
      call.set_transition_length(0);  // Instant update
      call.perform();
    }
  } else {
    ESP_LOGV(TAG, "Ignoring color temperature update from slave (waiting for expected: 0x%02X, got: 0x%02X)",
             this->pending_change_.expected_color_temp, status.color_temp);
  }
}

}  // namespace hubspace
}  // namespace esphome
