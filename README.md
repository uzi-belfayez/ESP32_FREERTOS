# CO₂, Temperature, and Humidity Monitoring System with Presence Detection

## Overview

This project implements an embedded system for monitoring CO₂ levels, temperature, and humidity, with presence detection to activate the display. The system uses two ESP32 microcontrollers: one (ESP32 DevKit) handles CO₂ measurement and presence detection using the ESP-IDF framework, while the other (ESP32-WROOM with OLED) manages temperature, humidity, and display using the Arduino framework. The two ESP32s communicate via UART, and a web server provides real-time data visualization through WebSocket.

### Features
- Measures CO₂ levels (MH-Z19B sensor), temperature, and humidity (DHT22 sensor).
- Detects presence using an HC-SR04 ultrasonic sensor (activates display when an object is within 50 cm).
- Displays data on an OLED screen (128x64 pixels) when presence is detected.
- Hosts a web server for real-time data visualization via WebSocket.
- Implements a multiple producers - single consumer model for data handling.

## Hardware Requirements

- **ESP32 DevKit**: For CO₂ measurement and presence detection.
- **ESP32-WROOM with OLED**: For temperature/humidity measurement and display.
- **Sensors**:
  - MH-Z19B (CO₂ sensor, PWM mode).
  - HC-SR04 (ultrasonic sensor for presence detection).
  - DHT22 (temperature and humidity sensor).
- **OLED Display**: 128x64 pixels, I2C interface.
- **Cables and Breadboard**: For connections.
- **Power Supply**: 5V for sensors, 3.3V for ESP32 and OLED (ensure proper voltage levels).

### Wiring
#### ESP32 DevKit (CO₂ and Presence Detection)
- **HC-SR04**:
  - TRIGGER: GPIO 33
  - ECHO: GPIO 32
  - VCC: 5V, GND: Common ground
- **MH-Z19B**:
  - PWM: GPIO 4
  - VCC: 5V, GND: Common ground
- **UART to ESP32-WROOM**:
  - TX: GPIO 17
  - RX: GPIO 16

#### ESP32-WROOM (Temperature, Humidity, and Display)
- **DHT22**:
  - Data: GPIO 18
  - VCC: 3.3V or 5V, GND: Common ground
  - Pull-up resistor: 4.7kΩ between VCC and Data
- **OLED (I2C)**:
  - SDA: GPIO 5
  - SCL: GPIO 4
  - VCC: 3.3V, GND: Common ground
- **UART to ESP32 DevKit**:
  - TX: GPIO 17
  - RX: GPIO 16

#### Between ESP32s
- ESP32 DevKit TX (GPIO 17) → ESP32-WROOM RX (GPIO 16)
- ESP32 DevKit RX (GPIO 16) → ESP32-WROOM TX (GPIO 17)
- Connect GND of both ESP32s for a common ground.

## Software Requirements

- **ESP-IDF**: For the ESP32 DevKit (CO₂ and presence detection).
- **Arduino IDE** or **PlatformIO**: For the ESP32-WROOM (temperature, humidity, and display).
- **Libraries** (for Arduino):
  - `DHTesp` (for DHT22)
  - `SSD1306Wire` (for OLED display)
  - `ESPAsyncWebServer`, `AsyncTCP`, `ArduinoJson` (for web server and WebSocket)
  - `SPIFFS` (for hosting the web page)
- **Tools**:
  - PlatformIO for building and uploading the SPIFFS filesystem.
  - A serial monitor (e.g., Arduino IDE Serial Monitor) for debugging.

## Setup Instructions

1. **Verify Wiring**:
   - Ensure all connections are correct, with particular attention to the common ground between the two ESP32s for stable UART communication.

2. **Flash the ESP32 DevKit**:
   - Open the ESP-IDF project in your development environment.
   - Build and flash the program to the ESP32 DevKit.
   - Disconnect the power after flashing.

3. **Flash the ESP32-WROOM**:
   - Open the Arduino project in PlatformIO or Arduino IDE.
   - Build and flash the program to the ESP32-WROOM.

4. **Configure SPIFFS for the Web Page**:
   - Place the `index.html` file in the `data` folder of your PlatformIO project.
   - Run the following commands to build and upload the SPIFFS filesystem:
     ```
     pio run --target buildfs
     pio run --target uploadfs
     ```

5. **Power the ESP32s**:
   - Connect both ESP32s to a power source.

6. **Restart the ESP32-WROOM**:
   - Press the `EN` button on the ESP32-WROOM to restart it.

7. **Connect to Wi-Fi**:
   - Wait for the ESP32-WROOM to connect to the Wi-Fi network (ensure the SSID and password in the code match your network).
   - Check the serial monitor for the IP address of the ESP32-WROOM.

8. **System Ready**:
   - Once connected, the system is operational. Access the web interface using the IP address in a browser.

## Usage

- **Presence Detection**: Place an object within 50 cm of the HC-SR04 sensor to activate the OLED display, which will show CO₂, temperature, and humidity readings.
- **Local Display**: The OLED screen on the ESP32-WROOM displays the sensor data when presence is detected, turning off after 3 seconds of no detection.
- **Web Interface**: Open a browser and navigate to the ESP32-WROOM’s IP address to view real-time data (temperature, humidity, CO₂, and presence status) via the web page.

## Testing and Results

### Test Protocol
- **Presence Detection**: Objects were placed at distances from 10 cm to 100 cm to verify detection within 50 cm and OLED activation.
- **Sensor Measurements**: CO₂, temperature, and humidity were measured in a controlled environment and compared to reference devices.
- **Web Server**: The web interface was accessed on multiple devices to test real-time data updates via WebSocket.

### Results
- **Presence Detection**: Reliable within 50 cm (±2 cm accuracy), with occasional errors on reflective surfaces.
- **Sensor Accuracy**:
  - DHT22: ±0.5°C for temperature, ±2% for humidity.
  - MH-Z19B: ±50 ppm for CO₂ in the range of 400–1500 ppm.
- **Web Server**: Real-time updates every 1 second, with an average latency of 200 ms. Stable on a strong Wi-Fi connection.

### Known Issues
- Reflective surfaces may cause HC-SR04 errors (solution: add software filtering for outliers).
- Wi-Fi disconnections on unstable networks (solution: implement automatic reconnection).

## Contributing

Contributions are welcome! Please fork the repository, make your changes, and submit a pull request. For issues or feature requests, open an issue on the GitHub repository.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.