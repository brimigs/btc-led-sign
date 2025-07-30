# Building a Bitcoin Price LED Display: Complete Guide

## Overview

This guide walks through building a real-time Bitcoin price display using:
- **Solana/Pyth Network** for accurate price data
- **Node.js server** as a bridge between blockchain and hardware
- **Arduino Nano 33 IoT** to control the LED display
- **WS2812B LED strip** for the visual display

## Architecture

```
Pyth Oracle (Solana) → Node.js Server → Arduino (WiFi) → LED Strip
```

## Part 1: Setting Up the Node.js Backend

### Step 1: Initialize the Project

```bash
mkdir btc-led-sign
cd btc-led-sign
npm init -y
```

### Step 2: Install Dependencies

```bash
npm install @pythnetwork/hermes-client express cors dotenv
```

### Step 3: Create the Server (server/index.js)

```javascript
const express = require('express');
const cors = require('cors');
const { HermesClient } = require('@pythnetwork/hermes-client');

const app = express();
const PORT = process.env.PORT || 3000;

// Enable CORS for Arduino requests
app.use(cors());

// Initialize Pyth client
const hermesClient = new HermesClient('https://hermes.pyth.network');

// Bitcoin price feed ID on Pyth
const BTC_USD_PRICE_ID = '0xe62df6c8b4a85fe1a67db44dc12de5db330f7ac66b72dc658afedf0f4a415b43';
```

### Step 4: Implement Price Fetching

```javascript
let priceData = {
  price: 0,
  change24h: 0,
  changePercent24h: 0,
  lastUpdate: null
};

async function fetchBTCPrice() {
  try {
    const priceFeeds = await hermesClient.getLatestPriceFeeds([BTC_USD_PRICE_ID]);
    
    if (priceFeeds && priceFeeds.length > 0) {
      const btcFeed = priceFeeds[0];
      // Convert price from Pyth format
      const currentPrice = Number(btcFeed.price.price) * Math.pow(10, btcFeed.price.expo);
      
      // Update price data
      priceData.price = currentPrice;
      priceData.lastUpdate = new Date().toISOString();
      
      console.log(`BTC Price: $${currentPrice.toFixed(2)}`);
    }
  } catch (error) {
    console.error('Error fetching price:', error);
  }
}

// Fetch price every 5 seconds
setInterval(fetchBTCPrice, 5000);
fetchBTCPrice(); // Initial fetch
```

### Step 5: Create API Endpoint

```javascript
app.get('/api/btc-price', (req, res) => {
  res.json({
    price: priceData.price.toFixed(2),
    change24h: priceData.change24h.toFixed(2),
    changePercent24h: priceData.changePercent24h.toFixed(2),
    direction: priceData.change24h >= 0 ? 'up' : 'down',
    lastUpdate: priceData.lastUpdate
  });
});

app.listen(PORT, () => {
  console.log(`Server running on http://localhost:${PORT}`);
});
```

## Part 2: Arduino LED Controller

### Step 1: Install Arduino Libraries

In Arduino IDE, install:
- WiFiNINA (for Nano 33 IoT)
- ArduinoHttpClient
- Adafruit NeoPixel
- ArduinoJson

### Step 2: Basic Arduino Setup

```cpp
#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

// Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverAddress = "192.168.1.100"; // Your computer's IP
const int serverPort = 3000;

// LED setup
#define LED_PIN     6
#define LED_COUNT   30
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
```

### Step 3: WiFi Connection

```cpp
void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("\nConnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}
```

### Step 4: Fetching Price Data

```cpp
WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, serverPort);

void fetchBTCPrice() {
  client.get("/api/btc-price");
  
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  
  if (statusCode == 200) {
    // Parse JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      float price = doc["price"];
      String direction = doc["direction"];
      float percentChange = doc["changePercent24h"];
      
      // Update display
      updateLEDDisplay(price, direction, percentChange);
    }
  }
}
```

### Step 5: LED Display Logic

```cpp
void updateLEDDisplay(float price, String direction, float change) {
  // Clear previous display
  strip.clear();
  
  // Set color based on price direction
  uint32_t color;
  if (direction == "up") {
    color = strip.Color(0, 255, 0); // Green
  } else {
    color = strip.Color(255, 0, 0); // Red
  }
  
  // Simple visualization: fill LEDs proportionally
  int ledsToLight = map(abs(change), 0, 10, 1, LED_COUNT);
  
  for (int i = 0; i < ledsToLight; i++) {
    strip.setPixelColor(i, color);
  }
  
  strip.show();
}
```

## Part 3: Advanced Features

### Scrolling Text Display

For a true ticker display, implement a text scrolling system:

```cpp
class ScrollingText {
private:
  String text;
  int position;
  uint32_t color;
  
public:
  void setText(String newText, uint32_t newColor) {
    text = newText;
    color = newColor;
    position = LED_COUNT; // Start off-screen
  }
  
  void update() {
    strip.clear();
    
    // Render text at current position
    // (Implement character rendering here)
    
    position--;
    if (position < -text.length() * 6) {
      position = LED_COUNT;
    }
    
    strip.show();
  }
};
```

### 24-Hour Price Tracking

Add historical price tracking to calculate real 24h changes:

```javascript
// In server code
let priceHistory = [];
const HISTORY_DURATION = 24 * 60 * 60 * 1000; // 24 hours

