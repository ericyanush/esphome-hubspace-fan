#include "hubspace.h"
#include "fan.h"
#include "light.h"
#include "esphome/core/log.h"

namespace esphome {
namespace hubspace {

static const char *const TAG = "hubspace";

// Protocol constants
static const uint8_t START_BYTE = 0x20;
static const uint8_t MASTER_FRAME_LEN = 5;
static const uint8_t SLAVE_FRAME_LEN = 12;
static const uint32_t KEEPALIVE_INTERVAL_MS = 200;  // ~5 Hz

void HubSpaceComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up HubSpace...");
}

void HubSpaceComponent::loop() {
  const uint32_t now = millis();
  
  // Send boot sequence once, about 2.4s after startup
  if (!this->boot_sent_ && now > 2400) {
    this->send_boot_sequence();
    this->boot_sent_ = true;
  }
  
  // Send keepalive every ~200ms
  if (now - this->last_keepalive_ >= KEEPALIVE_INTERVAL_MS) {
    this->send_keepalive();
    this->last_keepalive_ = now;
  }
  
  // Read and process incoming data
  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);
    
    // Look for start byte
    if (byte == START_BYTE && this->rx_buffer_.empty()) {
      this->rx_buffer_.push_back(byte);
    } else if (!this->rx_buffer_.empty()) {
      this->rx_buffer_.push_back(byte);
      
      // Check if we have a complete frame
      if (this->rx_buffer_.size() == SLAVE_FRAME_LEN) {
        SlaveStatus status;
        if (this->parse_slave_frame(this->rx_buffer_, status)) {
          this->process_slave_status(status);
          this->last_rx_ = now;
        } else {
          ESP_LOGW(TAG, "Invalid slave frame checksum");
        }
        this->rx_buffer_.clear();
      } else if (this->rx_buffer_.size() > SLAVE_FRAME_LEN) {
        // Buffer overflow, reset
        ESP_LOGW(TAG, "RX buffer overflow, resetting");
        this->rx_buffer_.clear();
      }
    }
  }
}

void HubSpaceComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "HubSpace:");
  ESP_LOGCONFIG(TAG, "  UART: 9600 8N1");
  this->check_uart_settings(9600);
}

float HubSpaceComponent::get_setup_priority() const {
  return setup_priority::AFTER_WIFI;
}

void HubSpaceComponent::send_command(uint8_t cmd, uint8_t high, uint8_t low) {
  uint8_t frame[MASTER_FRAME_LEN];
  frame[0] = START_BYTE;
  frame[1] = cmd;
  frame[2] = high;
  frame[3] = low;
  frame[4] = this->calculate_checksum(frame, 4);
  
  this->write_array(frame, MASTER_FRAME_LEN);
  this->flush();
  
  ESP_LOGV(TAG, "TX: %02X %02X %02X %02X %02X", frame[0], frame[1], frame[2], frame[3], frame[4]);
}

void HubSpaceComponent::send_keepalive() {
  this->send_command(CMD_KEEPALIVE, 0x00, 0x00);
}

void HubSpaceComponent::send_boot_sequence() {
  ESP_LOGD(TAG, "Sending boot sequence");
  
  // Send boot commands
  this->send_command(CMD_BOOT_09, 0x00, 0x00);
  delay(10);
  this->send_command(CMD_BOOT_0A, 0x00, 0x00);
  delay(10);
  this->send_command(CMD_BOOT_0B, 0x00, 0x00);
  delay(10);
  this->send_command(CMD_BOOT_0C, 0x00, 0x00);
  delay(10);
  this->send_command(CMD_BOOT_0E, 0x00, 0x00);
  delay(10);
  
  // Resend last known state
  this->send_command(CMD_FAN_SPEED, this->last_fan_speed_, 0x00);
  delay(10);
  this->send_command(CMD_BRIGHTNESS, this->last_brightness_, 0x00);
  delay(10);
  
  ESP_LOGD(TAG, "Boot sequence complete");
}

void HubSpaceComponent::send_fan_speed(uint8_t speed) {
  this->last_fan_speed_ = speed;
  this->send_command(CMD_FAN_SPEED, speed, 0x00);
}

void HubSpaceComponent::send_brightness(uint8_t brightness) {
  this->last_brightness_ = brightness;
  this->send_command(CMD_BRIGHTNESS, brightness, 0x00);
}

void HubSpaceComponent::send_direction(bool reverse) {
  this->last_direction_ = reverse;
  this->send_command(CMD_DIRECTION, reverse ? 0x01 : 0x00, 0x00);
}

void HubSpaceComponent::send_color_temp(uint8_t temp_code) {
  this->last_color_temp_ = temp_code;
  this->send_command(CMD_COLOR_TEMP, temp_code, 0x00);
}

uint8_t HubSpaceComponent::calculate_checksum(const uint8_t *data, size_t len) {
  uint8_t checksum = 0;
  for (size_t i = 0; i < len; i++) {
    checksum ^= data[i];
  }
  return checksum;
}

bool HubSpaceComponent::parse_slave_frame(const std::vector<uint8_t> &frame, SlaveStatus &status) {
  if (frame.size() != SLAVE_FRAME_LEN || frame[0] != START_BYTE) {
    return false;
  }
  
  // Verify checksum
  uint8_t expected_checksum = this->calculate_checksum(frame.data(), SLAVE_FRAME_LEN - 1);
  if (frame[SLAVE_FRAME_LEN - 1] != expected_checksum) {
    ESP_LOGW(TAG, "Checksum mismatch: expected %02X, got %02X", expected_checksum, frame[SLAVE_FRAME_LEN - 1]);
    return false;
  }
  
  // Parse frame
  status.status = frame[1];
  status.rf_slot2 = frame[2];
  status.rf_slot3 = frame[3];
  status.fan_code = frame[4];
  status.brightness = frame[5];
  status.color_code = frame[6];
  status.timer_minutes = frame[7] | (frame[8] << 8);  // Little endian
  status.rf_slot9 = frame[9];
  status.stage = frame[10];
  
  ESP_LOGV(TAG, "RX: status=%02X fan=%02X bright=%d color=%02X timer=%d stage=%02X",
           status.status, status.fan_code, status.brightness, status.color_code, 
           status.timer_minutes, status.stage);
  
  return true;
}

void HubSpaceComponent::process_slave_status(const SlaveStatus &status) {
  // Update fan state if registered
  if (this->fan_ != nullptr) {
    this->fan_->update_from_slave(status.fan_code, status.stage);
  }
  
  // Update light state if registered
  if (this->light_ != nullptr) {
    this->light_->update_from_slave(status.brightness, status.color_code);
  }
}

}  // namespace hubspace
}  // namespace esphome
