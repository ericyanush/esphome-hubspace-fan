#include "fan.h"
#include "esphome/core/log.h"

namespace esphome {
namespace hubspace {

static const char *const TAG = "hubspace.fan";

void HubSpaceFan::setup() {
  ESP_LOGCONFIG(TAG, "Setting up HubSpace Fan...");
  this->state = false;
  this->speed = 0;
}

void HubSpaceFan::dump_config() {
  ESP_LOGCONFIG(TAG, "HubSpace Fan:");
  LOG_FAN("  ", "Fan", this);
}

fan::FanTraits HubSpaceFan::get_traits() {
  // 6 speed levels + direction control, no oscillation
  auto traits = fan::FanTraits(false, true, true, 6);
  return traits;
}

void HubSpaceFan::control(const fan::FanCall &call) {
  if (this->parent_ == nullptr) {
    ESP_LOGE(TAG, "Parent not set!");
    return;
  }

  bool state_changed = false;
  
  if (call.get_state().has_value()) {
    this->state = *call.get_state();
    state_changed = true;
  }
  
  if (call.get_speed().has_value()) {
    this->speed = *call.get_speed();
    state_changed = true;
  }
  
  if (call.get_direction().has_value()) {
    this->direction = *call.get_direction();
    bool reverse = (this->direction == fan::FanDirection::REVERSE);
    this->parent_->send_direction(reverse);
  }
  
  if (state_changed) {
    uint8_t fan_code;
    if (!this->state || this->speed == 0) {
      fan_code = FAN_OFF;
    } else {
      fan_code = this->speed_level_to_code(this->speed);
    }
    this->parent_->send_fan_speed(fan_code);
  }
  
  this->publish_state();
}

uint8_t HubSpaceFan::speed_level_to_code(int speed_level) {
  switch (speed_level) {
    case 1: return FAN_LEVEL_1;
    case 2: return FAN_LEVEL_2;
    case 3: return FAN_LEVEL_3;
    case 4: return FAN_LEVEL_4;
    case 5: return FAN_LEVEL_5;
    case 6: return FAN_LEVEL_6;
    default: return FAN_OFF;
  }
}

int HubSpaceFan::code_to_speed_level(uint8_t code) {
  switch (code) {
    case FAN_LEVEL_1: return 1;
    case FAN_LEVEL_2: return 2;
    case FAN_LEVEL_3: return 3;
    case FAN_LEVEL_4: return 4;
    case FAN_LEVEL_5: return 5;
    case FAN_LEVEL_6: return 6;
    case FAN_OFF:
    default: return 0;
  }
}

bool HubSpaceFan::is_direction_reverse(uint8_t stage) {
  // Check top two bits of stage byte
  // 0x00 (00xxxxxx) = Forward idle
  // 0x40 (01xxxxxx) = Forward -> Reverse transition
  // 0x80 (10xxxxxx) = Reverse idle
  // 0xC0 (11xxxxxx) = Reverse -> Forward transition
  return (stage & 0x80) != 0;
}

void HubSpaceFan::update_from_slave(uint8_t fan_code, uint8_t stage) {
  bool changed = false;
  
  // Update speed
  int new_speed = this->code_to_speed_level(fan_code);
  bool new_state = (new_speed > 0);
  
  if (this->state != new_state) {
    this->state = new_state;
    changed = true;
  }
  
  if (this->speed != new_speed) {
    this->speed = new_speed;
    changed = true;
  }
  
  // Update direction
  bool reverse = this->is_direction_reverse(stage);
  fan::FanDirection new_direction = reverse ? fan::FanDirection::REVERSE : fan::FanDirection::FORWARD;
  if (this->direction != new_direction) {
    this->direction = new_direction;
    changed = true;
  }
  
  if (changed) {
    this->publish_state();
  }
}

}  // namespace hubspace
}  // namespace esphome
