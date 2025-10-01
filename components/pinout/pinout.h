#ifndef PINOUT_H
#define PINOUT_H

/**
 * @file pinout.h
 * @brief Centralized GPIO Pin Configuration for ESP32 Sensor System
 * 
 * This header contains all GPIO pin assignments for the entire system.
 * Organizing pins here prevents conflicts and makes it easy to see 
 * the complete pin usage at a glance.
 * 
 * ESP32 Pin Usage Map:
 * - GPIO 0:  Boot/Flash (system reserved)
 * - GPIO 1:  UART TX (system reserved)
 * - GPIO 2:  ST7789 DC (Data/Command)
 * - GPIO 3:  UART RX (system reserved)
 * - GPIO 4:  ST7789 RST (Reset)
 * - GPIO 5:  Available
 * - GPIO 6-11: Flash memory (system reserved)
 * - GPIO 12: Available
 * - GPIO 13: Available
 * - GPIO 14: Available
 * - GPIO 15: Available
 * - GPIO 16: Available
 * - GPIO 17: Available
 * - GPIO 18: ST7789 SCK (SPI Clock)
 * - GPIO 19: Available
 * - GPIO 20: Available
 * - GPIO 21: Available (I2C SDA)
 * - GPIO 22: DHT11 Data
 * - GPIO 23: ST7789 SDA (SPI MOSI)
 * - GPIO 24: Available
 * - GPIO 25: Available (DAC1)
 * - GPIO 26: Available (DAC2)
 * - GPIO 27: Available
 * - GPIO 32: Available (ADC1_CH4)
 * - GPIO 33: Available (ADC1_CH5)
 * - GPIO 34: Available (ADC1_CH6, input only)
 * - GPIO 35: Available (ADC1_CH7, input only)
 * - GPIO 36: Available (ADC1_CH0, input only)
 * - GPIO 39: Available (ADC1_CH3, input only)
 */

// ===================================================================
// ST7789 240x240 TFT Display (SPI Interface)
// ===================================================================
#define ST7789_SCK_PIN      18  // SPI clock pin
#define ST7789_SDA_PIN      23  // SPI data pin (MOSI)
#define ST7789_RST_PIN      4   // Hardware reset pin (active low)
#define ST7789_DC_PIN       2   // Data/Command select pin

// ===================================================================
// DHT11 Temperature and Humidity Sensor
// ===================================================================
#define DHT11_DATA_PIN      22  // Single-wire data pin

// ===================================================================
// Available GPIO Pins for Future Sensors
// ===================================================================
// Available GPIO pins for expansion:
// GPIO 5, 12, 13, 14, 15, 16, 17, 19, 20, 21, 24, 25, 26, 27, 32, 33

// I2C Bus (if needed for future sensors)
#define I2C_SDA_PIN         21  // I2C Data (available)
#define I2C_SCL_PIN         20  // I2C Clock (available)

// SPI Bus Extension (if more SPI devices needed)
#define SPI_CS1_PIN         12  // Additional chip select (available)
#define SPI_CS2_PIN         13  // Additional chip select (available)

// Analog Input Pins (for analog sensors)
#define ANALOG_PIN_1        32  // ADC1_CH4 (available)
#define ANALOG_PIN_2        33  // ADC1_CH5 (available)
#define ANALOG_PIN_3        34  // ADC1_CH6 (input only, available)
#define ANALOG_PIN_4        35  // ADC1_CH7 (input only, available)
#define ANALOG_PIN_5        36  // ADC1_CH0 (input only, available)
#define ANALOG_PIN_6        39  // ADC1_CH3 (input only, available)

// Digital I/O Pins (for digital sensors/actuators)
#define DIGITAL_PIN_1       5   // Available (former HC-SR04 trigger)
#define DIGITAL_PIN_2       14  // Available
#define DIGITAL_PIN_3       15  // Available
#define DIGITAL_PIN_4       16  // Available
#define DIGITAL_PIN_5       17  // Available
#define DIGITAL_PIN_6       19  // Available (former HC-SR04 echo)
#define DIGITAL_PIN_7       24  // Available
#define DIGITAL_PIN_8       25  // Available (also DAC1)
#define DIGITAL_PIN_9       26  // Available (also DAC2)
#define DIGITAL_PIN_10      27  // Available

// ===================================================================
// Pin Validation Macros
// ===================================================================
// Use these to check for pin conflicts during development
#define PIN_IS_USED(pin) ( \
    (pin) == ST7789_SCK_PIN || (pin) == ST7789_SDA_PIN || \
    (pin) == ST7789_RST_PIN || (pin) == ST7789_DC_PIN || \
    (pin) == DHT11_DATA_PIN \
)

#define PIN_IS_AVAILABLE(pin) (!PIN_IS_USED(pin) && (pin) != 0 && (pin) != 1 && (pin) != 3 && \
    (pin) < 6 || (pin) > 11)  // Exclude system reserved pins

// ===================================================================
// Future Sensor Suggestions
// ===================================================================
/*
 * Suggested pin assignments for common sensors:
 * 
 * HC-SR04 Ultrasonic Sensor: GPIO 5 (trigger), GPIO 19 (echo)
 * DS18B20 Temperature Sensor (OneWire): GPIO 14
 * BME280 Environmental Sensor (I2C): GPIO 21 (SDA), GPIO 20 (SCL)
 * PIR Motion Sensor: GPIO 15
 * Photoresistor (LDR): GPIO 32 (ADC)
 * Servo Motor PWM: GPIO 16
 * LED Strip (WS2812B): GPIO 17
 * Buzzer/Speaker: GPIO 25 (DAC)
 * Potentiometer: GPIO 33 (ADC)
 * Push Buttons: GPIO 12, GPIO 13
 * Relay Control: GPIO 26, GPIO 27
 */

#endif // PINOUT_H