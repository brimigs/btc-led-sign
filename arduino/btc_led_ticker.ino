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

// LED configuration
#define LED_PIN     6
#define LED_COUNT   30
#define BRIGHTNESS  50

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, serverPort);

// Price data
float currentPrice = 0;
float priceChange = 0;
float percentChange = 0;
String priceDirection = "up";

// Display variables
String displayText = "";
int scrollPosition = 0;
unsigned long lastScrollTime = 0;
unsigned long lastUpdateTime = 0;
const int SCROLL_DELAY = 150;
const int UPDATE_INTERVAL = 5000; // 5 seconds

// Colors
uint32_t colorGreen = strip.Color(0, 255, 0);
uint32_t colorRed = strip.Color(255, 0, 0);
uint32_t colorWhite = strip.Color(255, 255, 255);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port
  }

  // Initialize LED strip
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  
  // Show connection status
  setAllLEDs(strip.Color(0, 0, 255)); // Blue for connecting
  
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
  if (millis() - lastScrollTime > SCROLL_DELAY) {
    updateDisplay();
    lastScrollTime = millis();
  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  
  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, password);
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Show success
  flashLEDs(colorGreen, 3);
}

void fetchBTCPrice() {
  Serial.println("Fetching BTC price...");
  
  client.get("/api/btc-price");
  
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  
  if (statusCode == 200) {
    // Parse JSON response
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      currentPrice = doc["price"].as<float>();
      priceChange = doc["change24h"].as<float>();
      percentChange = doc["changePercent24h"].as<float>();
      priceDirection = doc["direction"].as<String>();
      
      // Format display text
      char changeSymbol = priceDirection == "up" ? '+' : '-';
      displayText = "BTC $" + String(currentPrice, 2) + " " + 
                    String(changeSymbol) + String(abs(percentChange), 1) + "%";
      
      Serial.println("Price updated: " + displayText);
    }
  } else {
    Serial.println("Error fetching price: " + String(statusCode));
  }
}

void updateDisplay() {
  // Clear strip
  strip.clear();
  
  // Determine color based on price direction
  uint32_t textColor = priceDirection == "up" ? colorGreen : colorRed;
  
  // Simple scrolling text simulation
  // For a real implementation, you'd want a proper font system
  // This is a simplified version that just moves colored blocks
  
  int textLength = displayText.length() * 6; // Approximate pixel width
  
  for (int i = 0; i < LED_COUNT; i++) {
    int pixelPosition = (i + scrollPosition) % (textLength + LED_COUNT);
    
    // Create a simple pattern to represent text
    if (pixelPosition < textLength && pixelPosition % 6 < 4) {
      strip.setPixelColor(i, textColor);
    }
  }
  
  strip.show();
  
  // Update scroll position
  scrollPosition++;
  if (scrollPosition >= textLength + LED_COUNT) {
    scrollPosition = 0;
  }
}

void setAllLEDs(uint32_t color) {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void flashLEDs(uint32_t color, int times) {
  for (int i = 0; i < times; i++) {
    setAllLEDs(color);
    delay(200);
    setAllLEDs(0);
    delay(200);
  }
}