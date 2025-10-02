#ifndef PINOUT_H
#define PINOUT_H

/**
 * @file pinout.h
 * @brief ESP32 GPIO Pin Assignment and Management System
 * 
 * This header provides centralized GPIO pin management for the ESP32 Environmental
 * Monitor project. By consolidating all pin assignments in a single location,
 * this system prevents pin conflicts, simplifies hardware modifications, and
 * provides clear documentation of the complete system pinout.
 * 
 * Design Philosophy:
 * Centralized pin management offers several advantages:
 * - Prevents accidental pin conflicts between components
 * - Enables easy hardware reconfiguration without code changes
 * - Provides clear documentation of physical connections
 * - Simplifies PCB design and prototyping
 * - Facilitates hardware debugging and troubleshooting
 * 
 * ESP32 GPIO Considerations:
 * The ESP32 has specific GPIO limitations that must be considered:
 * - GPIO 0: Boot mode selection (avoid for regular I/O)
 * - GPIO 2: Boot mode selection (use with caution)
 * - GPIO 6-11: Connected to internal flash (DO NOT USE)
 * - GPIO 12: MTDI (affects boot voltage, use carefully)
 * - GPIO 15: MTDO (affects boot silence, use carefully)
 * - GPIO 34-39: Input-only pins (no internal pull-up/pull-down)
 * 
 * Hardware Architecture:
 * This pinout supports the following system configuration:
 * - ST7789 240x240 TFT Display (SPI interface)
 * - DHT11 Temperature/Humidity Sensor (single-wire protocol)
 * - Future expansion capability for additional sensors
 * - WiFi connectivity (internal ESP32 radio)
 * 
 * Complete ESP32 Pin Usage Map:
 * - GPIO 0:  Boot/Flash (system reserved)
 * - GPIO 1:  UART TX (system reserved)
 * - GPIO 2:  ST7789 DC (Data/Command)
 * - GPIO 3:  UART RX (system reserved)
 * - GPIO 4:  ST7789 RST (Reset)
 * - GPIO 5:  Available for expansion
 * - GPIO 6-11: Flash memory (system reserved - DO NOT USE)
 * - GPIO 12: Available (MTDI - boot sensitive)
 * - GPIO 13: Available for expansion
 * - GPIO 14: Available for expansion
 * - GPIO 15: Available (MTDO - boot sensitive)
 * - GPIO 16: Available for expansion
 * - GPIO 17: Available for expansion
 * - GPIO 18: ST7789 SCK (SPI Clock)
 * - GPIO 19: Available for expansion
 * - GPIO 20: Available for expansion
 * - GPIO 21: Available (default I2C SDA)
 * - GPIO 22: DHT11 Data (single-wire communication)
 * - GPIO 23: ST7789 SDA (SPI MOSI)
 * - GPIO 24: Available for expansion
 * - GPIO 25: Available (DAC1, LED status indicator)
 * - GPIO 26: Available (DAC2)
 * - GPIO 27: Available for expansion
 * - GPIO 32: Available (ADC1_CH4)
 * - GPIO 33: Available (ADC1_CH5)
 * - GPIO 34: Available (ADC1_CH6, input only)
 * - GPIO 35: Available (ADC1_CH7, input only)
 * - GPIO 36: Available (ADC1_CH0, input only)
 * - GPIO 39: Available (ADC1_CH3, input only)
 * 
 * @author ESP32 Hardware Team
 * @version 1.3
 * @date 2025-10-01
 * 
 * @note This file should be included by all component drivers requiring GPIO access
 * @note Modify pin assignments here when hardware changes are needed
 * @note Always verify pin conflicts before adding new components
 */

// ===================================================================
// ST7789 240x240 TFT Display Pin Assignments (SPI Interface)
// ===================================================================
/**
 * @brief ST7789 SPI Clock Pin (SCK)
 * 
 * Provides the SPI clock signal for synchronizing data transmission to the display.
 * GPIO 18 is part of the ESP32's hardware SPI (VSPI) interface, enabling
 * high-speed communication with minimal CPU overhead and optimal signal integrity.
 * 
 * @note Maximum frequency: 40+ MHz depending on display module
 * @note Hardware SPI pin for best performance
 */
#define ST7789_SCK_PIN      18

/**
 * @brief ST7789 SPI Data Pin (MOSI/SDA)
 * 
 * Carries command and pixel data from ESP32 to the display controller.
 * This is a unidirectional signal as the ST7789 does not send data back.
 * GPIO 23 is the hardware VSPI MOSI pin for maximum transfer speed.
 * 
 * @note Unidirectional output (ESP32 to display only)
 * @note Hardware SPI pin for optimal performance
 */
