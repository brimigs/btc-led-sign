const express = require('express');
const cors = require('cors');
const { HermesClient } = require('@pythnetwork/hermes-client');
require('dotenv').config();

const app = express();
const PORT = process.env.PORT || 3000;

app.use(cors());
app.use(express.json());

const hermesClient = new HermesClient('https://hermes.pyth.network');

const BTC_USD_PRICE_ID = '0xe62df6c8b4a85fe1a67db44dc12de5db330f7ac66b72dc658afedf0f4a415b43';

let priceData = {
  price: 0,
  change24h: 0,
  changePercent24h: 0,
  lastUpdate: null,
  confidence: 0
};

let price24hAgo = null;
let priceHistory = [];
const HISTORY_DURATION = 24 * 60 * 60 * 1000; // 24 hours in ms

async function fetchBTCPrice() {
  try {
    const priceFeeds = await hermesClient.getLatestPriceFeeds([BTC_USD_PRICE_ID]);
    
    if (priceFeeds && priceFeeds.length > 0) {
      const btcFeed = priceFeeds[0];
      const currentPrice = Number(btcFeed.price.price) * Math.pow(10, btcFeed.price.expo);
      
      // Store price history
      const now = Date.now();
      priceHistory.push({ price: currentPrice, timestamp: now });
      
      // Remove old entries (older than 24h)
      priceHistory = priceHistory.filter(entry => now - entry.timestamp < HISTORY_DURATION);
      
      // Find price from ~24h ago
      const targetTime = now - HISTORY_DURATION;
      const oldestEntry = priceHistory[0];
      
      if (oldestEntry && (now - oldestEntry.timestamp) >= (23 * 60 * 60 * 1000)) {
        price24hAgo = oldestEntry.price;
      }
      
      // Calculate 24h change
      if (price24hAgo) {
        priceData.change24h = currentPrice - price24hAgo;
        priceData.changePercent24h = ((currentPrice - price24hAgo) / price24hAgo) * 100;
      }
      
      priceData.price = currentPrice;
      priceData.confidence = Number(btcFeed.price.conf) * Math.pow(10, btcFeed.price.expo);
      priceData.lastUpdate = new Date().toISOString();
      
      console.log(`BTC Price: $${currentPrice.toFixed(2)} | Change: ${priceData.changePercent24h.toFixed(2)}%`);
    }
  } catch (error) {
    console.error('Error fetching BTC price:', error);
  }
}

// Fetch price every 5 seconds
setInterval(fetchBTCPrice, 5000);
fetchBTCPrice(); // Initial fetch

// API endpoint for Arduino
app.get('/api/btc-price', (req, res) => {
  res.json({
    price: priceData.price.toFixed(2),
    change24h: priceData.change24h.toFixed(2),
    changePercent24h: priceData.changePercent24h.toFixed(2),
    direction: priceData.change24h >= 0 ? 'up' : 'down',
    lastUpdate: priceData.lastUpdate
  });
});

// Simple endpoint for Arduino to test connection
app.get('/api/health', (req, res) => {
  res.json({ status: 'ok', timestamp: new Date().toISOString() });
});

app.listen(PORT, () => {
  console.log(`BTC LED Sign Server running on http://localhost:${PORT}`);
  console.log(`Arduino endpoint: http://localhost:${PORT}/api/btc-price`);
});