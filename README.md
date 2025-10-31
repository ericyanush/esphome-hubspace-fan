# ESPHome HubSpace Fan/Light Component

A custom ESPHome component for controlling HubSpace fan and light combination devices via UART protocol.

## Features

- **UART Protocol Implementation**: Full implementation of the HubSpace UART protocol (9600 8N1)
- **Fan Control**: 
  - 6 discrete speed levels (16%, 33%, 50%, 66%, 83%, 100%)
  - Direction control (forward/reverse)
  - Real-time state synchronization
- **Light Control**: 
  - Brightness control (0-100%)
  - Color temperature control (2700K-6500K in 6 steps)
  - Real-time state synchronization
- **Bidirectional Communication**: Both commands to device and status updates from device
- **Automatic Keep-Alive**: Heartbeat signal every ~200ms as required by protocol
- **Boot Sequence**: Proper initialization sequence on startup

## Installation

### Option 1: External Components (Recommended)

Add this to your ESPHome configuration:

```yaml
external_components:
  - source: github://ericyanush/esphome-hubspace-fan
    components: [ hubspace ]
```

### Option 2: Manual Installation

1. Clone this repository
2. Copy the `components/hubspace` directory to your ESPHome configuration directory
3. Reference it in your YAML configuration

## Configuration

### Basic Configuration

```yaml
# UART configuration (required - 9600 8N1)
uart:
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 9600

# Enable the HubSpace component
hubspace:
  id: hubspace_hub

# Configure the fan
fan:
  - platform: hubspace
    name: "Bedroom Fan"
    hubspace_id: hubspace_hub

# Configure the light
light:
  - platform: hubspace
    name: "Bedroom Light"
    hubspace_id: hubspace_hub
```

### Complete Example

See [example.yaml](example.yaml) for a complete configuration example.

## Hardware Connection

The component communicates with the HubSpace fan controller via UART:

- **Baud Rate**: 9600 bps
- **Data Format**: 8N1 (8 data bits, no parity, 1 stop bit)
- **TX Pin**: Connect to the fan controller's RX
- **RX Pin**: Connect to the fan controller's TX

The ESP device acts as the "master" in the protocol, sending commands and receiving status updates from the fan controller (slave).

## Protocol Details

This component implements the full UART protocol as documented in [PROTOCOL.md](PROTOCOL.md):

### Master Commands (ESP → Fan Controller)
- **Keep-Alive** (0x01): Sent every ~200ms
- **Fan Speed** (0x02): Set fan to one of 6 speed levels
- **Brightness** (0x03): Set light brightness (0-100%)
- **Direction** (0x04): Set fan direction (forward/reverse)
- **Color Temperature** (0x08): Set color temp (2700K-6500K)
- **Boot Sequence** (0x09-0x0E): Initialization on startup

### Slave Status (Fan Controller → ESP)
- 12-byte frames with fan speed, brightness, color temp, direction
- Automatic state synchronization
- XOR checksum validation

## Component Structure

```
components/hubspace/
├── __init__.py          # Main component configuration
├── hubspace.h           # C++ header for base component
├── hubspace.cpp         # C++ implementation for UART protocol
├── fan.py               # Fan platform configuration
├── fan.h                # C++ header for fan
├── fan.cpp              # C++ implementation for fan control
├── light.py             # Light platform configuration
├── light.h              # C++ header for light
└── light.cpp            # C++ implementation for light control
```

## Features

### Fan Control

- **On/Off**: Turn the fan on or off
- **Speed Control**: 6 discrete speed levels
  - Level 1: 16%
  - Level 2: 33%
  - Level 3: 50%
  - Level 4: 66%
  - Level 5: 83%
  - Level 6: 100%
- **Direction**: Forward or reverse rotation
- **State Sync**: Automatic synchronization with device state

### Light Control

- **On/Off**: Turn the light on or off
- **Brightness**: Adjust light brightness from 0-100%
- **Color Temperature**: Adjust from warm (2700K) to cool (6500K) in 6 steps:
  - 2700K (Warm White)
  - 3000K
  - 3500K
  - 4000K
  - 5000K
  - 6500K (Cool White)
- **State Sync**: Automatic synchronization with device state

## Development

This component is structured following ESPHome's component architecture:

1. **Python Configuration** (`__init__.py`, `fan.py`, `light.py`): Handles YAML configuration parsing and code generation
2. **C++ Implementation** (`.h` and `.cpp` files): Implements the actual UART protocol and device control logic

### Protocol Implementation

The UART protocol is fully implemented in `hubspace.cpp`:
- XOR checksum calculation and validation
- 5-byte command frames to device
- 12-byte status frame parsing from device
- Keep-alive heartbeat management
- Boot sequence on startup

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is open source and available under the MIT License.

## Acknowledgments

- Protocol reverse-engineered and documented in [PROTOCOL.md](PROTOCOL.md)
- Built on the ESPHome framework