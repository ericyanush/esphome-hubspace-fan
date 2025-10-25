#pragma once

#include "esphome/components/fan/fan.h"
#include "esphome/core/component.h"
#include "hubspace.h"

namespace esphome {
namespace hubspace {

class HubSpaceFan : public fan::Fan, public Component {
 public:
  void setup() override;
  void dump_config() override;
  fan::FanTraits get_traits() override;

  void set_parent(HubSpaceComponent *parent) { this->parent_ = parent; }
  void update_from_slave(uint8_t fan_code, uint8_t stage);

 protected:
  void control(const fan::FanCall &call) override;
  uint8_t speed_level_to_code(int speed_level);
  int code_to_speed_level(uint8_t code);
  bool is_direction_reverse(uint8_t stage);

  HubSpaceComponent *parent_{nullptr};
};

}  // namespace hubspace
}  // namespace esphome
