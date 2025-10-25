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
  auto traits = fan::FanTraits(false, true, false, 4);
  return traits;
}

void HubSpaceFan::control(const fan::FanCall &call) {
  if (call.get_state().has_value()) {
    this->state = *call.get_state();
  }
  if (call.get_speed().has_value()) {
    this->speed = *call.get_speed();
  }
  
  this->publish_state();
}

}  // namespace hubspace
}  // namespace esphome
