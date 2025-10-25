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
  void write_state(light::LightState *state) override;

  void set_parent(HubSpaceComponent *parent) { this->parent_ = parent; }
  void update_from_slave(uint8_t brightness, uint8_t color_code);

 protected:
  uint8_t kelvin_to_code(float kelvin);
  float code_to_kelvin(uint8_t code);

  HubSpaceComponent *parent_{nullptr};
};

}  // namespace hubspace
}  // namespace esphome
