#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Server details
const char* serverAddress = "192.168.1.100"; // Replace with your computer's IP
const int serverPort = 3000;

// LED Matrix configuration
#define LED_PIN     6
#define MATRIX_WIDTH  30  // Number of LEDs in your strip
#define MATRIX_HEIGHT 1   // Single row
#define BRIGHTNESS  50

Adafruit_NeoPixel strip(MATRIX_WIDTH, LED_PIN, NEO_GRB + NEO_KHZ800);

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, serverPort);

// Price data
float currentPrice = 0;
float priceChange = 0;
float percentChange = 0;
String priceDirection = "up";

// Display variables
String displayText = "";
int scrollOffset = 0;
unsigned long lastScrollTime = 0;
unsigned long lastUpdateTime = 0;
const int SCROLL_SPEED = 100; // ms between shifts
const int UPDATE_INTERVAL = 5000; // 5 seconds

// Simple 3x5 font for numbers and symbols
const uint8_t font3x5[][3] = {
  {0x7, 0x5, 0x7}, // 0
  {0x2, 0x2, 0x2}, // 1
  {0x7, 0x3, 0x7}, // 2
  {0x7, 0x3, 0x7}, // 3
  {0x5, 0x7, 0x1}, // 4
  {0x7, 0x4, 0x7}, // 5
  {0x7, 0x7, 0x7}, // 6
  {0x7, 0x1, 0x1}, // 7
  {0x7, 0x7, 0x7}, // 8
  {0x7, 0x7, 0x1}, // 9
  {0x0, 0x2, 0x0}, // .
  {0x0, 0x7, 0x0}, // +
  {0x0, 0x1, 0x0}, // -
  {0x5, 0x2, 0x5}, // %
  {0x7, 0x4, 0x7}, // $
  {0x2, 0x5, 0x2}, // ▲ (up arrow)
  {0x2, 0x5, 0x2}, // ▼ (down arrow)
  {0x0, 0x0, 0x0}, // space
  {0x7, 0x5, 0x5}, // B
  {0x2, 0x2, 0x2}, // T
  {0x5, 0x5, 0x7}, // C
};

// Virtual display buffer
const int BUFFER_WIDTH = 200; // Wide enough for scrolling text
uint32_t displayBuffer[BUFFER_WIDTH];

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize

  Serial.println("BTC LED Sign Starting...");

  // Initialize LED strip
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  
  // Show startup animation
  startupAnimation();
  
  // Connect to WiFi
  connectToWiFi();
  
  // Initial price fetch
  fetchBTCPrice();
}

void loop() {
  // Update price data
  if (millis() - lastUpdateTime > UPDATE_INTERVAL) {
    fetchBTCPrice();
    lastUpdateTime = millis();
  }
  
  // Scroll the display
  if (millis() - lastScrollTime > SCROLL_SPEED) {
    scrollDisplay();
    lastScrollTime = millis();
  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  // Show connecting animation
  uint32_t blue = strip.Color(0, 0, 255);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    // Animated blue dots while connecting
    for (int i = 0; i < 3; i++) {
      strip.clear();
      strip.setPixelColor((attempts + i) % MATRIX_WIDTH, blue);
      strip.show();
      delay(100);
    }
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Success animation
    uint32_t green = strip.Color(0, 255, 0);
    for (int i = 0; i < MATRIX_WIDTH; i++) {
      strip.setPixelColor(i, green);
      strip.show();
      delay(30);
    }
    delay(500);
    strip.clear();
    strip.show();
  } else {
    Serial.println("\nWiFi connection failed!");
    // Error animation
    flashColor(strip.Color(255, 0, 0), 5);
  }
}

void fetchBTCPrice() {
  Serial.println("Fetching BTC price...");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return;
  }
  
  client.get("/api/btc-price");
  
  int statusCode = client.responseStatusCode();
  
  if (statusCode == 200) {
    String response = client.responseBody();
    
    // Parse JSON response
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      currentPrice = doc["price"].as<float>();
      priceChange = doc["change24h"].as<float>();
      percentChange = doc["changePercent24h"].as<float>();
      priceDirection = doc["direction"].as<String>();
      
      // Format display text with proper symbols
      String arrow = priceDirection == "up" ? "▲" : "▼";
      displayText = "BTC $" + formatPrice(currentPrice) + " " + arrow + " " + 
                    String(abs(percentChange), 1) + "%   ";
      
      Serial.println("Updated: " + displayText);
      
      // Update display buffer
      renderTextToBuffer();
    } else {
      Serial.println("JSON parse error");
    }
  } else {
    Serial.println("HTTP Error: " + String(statusCode));
  }
}

String formatPrice(float price) {
  // Format price with commas
  int wholePart = (int)price;
  int decimalPart = (int)((price - wholePart) * 100);
  
  String formatted = "";
  String whole = String(wholePart);
  
  // Add commas
  int digitCount = 0;
  for (int i = whole.length() - 1; i >= 0; i--) {
    if (digitCount > 0 && digitCount % 3 == 0) {
      formatted = "," + formatted;
    }
    formatted = whole[i] + formatted;
    digitCount++;
  }
  
  // Add decimal part
  formatted += ".";
  if (decimalPart < 10) formatted += "0";
  formatted += String(decimalPart);
  
  return formatted;
}

void renderTextToBuffer() {
  // Clear buffer
  for (int i = 0; i < BUFFER_WIDTH; i++) {
    displayBuffer[i] = 0;
  }
  
  // This is a simplified renderer - in practice you'd want a proper font system
  // For now, we'll create colored blocks to represent the price movement
  uint32_t textColor = priceDirection == "up" ? strip.Color(0, 255, 0) : strip.Color(255, 0, 0);
  
  // Create a pattern in the buffer
  int textLength = displayText.length() * 4; // Approximate width
  for (int i = 0; i < textLength && i < BUFFER_WIDTH; i++) {
    if (i % 5 < 3) { // Simple pattern to simulate text
      displayBuffer[i] = textColor;
    }
  }
}

void scrollDisplay() {
  strip.clear();
  
  // Copy from buffer to LED strip with offset
  for (int i = 0; i < MATRIX_WIDTH; i++) {
    int bufferIndex = (i + scrollOffset) % BUFFER_WIDTH;
    if (displayBuffer[bufferIndex] != 0) {
      strip.setPixelColor(i, displayBuffer[bufferIndex]);
    }
  }
  
  strip.show();
  
  // Update scroll position
  scrollOffset++;
  if (scrollOffset >= BUFFER_WIDTH) {
    scrollOffset = 0;
  }
}

void startupAnimation() {
  // Rainbow startup effect
  for (int j = 0; j < 256; j++) {
    for (int i = 0; i < MATRIX_WIDTH; i++) {
      int pixelHue = (i * 65536L / MATRIX_WIDTH) + (j * 256);
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show();
    delay(10);
  }
  
  // Fade out
  for (int brightness = BRIGHTNESS; brightness >= 0; brightness--) {
    strip.setBrightness(brightness);
    strip.show();
    delay(10);
  }
  
  strip.clear();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

void flashColor(uint32_t color, int times) {
  for (int i = 0; i < times; i++) {
    for (int j = 0; j < MATRIX_WIDTH; j++) {
      strip.setPixelColor(j, color);
    }
    strip.show();
    delay(200);
    strip.clear();
    strip.show();
    delay(200);
  }
}