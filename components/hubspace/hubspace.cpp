#include "hubspace.h"
#include "esphome/core/log.h"

namespace esphome {
namespace hubspace {

static const char *const TAG = "hubspace";

void HubSpaceComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up HubSpace...");
}

void HubSpaceComponent::loop() {
  // Main loop for handling communication
}

void HubSpaceComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "HubSpace:");
}

float HubSpaceComponent::get_setup_priority() const {
  return setup_priority::AFTER_WIFI;
}

}  // namespace hubspace
}  // namespace esphome
