# Development Guide

## Component Architecture

This ESPHome custom component is structured following the standard ESPHome architecture with three main layers:

### 1. Python Configuration Layer

Located in `components/hubspace/`:
- `__init__.py` - Main component configuration and schema
- `fan.py` - Fan platform configuration
- `light.py` - Light platform configuration

These files handle:
- YAML configuration parsing
- Config validation
- Code generation for C++ classes
- Registration of components with ESPHome

### 2. C++ Implementation Layer

#### Base Component (`hubspace.h/cpp`)
- Manages the lifecycle of the HubSpace component
- Handles setup after WiFi is ready
- Provides a central point for device communication

#### Fan Platform (`fan.h/cpp`)
- Inherits from ESPHome's `fan::Fan` class
- Implements 4-speed fan control
- Handles state management and publishing

#### Light Platform (`light.h/cpp`)
- Inherits from ESPHome's `light::LightOutput` class
- Implements brightness control
- Handles light state management

## Key Features Implemented

### Fan Control
- **Speed Levels**: 4 discrete speed levels
- **State Management**: On/off state tracking
- **Home Assistant Integration**: Full integration with HA fan entity

### Light Control
- **Brightness**: 0-100% brightness control
- **Color Mode**: Brightness-only mode
- **State Management**: Current brightness tracking

## Future Enhancements

Potential areas for expansion:

1. **Protocol Implementation**
   - Add actual HubSpace protocol communication
   - Implement device discovery
   - Add authentication/pairing logic

2. **Additional Features**
   - Color temperature control (if supported by device)
   - RGB color control (if supported)
   - Fan oscillation control
   - Fan direction control

3. **Network Communication**
   - Implement UDP/TCP communication with HubSpace devices
   - Add mDNS discovery
   - Cloud API integration (if needed)

4. **Configuration Options**
   - Device IP address configuration
   - Device ID/authentication tokens
   - Update intervals
   - Timeout settings

## Testing

To test this component:

1. Create an ESPHome configuration using the example
2. Compile the configuration
3. Flash to an ESP8266 or ESP32 device
4. Monitor logs for proper initialization
5. Control via Home Assistant interface

## Code Style

This component follows ESPHome's coding standards:
- C++ code follows ESPHome style guide
- Python code follows PEP 8
- 2-space indentation for C++, 4-space for Python
- Descriptive variable names
- Comments for complex logic

## Component Dependencies

- **network**: Required for WiFi connectivity
- **fan**: Fan component for fan entity
- **light**: Light component for light entity

## How ESPHome Components Work

1. **Configuration Phase**: Python code parses YAML, validates config
2. **Code Generation**: Python generates C++ code for the firmware
3. **Compilation**: C++ code is compiled into ESP firmware
4. **Runtime**: C++ code runs on the device, managing hardware/network

This architecture keeps configuration simple while allowing powerful C++ code on the device.
