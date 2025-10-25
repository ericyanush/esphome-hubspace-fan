#pragma once

#include "esphome/components/light/light_output.h"
#include "esphome/core/component.h"

namespace esphome {
namespace hubspace {

class HubSpaceLight : public light::LightOutput, public Component {
 public:
  void setup() override;
  void dump_config() override;
  light::LightTraits get_traits() override;
  void write_state(light::LightState *state) override;
};

}  // namespace hubspace
}  // namespace esphome
