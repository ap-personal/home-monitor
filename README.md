# ESP32 Temperature & Humidity Monitor with ST7789 Display

A complete ESP32-based environmental monitoring system featuring temperature and humidity sensing with a high-quality TFT display. Built using clean component-based architecture with the ESP-IDF framework.

## 🌟 Features

- **Real-time Environmental Monitoring**: Continuous temperature and humidity readings
- **Large Font Display**: Easy-to-read measurements on 240x240 TFT display
- **Component-Based Architecture**: Clean, modular code structure
- **Centralized Pin Management**: Organized GPIO configuration for easy expansion
- **Robust Error Handling**: Graceful handling of sensor failures with cached readings
- **Low Power Design**: Optimized for continuous monitoring applications

## 📊 System Overview

This project creates a standalone environmental monitoring station that:
- Reads temperature and humidity from DHT11 sensor every 3 seconds
- Displays real-time data on a vibrant ST7789 TFT screen
- Maintains system reliability with comprehensive error handling
- Provides a foundation for expanding with additional sensors

## 🔧 Hardware Requirements

### Core Components

| Component | Model | Purpose |
|-----------|-------|---------|
| Microcontroller | ESP32 WROOM-32 | Main processing unit |
| Display | ST7789 240x240 TFT | Visual output |
| Sensor | DHT11 | Temperature & humidity |

### Wiring Connections

#### ST7789 240x240 TFT Display
| ST7789 Pin | ESP32 GPIO | Function |
|------------|------------|----------|
| GND        | GND        | Ground |
| VCC        | 3.3V       | Power Supply |
| SCK        | GPIO 18    | SPI Clock |
| SDA        | GPIO 23    | SPI Data (MOSI) |
| RES        | GPIO 4     | Reset (active low) |
| DC         | GPIO 2     | Data/Command Select |
| CS         | Not Used   | Chip Select (tied low) |

#### DHT11 Temperature & Humidity Sensor
| DHT11 Pin | ESP32 GPIO | Function |
|-----------|------------|----------|
| VCC       | 3.3V       | Power Supply |
| GND       | GND        | Ground |
| DATA      | GPIO 22    | Single-wire data |

### Available GPIO Pins for Expansion
- **Digital I/O**: GPIO 5, 12, 13, 14, 15, 16, 17, 19, 20, 21, 24, 25, 26, 27
- **Analog Input**: GPIO 32, 33, 34, 35, 36, 39
- **I2C Ready**: GPIO 20 (SCL), GPIO 21 (SDA)

## 🏗️ Project Architecture

### Component Structure
```
components/
├── pinout/                 # Centralized GPIO pin management
│   ├── pinout.h           # Pin definitions and conflict prevention
│   └── CMakeLists.txt     # Build configuration
├── st7789/                # ST7789 TFT display driver
│   ├── st7789.c           # Display driver implementation
│   ├── st7789.h           # Display API and constants
│   └── CMakeLists.txt     # Build configuration
├── dht11/                 # DHT11 temperature/humidity sensor
│   ├── dht11.c            # Sensor driver with precise timing
│   ├── dht11.h            # Sensor API and data structures
│   └── CMakeLists.txt     # Build configuration
└── system_manager/        # System coordinator and application logic
    ├── system_manager.c   # Main application and sensor coordination
    ├── system_manager.h   # System management API
    └── CMakeLists.txt     # Build configuration

main/
├── main.c                 # Minimal application entry point (25 lines)
└── CMakeLists.txt         # Main component configuration
```

### Clean Architecture Benefits

1. **Separation of Concerns**: Each component handles one responsibility
2. **Easy Testing**: Components can be tested independently
3. **Simple Expansion**: Add new sensors without affecting existing code
4. **Pin Conflict Prevention**: Centralized pinout management
5. **Maintainable Code**: Clear dependencies and interfaces

## 💾 Software Features

### ST7789 Display Driver
- **High-Speed SPI**: 27MHz for smooth graphics
- **Large Font Support**: Custom large font for easy reading
- **RGB565 Color Format**: 65,536 color support
- **Optimized Memory Usage**: Efficient pixel rendering
- **Hardware Reset Sequence**: Reliable initialization

### DHT11 Sensor Driver
- **Precise Timing**: Microsecond-accurate bit-bang protocol
- **Interrupt-Safe**: Critical sections protected from timing interference
- **Checksum Validation**: Data integrity verification
- **Error Recovery**: Graceful handling of communication failures
- **Cached Readings**: Fallback to last valid measurement

### System Manager
- **Coordinated Sensors**: Manages reading timing and conflicts
- **Display Layout**: Centered, easy-to-read temperature and humidity
- **Error Handling**: Continues operation despite sensor failures
- **Task Management**: FreeRTOS task for non-blocking operation

### Centralized Pin Management
- **Conflict Prevention**: `PIN_IS_USED()` macro prevents conflicts
- **Future Planning**: Pre-defined pins for common sensors
- **Documentation**: Complete pin usage map
- **Validation**: Compile-time pin conflict detection

## 📱 Display Output

The system shows a clean, centered display:

```
     ╔══════════════════════╗
     ║                      ║
     ║                      ║
     ║   TEMP: 23.5°C       ║
     ║                      ║
     ║   HUMD: 65%          ║
     ║                      ║
     ║                      ║
     ╚══════════════════════╝
```

**Display Features:**
- **Large, readable fonts** for easy viewing
- **Color-coded readings**: Red for temperature, Blue for humidity
- **Centered layout** optimized for 240x240 display
- **Real-time updates** every 3 seconds
- **Error indicators** show "--.-°C" / "--%"  for failed readings

## 🚀 Getting Started

### Prerequisites