#define ST7789_SDA_PIN      23

/**
 * @brief ST7789 Hardware Reset Pin (RST)
 * 
 * Provides hardware reset capability for the display controller. Used during
 * initialization to ensure the controller starts in a known, clean state.
 * Must be driven low for minimum 10ms, then high with 120ms recovery time.
 * 
 * @note Active low reset signal
 * @note Required for proper display initialization
 */
#define ST7789_RST_PIN      4

/**
 * @brief ST7789 Data/Command Selection Pin (DC)
 * 
 * Controls whether transmitted SPI data should be interpreted as commands (DC=LOW)
 * or pixel data (DC=HIGH). This signal must be precisely timed with SPI data.
 * Critical for proper display controller operation.
 * 
 * @note LOW = command mode, HIGH = data mode
 * @note Must be stable during SPI transmission
 */
#define ST7789_DC_PIN       2

// ===================================================================
// DHT11 Temperature and Humidity Sensor Pin Assignment
// ===================================================================
/**
 * @brief DHT11 Single-Wire Data Pin
 * 
 * Handles bidirectional communication with the DHT11 temperature and humidity
 * sensor using a proprietary single-wire protocol with microsecond timing.
 * Requires open-drain configuration with pull-up resistor for reliable operation.
 * 
 * Protocol: Custom single-wire with precise timing (18ms start + 40-bit data)
 * Configuration: Open-drain output with internal or external pull-up (4.7kΩ recommended)
 * 
 * @note Bidirectional communication (input/output switching required)
 * @note Microsecond-precision timing critical for data integrity
 * @note External 4.7kΩ pull-up recommended for long cables
 */
#define DHT11_DATA_PIN      22

// ===================================================================
// WiFi Status Indicator (Optional Visual Feedback)
// ===================================================================
/**
 * @brief WiFi Connection Status LED Pin
 * 
 * Optional LED to provide visual indication of WiFi connection status.
 * Can be used to show connection state, data transmission activity,
 * or system status without requiring the main display.
 * 
 * LED States:
 * - OFF: WiFi disconnected or disabled
 * - ON: WiFi connected and operational
 * - BLINKING: WiFi connection attempt in progress
 * 
 * @note GPIO 25 has DAC capability for PWM brightness control
 * @note External current-limiting resistor required (220Ω typical)
 * @note Optional feature - can be left unconnected
 */
#define WIFI_STATUS_LED_PIN 25

// ===================================================================
// Communication Bus Pin Assignments for Future Expansion
// ===================================================================

/**
 * @brief I2C Serial Data Line (SDA)
 * 
 * Default I2C data pin for connecting multiple I2C sensors and devices.
 * Supports sensors like BME280, RTC modules, OLED displays, and more.
 * Multiple devices can share the same I2C bus using different addresses.
 * 
 * @note Requires external pull-up resistors (4.7kΩ typical)
 * @note Default Wire library pin for ESP32
 * @note Supports up to 400kHz fast mode
 */
#define I2C_SDA_PIN         21

/**
 * @brief I2C Serial Clock Line (SCL)
 * 
 * I2C clock pin for synchronizing data transmission on the I2C bus.
 * Works with I2C_SDA_PIN to provide two-wire communication interface.
 * 
 * @note Alternative pin required if GPIO 22 conflicts with DHT11
 * @note Consider using GPIO 26 for SCL when DHT11 is on GPIO 22
 */
#define I2C_SCL_PIN         20

/**
 * @brief Additional SPI Chip Select Pins
 * 
 * For connecting multiple SPI devices that share the same SPI bus
 * but require individual chip select signals for device selection.
 * Useful for adding SPI sensors, SD cards, or other SPI peripherals.
 */
#define SPI_CS1_PIN         12  ///< First additional SPI chip select
#define SPI_CS2_PIN         13  ///< Second additional SPI chip select

// ===================================================================
// Analog Input Pin Assignments for Sensor Expansion
// ===================================================================

/**
 * @brief Primary Analog Input Pins (ADC1 Channel Group)
 * 
 * These pins provide high-resolution analog-to-digital conversion for
 * reading variable voltage sensors like potentiometers, light sensors,
 * pressure sensors, and other analog devices.
 * 
 * ADC1 Advantages:
 * - Not affected by WiFi operation (unlike ADC2)
 * - More stable readings during wireless communication
 * - Recommended for continuous sensor monitoring
 */
#define ANALOG_PIN_1        32  ///< ADC1_CH4 - General analog input
#define ANALOG_PIN_2        33  ///< ADC1_CH5 - General analog input

