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
    
    // Track expected direction change
    this->pending_change_.has_direction_change = true;
    this->pending_change_.expected_direction = reverse ? DIRECTION_REVERSE : DIRECTION_FORWARD;
  }
  
  if (state_changed) {
    uint8_t fan_code;
    if (!this->state || this->speed == 0) {
      fan_code = FAN_OFF;
    } else {
      fan_code = this->speed_level_to_code(this->speed);
    }
    this->parent_->send_fan_speed(fan_code);
    
    // Track expected speed change
    this->pending_change_.has_speed_change = true;
    this->pending_change_.expected_speed = static_cast<FanSpeed>(fan_code);
  }
  
  this->publish_state();
}

FanSpeed HubSpaceFan::speed_level_to_code(int speed_level) {
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

void HubSpaceFan::update_from_slave(FanStatus status) {
  // Check if this update matches our pending changes
  bool speed_matches = !this->pending_change_.has_speed_change || 
                      (status.fan_speed == this->pending_change_.expected_speed);
  bool direction_matches = !this->pending_change_.has_direction_change || 
                          (status.direction == this->pending_change_.expected_direction);
  
  // Clear pending changes if slave has caught up
  if (speed_matches && this->pending_change_.has_speed_change) {
    ESP_LOGV(TAG, "Slave confirmed speed change: 0x%02X", status.fan_speed);
    this->pending_change_.has_speed_change = false;
  }
  
  if (direction_matches && this->pending_change_.has_direction_change) {
    ESP_LOGV(TAG, "Slave confirmed direction change: %s", 
             status.direction == DIRECTION_REVERSE ? "REVERSE" : "FORWARD");
    this->pending_change_.has_direction_change = false;
  }
  
  bool changed = false;
  
  // Update speed only if not waiting for our change to be reflected
  if (speed_matches) {
    int new_speed = this->code_to_speed_level(status.fan_speed);
    bool new_state = (new_speed > 0);
    
    if (this->state != new_state) {
      this->state = new_state;
      changed = true;
    }
    
    if (this->speed != new_speed) {
      this->speed = new_speed;
      changed = true;
    }
  } else {
    ESP_LOGV(TAG, "Ignoring speed update from slave (waiting for expected: 0x%02X, got: 0x%02X)",
             this->pending_change_.expected_speed, status.fan_speed);
  }
  
  // Update direction only if not waiting for our change to be reflected
  if (direction_matches) {
    fan::FanDirection new_direction = status.direction == FanDirection::DIRECTION_REVERSE ? fan::FanDirection::REVERSE : fan::FanDirection::FORWARD;
    if (this->direction != new_direction) {
      this->direction = new_direction;
      changed = true;
    }
  } else {
    ESP_LOGV(TAG, "Ignoring direction update from slave (waiting for expected: %s, got: %s)",
             this->pending_change_.expected_direction == DIRECTION_REVERSE ? "REVERSE" : "FORWARD",
             status.direction == DIRECTION_REVERSE ? "REVERSE" : "FORWARD");
  }
  
  if (changed) {
    this->publish_state();
  }
}

}  // namespace hubspace
}  // namespace esphome
