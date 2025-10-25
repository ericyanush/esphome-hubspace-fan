# ESPHome HubSpace Fan/Light Component

A custom ESPHome component for controlling HubSpace fan and light combination devices, similar to the Tuya component.

## Features

- Control HubSpace fan devices with multiple speed levels (4 speeds)
- Control HubSpace light devices with brightness control
- Integrates seamlessly with Home Assistant
- Based on ESPHome's component architecture

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
# Enable the HubSpace component
hubspace:

# Configure the fan
fan:
  - platform: hubspace
    name: "Bedroom Fan"

# Configure the light
light:
  - platform: hubspace
    name: "Bedroom Light"
```

### Complete Example

See [example.yaml](example.yaml) for a complete configuration example.

## Component Structure

```
components/hubspace/
├── __init__.py          # Main component configuration
├── hubspace.h           # C++ header for base component
├── hubspace.cpp         # C++ implementation for base component
├── fan.py               # Fan platform configuration
├── fan.h                # C++ header for fan
├── fan.cpp              # C++ implementation for fan
├── light.py             # Light platform configuration
├── light.h              # C++ header for light
└── light.cpp            # C++ implementation for light
```

## Features

### Fan Control

- **On/Off**: Turn the fan on or off
- **Speed Control**: 4-speed fan control (low, medium, medium-high, high)
- **State Persistence**: Fan state is maintained and published to Home Assistant

### Light Control

- **On/Off**: Turn the light on or off
- **Brightness**: Adjust light brightness from 0-100%
- **Smooth Transitions**: Brightness changes with smooth transitions

## Development

This component is structured following ESPHome's component architecture:

1. **Python Configuration** (`__init__.py`, `fan.py`, `light.py`): Handles YAML configuration parsing and code generation
2. **C++ Implementation** (`.h` and `.cpp` files): Implements the actual device control logic

### Extending the Component

To add additional features:

1. Update the Python configuration files to add new config options
2. Modify the C++ implementation to handle the new features
3. Update the README with new configuration examples

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is open source and available under the MIT License.

## Acknowledgments

- Inspired by the ESPHome Tuya component
- Built on the ESPHome framework