/**
 * @brief Input-Only Analog Pins (Higher ADC1 Channels)
 * 
 * These pins can only be used as inputs and lack internal pull-up/pull-down
 * resistors. They're excellent for analog sensors but require external
 * pull resistors for reliable digital input operation.
 * 
 * @note No internal pull resistors available
 * @note Excellent for analog sensors (photoresistors, thermistors, etc.)
 * @note External biasing required for digital applications
 */
#define ANALOG_PIN_3        34  ///< ADC1_CH6 - Input-only analog
#define ANALOG_PIN_4        35  ///< ADC1_CH7 - Input-only analog
#define ANALOG_PIN_5        36  ///< ADC1_CH0 - Input-only analog (VP)
#define ANALOG_PIN_6        39  ///< ADC1_CH3 - Input-only analog (VN)

// ===================================================================
// Digital I/O Pin Assignments for General Purpose Expansion
// ===================================================================

/**
 * @brief General Purpose Digital I/O Pins
 * 
 * These pins provide standard digital input/output capability for
 * connecting sensors, actuators, buttons, LEDs, and other digital devices.
 * All pins support both input and output modes with configurable pull resistors.
 */
#define DIGITAL_PIN_1       5   ///< General digital I/O (former HC-SR04 trigger)
#define DIGITAL_PIN_2       14  ///< General digital I/O
#define DIGITAL_PIN_3       15  ///< General digital I/O (MTDO - boot sensitive)
#define DIGITAL_PIN_4       16  ///< General digital I/O
#define DIGITAL_PIN_5       17  ///< General digital I/O
#define DIGITAL_PIN_6       19  ///< General digital I/O (former HC-SR04 echo)
#define DIGITAL_PIN_7       24  ///< General digital I/O
#define DIGITAL_PIN_8       25  ///< General digital I/O (also DAC1)
#define DIGITAL_PIN_9       26  ///< General digital I/O (also DAC2)
#define DIGITAL_PIN_10      27  ///< General digital I/O

// ===================================================================
// Pin Validation and Conflict Detection Macros
// ===================================================================

/**
 * @brief Check if a GPIO pin is currently assigned to a system component
 * 
 * This macro helps prevent accidental pin conflicts by identifying pins
 * that are already in use by existing hardware components.
 * 
 * @param pin GPIO pin number to check
 * @return true if pin is currently assigned, false if available
 * 
 * @note Use this macro before assigning new pins to avoid conflicts
 */
#define PIN_IS_USED(pin) ( \
    (pin) == ST7789_SCK_PIN || (pin) == ST7789_SDA_PIN || \
    (pin) == ST7789_RST_PIN || (pin) == ST7789_DC_PIN || \
    (pin) == DHT11_DATA_PIN || (pin) == WIFI_STATUS_LED_PIN \
)

/**
 * @brief Check if a GPIO pin is available for new assignments
 * 
 * Validates that a pin is not currently used and avoids system-reserved
 * pins that should not be used for general I/O purposes.
 * 
 * @param pin GPIO pin number to check
 * @return true if pin is available for assignment, false if reserved/used
 * 
 * @note Excludes boot pins (0,1,3) and flash pins (6-11)
 */
#define PIN_IS_AVAILABLE(pin) (!PIN_IS_USED(pin) && (pin) != 0 && (pin) != 1 && (pin) != 3 && \
    ((pin) < 6 || (pin) > 11))  // Exclude system reserved pins

/**
 * @brief Validate GPIO pin number is within ESP32 range
 * 
 * @param pin GPIO pin number to validate
 * @return true if pin number is valid (0-39), false otherwise
 */
#define IS_VALID_GPIO_PIN(pin) ((pin) >= 0 && (pin) <= 39)

/**
 * @brief Check if pin is reserved for internal flash memory
 * 
 * GPIO pins 6-11 are connected to the ESP32's internal flash memory
 * and must never be used for external connections.
 * 
 * @param pin GPIO pin number to check
 * @return true if pin is flash-reserved (DO NOT USE), false if safe
 * 
 * @warning Using flash pins will cause system malfunction or damage
 */
#define IS_FLASH_PIN(pin) ((pin) >= 6 && (pin) <= 11)

/**
 * @brief Check if pin is input-only (no output capability)
 * 
 * GPIO pins 34-39 can only be used as inputs and lack internal
 * pull-up or pull-down resistors. Suitable for analog inputs.
 * 
 * @param pin GPIO pin number to check
 * @return true if pin is input-only, false if bidirectional
 * 
 * @note Input-only pins require external pull resistors
 */
#define IS_INPUT_ONLY_PIN(pin) ((pin) >= 34 && (pin) <= 39)

// ===================================================================
// Suggested Pin Assignments for Common Sensor Expansions
// ===================================================================