- ESP-IDF v5.5 or later
- ESP32 development board
- ST7789 240x240 TFT display
- DHT11 temperature/humidity sensor
- Breadboard and jumper wires

### Building and Flashing

1. **Set up ESP-IDF environment:**
```bash
# Windows
C:\Espressif\frameworks\esp-idf-v5.5\export.bat

# Linux/Mac
source ~/esp/esp-idf/export.sh
```

2. **Navigate to project directory:**
```bash
cd /path/to/02_esp32_st7789
```

3. **Configure project (optional):**
```bash
idf.py menuconfig
```

4. **Build the project:**
```bash
idf.py build
```

5. **Flash to ESP32:**
```bash
idf.py flash
```

6. **Monitor output:**
```bash
idf.py monitor
```

### Expected Behavior

1. **Startup Screen**: "SYSTEM READY" message (2 seconds)
2. **Sensor Initialization**: DHT11 and display setup
3. **Continuous Monitoring**: Temperature and humidity display updates every 3 seconds
4. **Serial Output**: Detailed logging of sensor readings and system status

## 🔧 Configuration

### Modifying Pin Assignments

Edit `components/pinout/pinout.h` to change GPIO assignments:

```c
// Current pin assignments
#define ST7789_SCK_PIN      18  // SPI clock
#define ST7789_SDA_PIN      23  // SPI data
#define ST7789_RST_PIN      4   // Reset
#define ST7789_DC_PIN       2   // Data/Command
#define DHT11_DATA_PIN      22  // DHT11 data

// Available for new sensors
#define DIGITAL_PIN_1       5   // Available GPIO
#define ANALOG_PIN_1        32  // Available ADC
```

### Adding New Sensors

1. **Check pin availability** in `pinout.h`
2. **Create new component** in `components/` directory
3. **Add dependency** to `system_manager/CMakeLists.txt`
4. **Include in system_manager.c** for coordination

### Customizing Display

Modify display layout in `components/system_manager/system_manager.c`:

```c
// Temperature display
st7789_draw_large_string(10, 80, "TEMP:", ST7789_WHITE, ST7789_BLACK);
st7789_draw_large_string(120, 80, temp_str, ST7789_RED, ST7789_BLACK);

// Humidity display  
st7789_draw_large_string(10, 140, "HUMD:", ST7789_WHITE, ST7789_BLACK);
st7789_draw_large_string(120, 140, humidity_str, ST7789_BLUE, ST7789_BLACK);
```

## 📈 Performance Specifications

| Specification | Value |
|---------------|-------|
| **Update Rate** | Every 3 seconds |
| **Display Resolution** | 240x240 pixels |
| **Color Depth** | 16-bit (65,536 colors) |
| **SPI Speed** | 27MHz |
| **Temperature Range** | 0-50°C (DHT11) |
| **Humidity Range** | 20-95% RH (DHT11) |
| **Memory Usage** | ~50KB total |
| **Power Consumption** | ~150mA @ 3.3V |

## 🛠️ Troubleshooting

### Display Issues

**Symptom**: Blank or corrupted display
**Solutions**:
1. Check wiring connections
2. Verify 3.3V power supply
3. Confirm pin definitions in `pinout.h`
4. Test with `st7789_test()` function

### DHT11 Sensor Issues

**Symptom**: Shows "--.-°C" or "--%"
**Solutions**:
1. Check DHT11 wiring (especially data pin)
2. Verify 3.3V power to sensor
3. Ensure adequate power supply capacity
4. Check for loose connections

### Build Errors

**Symptom**: Compilation failures
**Solutions**:
1. Verify ESP-IDF version (v5.5+)
2. Check component dependencies
3. Clean build: `idf.py fullclean && idf.py build`
4. Verify all header files include `pinout.h`

### Memory Issues

**Symptom**: Stack overflow or allocation failures
**Solutions**:
1. Increase task stack size in `system_manager.c`
2. Check for memory leaks in sensor drivers
3. Optimize buffer sizes

## 🔮 Future Enhancements

### Suggested Additional Sensors

| Sensor | GPIO Pins | Purpose |
|--------|-----------|---------|
| **BME280** | 20 (SCL), 21 (SDA) | Precision environmental sensing |
| **DS18B20** | 14 | High-accuracy temperature |
| **PIR Motion** | 15 | Occupancy detection |
| **Photoresistor** | 32 (ADC) | Light level monitoring |
| **Buzzer** | 25 (DAC) | Alert notifications |

### Software Enhancements

- **Data Logging**: Store readings to SD card or flash memory
- **WiFi Connectivity**: Send data to cloud services
- **Web Interface**: Browser-based monitoring dashboard
- **Alerts**: Configurable thresholds with notifications
- **Historical Graphs**: Trend visualization on display
- **Multiple Sensors**: Support for sensor arrays

### Hardware Upgrades

- **Battery Power**: Li-ion battery with charging circuit
- **Enclosure**: 3D-printed weatherproof housing
- **Larger Display**: 320x240 or OLED upgrade
- **External Sensors**: Waterproof probe extensions

## 🤝 Contributing

Contributions are welcome! Please follow these guidelines:

1. **Fork** the repository
2. **Create** a feature branch
3. **Follow** existing code style and architecture
4. **Test** thoroughly on hardware
5. **Document** changes in README
6. **Submit** pull request with detailed description

## 📄 License

This project is open-source and available under the MIT License. Feel free to use, modify, and distribute according to the license terms.

## 🙏 Acknowledgments

- **ESP-IDF Framework**: Espressif's comprehensive IoT development framework
- **ST7789 Community**: Open-source display driver implementations
- **DHT Sensor Libraries**: Reference implementations for timing protocols

---

**Built with ❤️ for the ESP32 community**