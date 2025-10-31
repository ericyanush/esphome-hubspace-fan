#pragma once

#include "esphome/components/light/light_output.h"
#include "esphome/core/component.h"
#include "hubspace.h"

namespace esphome {
namespace hubspace {

class HubSpaceLight : public light::LightOutput, public Component {
 public:
  void setup() override;
  void dump_config() override;
  light::LightTraits get_traits() override;
  void setup_state(light::LightState *state) override;
  void write_state(light::LightState *state) override;

  void set_parent(HubSpaceComponent *parent) { this->parent_ = parent; }
  void update_from_slave(LightStatus status);

 protected:
  light::LightState *state_{nullptr};
  ColorTemp kelvin_to_code(float kelvin);
  float code_to_kelvin(uint8_t code);

  HubSpaceComponent *parent_{nullptr};
  
  // Track pending changes to avoid update loops
  struct PendingChange {
    bool has_brightness_change{false};
    uint8_t expected_brightness{0};
    bool has_color_change{false};
    ColorTemp expected_color_temp{TEMP_3500K};
  };
  PendingChange pending_change_;
};

}  // namespace hubspace
}  // namespace esphome
