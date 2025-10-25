#pragma once

#include "esphome/components/fan/fan.h"
#include "esphome/core/component.h"

namespace esphome {
namespace hubspace {

class HubSpaceFan : public fan::Fan, public Component {
 public:
  void setup() override;
  void dump_config() override;
  fan::FanTraits get_traits() override;

 protected:
  void control(const fan::FanCall &call) override;
};

}  // namespace hubspace
}  // namespace esphome
