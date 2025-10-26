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
static const uint32_t KEEPALIVE_INTERVAL_MS = 200;  // 5 Hz

void HubSpaceComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up HubSpace...");
  this->send_boot_sequence();
}

void HubSpaceComponent::loop() {
  const uint32_t now = millis();
  
  // Process command queue
  this->process_command_queue();
  
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
  
  this->last_command_sent_ = millis();
  
  ESP_LOGV(TAG, "TX: %02X %02X %02X %02X %02X", frame[0], frame[1], frame[2], frame[3], frame[4]);
}

void HubSpaceComponent::queue_command(uint8_t cmd, uint8_t high, uint8_t low) {
  // Check if this command is already in the queue (avoid duplicates)
  for (auto &queued_cmd : this->command_queue_) {
    if (queued_cmd.cmd == cmd) {
      // Update existing command with new values
      queued_cmd.high = high;
      queued_cmd.low = low;
      queued_cmd.queued_time = millis();
      ESP_LOGV(TAG, "Updated queued command: 0x%02X %02X %02X", cmd, high, low);
      return;
    }
  }
  
  // Add new command to queue
  QueuedCommand new_cmd = {cmd, high, low, millis()};
  this->command_queue_.push_back(new_cmd);
  ESP_LOGV(TAG, "Queued command: 0x%02X %02X %02X", cmd, high, low);
}

void HubSpaceComponent::process_command_queue() {
  if (this->command_queue_.empty()) {
    return;
  }
  
  uint32_t now = millis();
  
  // If we're waiting for a response, check for timeout
  if (this->pending_response_cmd_ != 0) {
    if (now - this->pending_cmd_sent_time_ > COMMAND_TIMEOUT_MS) {
      ESP_LOGW(TAG, "Command 0x%02X timed out, proceeding to next command", this->pending_response_cmd_);
      this->pending_response_cmd_ = 0;  // Clear timeout and continue
    } else {
      return;  // Still waiting for response
    }
  }
  
  // Check if enough time has passed since last command (fallback protection)
  if (now - this->last_command_sent_ < COMMAND_INTERVAL_MS) {
    return;
  }
  
  // Send the first queued command
  QueuedCommand &cmd = this->command_queue_.front();
  this->send_command(cmd.cmd, cmd.high, cmd.low);
  
  // Track that we're waiting for a response to this command
  this->pending_response_cmd_ = cmd.cmd;
  this->pending_cmd_sent_time_ = now;
  
  ESP_LOGD(TAG, "Sent queued command: 0x%02X %02X %02X (queued for %dms), waiting for response", 
           cmd.cmd, cmd.high, cmd.low, now - cmd.queued_time);
  
  // Remove the sent command from queue
  this->command_queue_.erase(this->command_queue_.begin());
}

void HubSpaceComponent::send_keepalive() {
  // Don't send keepalive if we're waiting for a command response
  if (this->pending_response_cmd_ != 0) {
    return;
  }
  
  // Don't queue keepalive commands, send immediately if no recent commands
  uint32_t now = millis();
  if (now - this->last_command_sent_ >= COMMAND_INTERVAL_MS) {
    this->send_command(CMD_KEEPALIVE, 0x00, 0x00);
    // Don't set pending_response_cmd for keepalive as it may not always get a response
  }
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
  
  ESP_LOGD(TAG, "Boot sequence complete");
}

void HubSpaceComponent::send_fan_speed(FanSpeed speed) {
  this->queue_command(CMD_FAN_SPEED, static_cast<uint8_t>(speed), 0x00);
}

void HubSpaceComponent::send_brightness(uint8_t brightness) {
  this->queue_command(CMD_BRIGHTNESS, brightness, 0x00);
}

void HubSpaceComponent::send_direction(bool reverse) {
  this->queue_command(CMD_DIRECTION, reverse ? 0x01 : 0x00, 0x00);
}

void HubSpaceComponent::send_color_temp(ColorTemp temp_code) {
  this->queue_command(CMD_COLOR_TEMP, static_cast<uint8_t>(temp_code), 0x00);
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
  status.response_cmd = frame[1];
  status.reserved1 = frame[2];
  status.reserved2 = frame[3];
  status.fan_code = frame[4];
  status.brightness = frame[5];
  status.color_code = frame[6];
  status.timer_minutes = frame[7] | (frame[8] << 8);  // Little endian
  status.reserved3 = frame[9];
  status.stage = frame[10];
  
  ESP_LOGV(TAG, "RX: response_cmd=%02X fan=%02X bright=%d color=%02X timer=%d stage=%02X",
           status.response_cmd, status.fan_code, status.brightness, status.color_code, 
           status.timer_minutes, status.stage);
  
  return true;
}

void HubSpaceComponent::process_slave_status(const SlaveStatus &status) {
  // Check if this response matches our pending command
  if (this->pending_response_cmd_ != 0 && status.response_cmd == this->pending_response_cmd_) {
    uint32_t response_time = millis() - this->pending_cmd_sent_time_;
    ESP_LOGD(TAG, "Received response for command 0x%02X after %dms", status.response_cmd, response_time);
    this->pending_response_cmd_ = 0;  // Clear pending response
  } else if (this->pending_response_cmd_ != 0 && status.response_cmd != this->pending_response_cmd_) {
    ESP_LOGV(TAG, "Received response 0x%02X while waiting for 0x%02X", 
             status.response_cmd, this->pending_response_cmd_);
  }
  
  DeviceStatus new_status = {
    FanStatus{
      static_cast<FanSpeed>(status.fan_code),
      static_cast<FanDirection>(status.stage)
    },
    LightStatus{
      status.brightness,
      static_cast<ColorTemp>(status.color_code)
    }
  };
  
  // Determine if we should trigger updates
  bool should_update_fan = false;
  bool should_update_light = false;
  
  // Always update on non-keepalive responses (actual command responses)
  if (status.response_cmd != CMD_KEEPALIVE) {
    should_update_fan = true;
    should_update_light = true;
    ESP_LOGV(TAG, "Triggering updates due to non-keepalive response: 0x%02X", status.response_cmd);
  } else if (this->has_previous_status_) {
    // For keepalive responses, only update if there are actual changes
    if (new_status.fan_status.fan_speed != this->previous_device_status_.fan_status.fan_speed ||
        new_status.fan_status.direction != this->previous_device_status_.fan_status.direction) {
      should_update_fan = true;
      ESP_LOGV(TAG, "Fan status changed - triggering fan update");
    }
    
    if (new_status.light_status.brightness != this->previous_device_status_.light_status.brightness ||
        new_status.light_status.color_temp != this->previous_device_status_.light_status.color_temp) {
      should_update_light = true;
      ESP_LOGV(TAG, "Light status changed - triggering light update");
    }
  } else {
    // First time receiving status, trigger initial updates
    should_update_fan = true;
    should_update_light = true;
    ESP_LOGV(TAG, "First status received - triggering initial updates");
  }
  
  // Store the previous status for next comparison
  this->previous_device_status_ = this->device_status_;
  this->has_previous_status_ = true;
  
  // Update current status
  this->device_status_ = new_status;

  // Update fan state if registered and changes detected
  if (this->fan_ != nullptr && should_update_fan) {
    this->fan_->update_from_slave(this->device_status_.fan_status);
  }
  
  // Update light state if registered and changes detected
  if (this->light_ != nullptr && should_update_light) {
    this->light_->update_from_slave(this->device_status_.light_status);
  }
}

}  // namespace hubspace
}  // namespace esphome