/**
 * @brief Comprehensive Pin Assignment Guide for Popular Sensors
 * 
 * This section provides tested pin assignments for commonly requested
 * sensors and peripherals. These suggestions consider electrical
 * requirements, timing constraints, and pin conflicts.
 * 
 * ULTRASONIC DISTANCE SENSORS (HC-SR04, HC-SR05):
 * - Trigger Pin: GPIO 5 (digital output for 10μs pulse)
 * - Echo Pin: GPIO 19 (digital input for pulse measurement)
 * - VCC: 5V (use level shifter if needed)
 * - GND: Common ground
 * 
 * TEMPERATURE SENSORS:
 * - DS18B20 OneWire: GPIO 14 (single-wire protocol, needs pull-up)
 * - BME280 I2C: SDA=GPIO 21, SCL=GPIO 20 (temperature + humidity + pressure)
 * - Thermistor Analog: GPIO 32 or 33 (ADC1 channels)
 * 
 * MOTION AND PROXIMITY:
 * - PIR Motion Sensor: GPIO 15 (digital input with interrupt capability)
 * - Microwave Motion (RCWL-0516): GPIO 16 (digital input)
 * - Capacitive Touch: GPIO 4, 12, 13, 14, 15, 27, 32, 33 (touch-capable pins)
 * 
 * LIGHT AND OPTICAL:
 * - Photoresistor (LDR): GPIO 32, 33, 34, 35 (analog ADC inputs)
 * - TSL2561 Light Sensor I2C: SDA=GPIO 21, SCL=GPIO 20
 * - BH1750 Light Sensor I2C: SDA=GPIO 21, SCL=GPIO 20
 * 
 * ENVIRONMENTAL SENSORS:
 * - MQ-series Gas Sensors: GPIO 32, 33 (analog for gas concentration)
 * - Rain/Moisture Sensors: GPIO 34, 35 (analog inputs, no pull-up needed)
 * - Barometric Pressure BMP180: SDA=GPIO 21, SCL=GPIO 20
 * 
 * ACTUATORS AND OUTPUT:
 * - Servo Motors PWM: GPIO 16, 17 (PWM capable, good drive strength)
 * - LED Strip WS2812B: GPIO 17 (high-speed digital output)
 * - Buzzer/Speaker: GPIO 25 (DAC output for analog audio)
 * - Relay Control: GPIO 26, 27 (digital outputs with good drive strength)
 * - Stepper Motor Control: GPIO 12, 13, 14, 15 (4 pins for full step)
 * 
 * COMMUNICATION INTERFACES:
 * - GPS Module UART: TX=GPIO 17, RX=GPIO 16
 * - GSM Module UART: TX=GPIO 17, RX=GPIO 16
 * - Bluetooth Classic UART: TX=GPIO 17, RX=GPIO 16
 * - LoRa Module SPI: CS=GPIO 12, additional control pins available
 * 
 * USER INTERFACE:
 * - Push Buttons: GPIO 12, 13 (with internal pull-up)
 * - Rotary Encoder: GPIO 14 (CLK), GPIO 15 (DT), GPIO 16 (SW)
 * - Potentiometer: GPIO 33 (analog ADC input)
 * - Joystick: GPIO 32 (X-axis), GPIO 33 (Y-axis), GPIO 17 (button)
 * 
 * STORAGE AND MEMORY:
 * - SD Card SPI: CS=GPIO 12, use existing SPI pins (18, 23)
 * - EEPROM I2C: SDA=GPIO 21, SCL=GPIO 20
 * 
 * POWER MANAGEMENT:
 * - Battery Voltage Monitor: GPIO 35 (input-only, voltage divider)
 * - Power Control Relay: GPIO 26 (digital output)
 * 
 * SPECIAL PURPOSE:
 * - Watchdog Timer Output: GPIO 27 (external WDT reset)
 * - Status LEDs: GPIO 25 (DAC for brightness), GPIO 26, GPIO 27
 * - Test Points: GPIO 19, GPIO 24 (easy access for debugging)
 * 
 * PIN CONFLICT WARNINGS:
 * - Avoid GPIO 0, 2 for critical sensors (boot strapping pins)
 * - GPIO 12, 15 affect boot modes - use carefully
 * - ADC2 pins (25-27, 32-33) may be affected by WiFi operation
 * - Input-only pins (34-39) need external pull resistors
 * 
 * ELECTRICAL CONSIDERATIONS:
 * - 3.3V logic levels for all ESP32 GPIO pins
 * - Maximum 12mA source/sink current per pin
 * - Use level shifters for 5V devices
 * - Add current limiting resistors for LEDs
 * - Use pull-up resistors for I2C (4.7kΩ recommended)
 * - Consider EMI/noise for high-frequency signals
 */

#endif // PINOUT_H