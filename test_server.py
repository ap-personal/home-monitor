#!/usr/bin/env python3
"""
Simple HTTP Server to receive ESP32 sensor data
Run this script on your computer to receive and display sensor data.

Requirements:
- Python 3.6+
- No additional dependencies (uses built-in modules)

Usage:
1. Run: python3 test_server.py
2. Update wifi_config.h with this server's IP address
3. Flash and run your ESP32

Example URL: http://192.168.1.100:3000/api/sensor-data
"""

import json
import socketserver
from http.server import BaseHTTPRequestHandler
from datetime import datetime
import socket

class SensorDataHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path == '/api/sensor-data':
            # Read the request body
            content_length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(content_length)
            
            try:
                # Parse JSON data
                sensor_data = json.loads(post_data.decode('utf-8'))
                
                # Print received data with timestamp
                timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                print(f"\n[{timestamp}] Sensor Data Received:")
                print(f"  Device ID: {sensor_data.get('device_id', 'Unknown')}")
                print(f"  Temperature: {sensor_data.get('temperature', 'N/A')}°C")
                print(f"  Humidity: {sensor_data.get('humidity', 'N/A')}%")
                print(f"  WiFi Signal: {sensor_data.get('rssi', 'N/A')} dBm")
                print(f"  Timestamp: {sensor_data.get('timestamp', 'N/A')}")
                print("-" * 50)
                
                # Send success response
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.send_header('Access-Control-Allow-Origin', '*')
                self.end_headers()
                
                response = {
                    "status": "success",
                    "message": "Data received successfully",
                    "received_at": timestamp
                }
                self.wfile.write(json.dumps(response).encode())
                
            except json.JSONDecodeError:
                # Handle invalid JSON
                self.send_response(400)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                error_response = {"status": "error", "message": "Invalid JSON data"}
                self.wfile.write(json.dumps(error_response).encode())
                print(f"[ERROR] Invalid JSON received: {post_data}")
                
        else:
            # Handle unknown endpoints
            self.send_response(404)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            error_response = {"status": "error", "message": "Endpoint not found"}
            self.wfile.write(json.dumps(error_response).encode())
    
    def do_GET(self):
        if self.path == '/':
            # Simple status page
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            
            html = f"""
            <!DOCTYPE html>
            <html>
            <head>
                <title>ESP32 Sensor Data Server</title>
                <style>
                    body {{ font-family: Arial, sans-serif; margin: 40px; }}
                    .status {{ color: green; font-weight: bold; }}
                </style>
            </head>
            <body>
                <h1>ESP32 Sensor Data Server</h1>
                <p class="status">✅ Server is running and ready to receive data</p>
                <p><strong>Server IP:</strong> {get_local_ip()}</p>
                <p><strong>Port:</strong> 3000</p>
                <p><strong>Endpoint:</strong> POST /api/sensor-data</p>
                <h2>Configuration for ESP32:</h2>
                <pre>
#define HTTP_SERVER_URL "http://{get_local_ip()}:3000/api/sensor-data"
                </pre>
                <p>Update your wifi_config.h file with the URL above.</p>
            </body>
            </html>
            """
            self.wfile.write(html.encode())
        else:
            self.send_response(404)
            self.end_headers()
    
    def log_message(self, format, *args):
        # Suppress default request logging
        pass

def get_local_ip():
    """Get the local IP address of this machine"""
    try:
        # Connect to a remote server to get local IP
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.connect(("8.8.8.8", 80))
        local_ip = sock.getsockname()[0]
        sock.close()
        return local_ip
    except:
        return "localhost"

def main():
    port = 3000
    local_ip = get_local_ip()
    
    print("=" * 60)
    print("ESP32 Sensor Data Test Server")
    print("=" * 60)
    print(f"Server starting on {local_ip}:{port}")
    print(f"Endpoint: POST /api/sensor-data")
    print(f"Web interface: http://{local_ip}:{port}")
    print("\nUpdate your ESP32 wifi_config.h with:")
    print(f'#define HTTP_SERVER_URL "http://{local_ip}:{port}/api/sensor-data"')
    print("\nPress Ctrl+C to stop the server")
    print("=" * 60)
    
    try:
        with socketserver.TCPServer(("", port), SensorDataHandler) as httpd:
            httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n\nServer stopped by user")
    except Exception as e:
        print(f"\nError starting server: {e}")
        print("Make sure port 3000 is not already in use")

if __name__ == "__main__":
    main()