function updatePriceHistory(price) {
  const now = Date.now();
  priceHistory.push({ price, timestamp: now });
  
  // Remove old entries
  priceHistory = priceHistory.filter(
    entry => now - entry.timestamp < HISTORY_DURATION
  );
  
  // Calculate 24h change
  if (priceHistory.length > 0) {
    const oldestPrice = priceHistory[0].price;
    priceData.change24h = price - oldestPrice;
    priceData.changePercent24h = ((price - oldestPrice) / oldestPrice) * 100;
  }
}
```

### Error Handling and Reconnection

```cpp
void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  
  // Fetch price with error handling
  if (millis() - lastUpdateTime > UPDATE_INTERVAL) {
    if (!fetchBTCPrice()) {
      // Show error pattern on LEDs
      showErrorPattern();
    }
    lastUpdateTime = millis();
  }
  
  // Update display
  scrollingText.update();
  delay(SCROLL_SPEED);
}
```

## Part 4: Hardware Assembly

### Wiring Diagram

```
Arduino Nano 33 IoT:
- Pin 6 → 470Ω resistor → Level Shifter Input
- 3.3V → Level Shifter VCC (low side)
- GND → Common Ground

Level Shifter:
- Output → LED Strip Data In
- 5V → Level Shifter VCC (high side)

LED Strip:
- Data → From Level Shifter
- 5V → External Power Supply (+)
- GND → External Power Supply (-)

Power Supply:
- 1000µF capacitor across terminals
- Common ground with Arduino
```

### Safety Considerations

1. **Power Requirements**: 30 LEDs × 60mA = 1.8A maximum. Use a 5V 3A+ supply.
2. **Level Shifting**: Arduino outputs 3.3V, LEDs need 5V data signal.
3. **Decoupling**: Capacitor prevents voltage spikes during color changes.

## Part 5: Testing and Debugging

### Server Testing

```bash
# Test the API endpoint
curl http://localhost:3000/api/btc-price

# Expected response:
{
  "price": "45123.45",
  "change24h": "1234.56",
  "changePercent24h": "2.81",
  "direction": "up",
  "lastUpdate": "2024-01-15T10:30:00Z"
}
```

### Arduino Serial Monitor

Add debug output:

```cpp
void setup() {
  Serial.begin(115200);
  Serial.println("BTC LED Sign Starting...");
  
  // ... rest of setup
}

void fetchBTCPrice() {
  Serial.print("Fetching price... ");
  // ... fetch logic
  Serial.println("Price: $" + String(price));
}
```

### Common Issues

1. **WiFi Won't Connect**
   - Check credentials
   - Ensure 2.4GHz network (not 5GHz)
   - Verify router allows IoT devices

2. **LEDs Not Lighting**
   - Check power connections
   - Verify data pin wiring
   - Test with simple example first

3. **Price Not Updating**
   - Check server is running
   - Verify IP address in Arduino code
   - Check firewall settings

## Part 6: Enhancements

### Multiple Cryptocurrencies

```javascript
const PRICE_IDS = {
  BTC: '0xe62df6c8b4a85fe1a67db44dc12de5db330f7ac66b72dc658afedf0f4a415b43',
  ETH: '0xff61491a931112ddf1bd8147cd1b641375f79f5825126d665480874634fd0ace',
  SOL: '0xef0d8b6fda2ceba41da15d4095d1da392a0d2f8ed0c6c7bc0f4cfac8c280b56d'
};

// Rotate through different assets
let currentAsset = 0;
const assets = ['BTC', 'ETH', 'SOL'];
```

### Web Dashboard

```javascript
// Add web dashboard endpoint
app.get('/', (req, res) => {
  res.send(`
    <html>
      <head>
        <title>BTC LED Sign Dashboard</title>
        <script>
          setInterval(() => {
            fetch('/api/btc-price')
              .then(r => r.json())
              .then(data => {
                document.getElementById('price').textContent = '$' + data.price;
                document.getElementById('change').textContent = data.changePercent24h + '%';
              });
          }, 1000);
        </script>
      </head>
      <body>
        <h1>Bitcoin Price</h1>
        <div id="price">Loading...</div>
        <div id="change">Loading...</div>
      </body>
    </html>
  `);
});
```

### Price Alerts

```cpp
float alertThresholds[] = {40000, 45000, 50000};

void checkPriceAlerts(float price) {
  for (float threshold : alertThresholds) {
    if (lastPrice < threshold && price >= threshold) {
      // Price crossed above threshold
      celebrationAnimation();
    } else if (lastPrice > threshold && price <= threshold) {
      // Price crossed below threshold
      warningAnimation();
    }
  }
  lastPrice = price;
}
```

## Conclusion

This guide covers building a complete Bitcoin price LED display system. The modular architecture makes it easy to:
- Switch between different price oracles
- Add new cryptocurrencies
- Implement custom animations
- Scale to larger LED displays

Key principles:
- **Separation of concerns**: Server handles blockchain, Arduino handles display
- **Reliability**: Error handling and automatic reconnection
- **Efficiency**: Minimal API calls and power usage
- **Extensibility**: Easy to add features and customize

Happy building! 🚀