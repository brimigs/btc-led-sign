# BTC LED Sign Setup Guide

## Overview
This project displays real-time Bitcoin prices on an LED strip using:
- Node.js server that fetches BTC prices from Pyth Network
- Arduino Nano 33 IoT that displays the price on WS2812B LEDs

## Server Setup

1. **Install dependencies:**
   ```bash
   npm install
   ```

2. **Find your computer's IP address:**
   - Mac: `ifconfig | grep inet`
   - Windows: `ipconfig`
   - Linux: `ip addr show`
   
   Look for your local IP (usually starts with 192.168.x.x)

3. **Start the server:**
   ```bash
   npm start
   ```
   
   You should see:
   ```
   BTC LED Sign Server running on http://localhost:3000
   Arduino endpoint: http://localhost:3000/api/btc-price
   ```

4. **Test the API:**
   Open http://localhost:3000/api/btc-price in your browser

## Arduino Setup

1. **Install Arduino IDE Libraries:**
   - WiFiNINA (for Arduino Nano 33 IoT)
   - ArduinoHttpClient
   - Adafruit NeoPixel
   - ArduinoJson

2. **Update the Arduino code:**
   - Open `arduino/btc_led_ticker.ino`
   - Change these lines:
     ```cpp
     const char* ssid = "YOUR_WIFI_SSID";
     const char* password = "YOUR_WIFI_PASSWORD";
     const char* serverAddress = "192.168.1.100"; // Your computer's IP
     ```

3. **Hardware Connections:**
   - LED Strip Data → Arduino Pin 6 (through 74AHCT125 level shifter)
   - LED Strip 5V → External 5V Power Supply
   - LED Strip GND → Power Supply GND + Arduino GND
   - 470Ω resistor in series with data line
   - 1000µF capacitor across power supply terminals

4. **Upload the code to Arduino**

## Display Format
The LED sign shows:
- Price in USD (e.g., "$89,234.56")
- Direction arrow (▲ for up, ▼ for down)
- 24-hour percentage change
- Green color for positive movement
- Red color for negative movement

## Troubleshooting

- **Server not accessible:** Check firewall settings
- **Arduino can't connect:** Verify WiFi credentials and server IP
- **LEDs not lighting:** Check power connections and data pin
- **Price not updating:** Check server console for errors

## Moving to Raspberry Pi

When ready to run 24/7:
1. Copy entire project folder to Pi
2. Install Node.js: `sudo apt install nodejs npm`
3. Run `npm install` on Pi
4. Update Arduino's `serverAddress` to Pi's IP
5. Set up as systemd service for auto-start