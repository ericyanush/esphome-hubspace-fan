#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/uart/uart.h"
#include <vector>

namespace esphome {
namespace hubspace {

// Forward declarations
class HubSpaceFan;
class HubSpaceLight;

// Fan speed levels according to protocol
enum FanSpeed : uint8_t {
  FAN_OFF = 0x00,
  FAN_LEVEL_1 = 0x10,  // 16%
  FAN_LEVEL_2 = 0x21,  // 33%
  FAN_LEVEL_3 = 0x32,  // 50%
  FAN_LEVEL_4 = 0x42,  // 66%
  FAN_LEVEL_5 = 0x53,  // 83%
  FAN_LEVEL_6 = 0x64,  // 100%
};

// Color temperature codes
enum ColorTemp : uint8_t {
  TEMP_2700K = 0x01,
  TEMP_3000K = 0x02,
  TEMP_3500K = 0x03,
  TEMP_4000K = 0x04,
  TEMP_5000K = 0x05,
  TEMP_6500K = 0x06,
};

// Master command codes
enum Command : uint8_t {
  CMD_KEEPALIVE = 0x01,
  CMD_FAN_SPEED = 0x02,
  CMD_BRIGHTNESS = 0x03,
  CMD_DIRECTION = 0x04,
  CMD_COLOR_TEMP = 0x08,
  CMD_BOOT_09 = 0x09,
  CMD_BOOT_0A = 0x0A,
  CMD_BOOT_0B = 0x0B,
  CMD_BOOT_0C = 0x0C,
  CMD_BOOT_0E = 0x0E,
};

// Slave status frame structure (12 bytes)
struct SlaveStatus {
  uint8_t status;           // byte 1
  uint8_t rf_slot2;         // byte 2
  uint8_t rf_slot3;         // byte 3
  uint8_t fan_code;         // byte 4
  uint8_t brightness;       // byte 5
  uint8_t color_code;       // byte 6
  uint16_t timer_minutes;   // bytes 7-8 (little endian)
  uint8_t rf_slot9;         // byte 9
  uint8_t stage;            // byte 10
};

class HubSpaceComponent : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;

  // Register fan and light components
  void register_fan(HubSpaceFan *fan) { this->fan_ = fan; }
  void register_light(HubSpaceLight *light) { this->light_ = light; }

  // Send commands
  void send_fan_speed(uint8_t speed);
  void send_brightness(uint8_t brightness);
  void send_direction(bool reverse);
  void send_color_temp(uint8_t temp_code);

 protected:
  // Protocol implementation
  void send_command(uint8_t cmd, uint8_t high, uint8_t low);
  void send_keepalive();
  void send_boot_sequence();
  uint8_t calculate_checksum(const uint8_t *data, size_t len);
  bool parse_slave_frame(const std::vector<uint8_t> &frame, SlaveStatus &status);
  void process_slave_status(const SlaveStatus &status);

  // State tracking
  HubSpaceFan *fan_{nullptr};
  HubSpaceLight *light_{nullptr};
  uint32_t last_keepalive_{0};
  uint32_t last_rx_{0};
  bool boot_sent_{false};
  std::vector<uint8_t> rx_buffer_;
  
  // Last sent values for resend on boot
  uint8_t last_fan_speed_{0x32};      // Default to level 3 (50%)
  uint8_t last_brightness_{0x39};     // Default to 57%
  uint8_t last_color_temp_{0x03};     // Default to 3500K
  bool last_direction_{false};        // Default to forward
};

}  // namespace hubspace
}  // namespace esphome
