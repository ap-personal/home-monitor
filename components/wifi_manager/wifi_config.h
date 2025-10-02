/**
 * @file wifi_config.h
 * @brief WiFi Configuration Settings
 * 
 * Edit this file to configure your WiFi credentials and server settings.
 * This file should be customized for your specific network and server setup.
 */

#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

// ===================================================================
// WiFi Network Configuration
// ===================================================================
// IMPORTANT: Replace these with your actual WiFi credentials
#define WIFI_SSID           "TempRouter"
#define WIFI_PASSWORD       "QPWO0192"

// Advanced WiFi Settings
#define WIFI_RETRY_COUNT    5           // Number of connection retry attempts
#define WIFI_TIMEOUT_MS     10000       // Connection timeout in milliseconds

// ===================================================================
// HTTP Server Configuration
// ===================================================================
// IMPORTANT: Replace with your actual server URL
// Examples:
// - Local server: "http://192.168.1.100:3000/api/sensor-data"
// - Cloud service: "https://your-domain.com/api/sensors"
// - Webhook URL: "https://hooks.zapier.com/hooks/catch/12345/abcde/"
#define HTTP_SERVER_URL     "http://192.168.0.246:3000/api/sensor-data"

// HTTP Settings
#define HTTP_TIMEOUT_MS     10000       // HTTP request timeout
#define HTTP_BUFFER_SIZE    1024        // HTTP buffer size

// ===================================================================
// Data Transmission Settings
// ===================================================================
#define DEVICE_ID           "ESP32_SENSOR_01"   // Unique device identifier
#define DATA_SEND_INTERVAL  20                  // Send data every N sensor readings (20 = 1 minute)

// ===================================================================
// Example Server Implementations
// ===================================================================

/*
 * OPTION 1: Simple Node.js Express Server
 * 
 * const express = require('express');
 * const app = express();
 * 
 * app.use(express.json());
 * 
 * app.post('/api/sensor-data', (req, res) => {
 *     console.log('Received data:', req.body);
 *     // Save to database, forward to other services, etc.
 *     res.json({ status: 'success', message: 'Data received' });
 * });
 * 
 * app.listen(3000, () => {
 *     console.log('Server running on port 3000');
 * });
 */

/*
 * OPTION 2: Python Flask Server
 * 
 * from flask import Flask, request, jsonify
 * 
 * app = Flask(__name__)
 * 
 * @app.route('/api/sensor-data', methods=['POST'])
 * def receive_data():
 *     data = request.get_json()
 *     print(f"Received: {data}")
 *     # Process data here
 *     return jsonify({"status": "success"})
 * 
 * if __name__ == '__main__':
 *     app.run(host='0.0.0.0', port=3000)
 */

/*
 * OPTION 3: Cloud Services
 * 
 * - ThingSpeak: "https://api.thingspeak.com/update.json"
 * - Adafruit IO: "https://io.adafruit.com/api/v2/your-username/feeds/temperature/data"
 * - Firebase: "https://your-project.firebaseio.com/sensors.json"
 * - AWS IoT: Use MQTT instead of HTTP
 */

/*
 * JSON Data Format Sent:
 * {
 *   "device_id": "ESP32_SENSOR_01",
 *   "timestamp": 1696118400,
 *   "temperature": 23.5,
 *   "humidity": 65.2,
 *   "rssi": -45
 * }
 */

#endif // WIFI_CONFIG_H