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

enum FanDirection : uint8_t {
  DIRECTION_FORWARD = 0x00,
  DIRECTION_REVERSE = 0x80,
  DIRECTION_CHANGE_TO_FORWARD = 0xC0,
  DIRECTION_CHANGE_TO_REVERSE = 0x40,
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
  uint8_t response_cmd;     // byte 1 - command code being responded to
  uint8_t reserved1;        // byte 2
  uint8_t reserved2;        // byte 3
  uint8_t fan_code;         // byte 4
  uint8_t brightness;       // byte 5
  uint8_t color_code;       // byte 6
  uint16_t timer_minutes;   // bytes 7-8 (little endian)
  uint8_t reserved3;        // byte 9
  uint8_t stage;            // byte 10
};

struct FanStatus {
  FanSpeed fan_speed;
  FanDirection direction;
};

struct LightStatus {
  uint8_t brightness;     // 0-100%
  ColorTemp color_temp;   // in Kelvin
};

struct DeviceStatus {
  FanStatus fan_status;
  LightStatus light_status;
};

// Command queue structure
struct QueuedCommand {
  uint8_t cmd;
  uint8_t high;
  uint8_t low;
  uint32_t queued_time;
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
  void send_fan_speed(FanSpeed speed);
  void send_brightness(uint8_t brightness);
  void send_direction(bool reverse);
  void send_color_temp(ColorTemp temp_code);

 protected:
  // Protocol implementation
  void send_command(uint8_t cmd, uint8_t high, uint8_t low);
  void queue_command(uint8_t cmd, uint8_t high, uint8_t low);
  void process_command_queue();
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
  uint32_t last_command_sent_{0};
  bool boot_sent_{false};
  std::vector<uint8_t> rx_buffer_;
  
  // Command queue
  std::vector<QueuedCommand> command_queue_;
  uint8_t pending_response_cmd_{0};  // Command waiting for response (0 = none pending)
  uint32_t pending_cmd_sent_time_{0};  // When the pending command was sent
  static const uint32_t COMMAND_INTERVAL_MS = 50;  // 50ms between commands
  static const uint32_t COMMAND_TIMEOUT_MS = 1000;  // 1s timeout for responses
  
  DeviceStatus device_status_;
};

}  // namespace hubspace
}  // namespace esphome
