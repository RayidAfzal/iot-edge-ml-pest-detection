#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>

// ========= NODE CONFIG =========
#define NODE_NAME "tomato"   // change to "carrot" on the second ESP32

// WiFi config (for TCP to gateway)
const char* WIFI_SSID = "Rayids24";
const char* WIFI_PASS = "fazzae69";

// TODO: change these when gateway is ready
const char* GATEWAY_HOST = "10.230.109.16";   // placeholder IP of gateway ESP32
const uint16_t GATEWAY_PORT = 5000;          // placeholder port

// ========= PIN CONFIG =========
#define DHTPIN   4
#define DHTTYPE  DHT11
DHT dht(DHTPIN, DHTTYPE);

const int SOIL_PIN = 34;   // analog
const int LDR_PIN  = 32;   // digital (D0 from module)
const int MQ_PIN   = 35;   // analog (A0 from MQ135 via divider)

// ========= HELPERS =========
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Soil: raw 0–4095 → 0–100 %
// You may need to invert depending on how your sensor behaves.
float normalizeSoil(int raw) {
  // assume: dry ≈ 3000–4095, wet ≈ 0–1000  (you can tune this)
  float pct = mapFloat(raw, 3500, 1000, 0.0, 100.0);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

// Gas: rough 0–4095 → 0–100 index
float normalizeGas(int raw) {
  float pct = mapFloat(raw, 0, 4095, 0.0, 100.0);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

// Light: digital from LDR module
// HIGH = bright, LOW = dark
float lightScore(int digitalState) {
  // just map to two levels for now
  return digitalState == HIGH ? 80.0 : 20.0;
}

// Simple rule-based pest risk (0.0–1.0)
float computeRisk(float temp, float hum, float soilPct, float lightPct, float gasPct) {
  float score = 0.0;

  // warm range
  if (temp >= 20 && temp <= 32) score += 0.25;
  // humid range
  if (hum  >= 60 && hum  <= 85) score += 0.25;
  // medium soil moisture
  if (soilPct >= 30 && soilPct <= 70) score += 0.20;
  // moderate light
  if (lightPct >= 20 && lightPct <= 80) score += 0.15;
  // elevated gas (rough indication of activity)
  if (gasPct >= 40) score += 0.15;

  if (score > 1.0) score = 1.0;
  return score;
}

// ========= WiFi + TCP =========
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 30) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed (will retry later).");
  }
}

void sendToGateway(const String& jsonPayload) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Still no WiFi, skipping gateway send.");
      return;
    }
  }

  WiFiClient client;
  if (!client.connect(GATEWAY_HOST, GATEWAY_PORT)) {
    Serial.println("Could not connect to gateway (TCP).");
    return;
  }

  // Send JSON line to gateway
  client.println(jsonPayload);
  client.stop();
}

// ========= SETUP =========
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println();
  Serial.println("=== Leaf Node Boot ===");
  Serial.print("Node: ");
  Serial.println(NODE_NAME);

  dht.begin();
  pinMode(LDR_PIN, INPUT);

  // set ADC resolution
  analogReadResolution(12); // ESP32 default: 0–4095

  connectWiFi();
}

// ========= LOOP =========
void loop() {
  // 1. Read DHT
  float temp = dht.readTemperature(); // °C
  float hum  = dht.readHumidity();    // %

  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT read failed, skipping this cycle.");
    delay(2000);
    return;
  }

  // 2. Read soil moisture
  int rawSoil = analogRead(SOIL_PIN);
  float soilPct = normalizeSoil(rawSoil);

  // 3. Read light (digital from module)
  int ldrState = digitalRead(LDR_PIN);
  float lightPct = lightScore(ldrState);

  // 4. Read gas (MQ-135 via divider)
  int rawGas = analogRead(MQ_PIN);
  float gasPct = normalizeGas(rawGas);

  // 5. Compute risk
  float risk = computeRisk(temp, hum, soilPct, lightPct, gasPct);

  // 6. Timestamp (just millis for now)
  unsigned long nowMs = millis();

  // 7. Create JSON string
  String json = "{";
  json += "\"node\":\"";          json += NODE_NAME;      json += "\",";
  json += "\"timestamp_ms\":";    json += nowMs;          json += ",";
  json += "\"temp\":";            json += String(temp, 2);    json += ",";
  json += "\"hum\":";             json += String(hum, 2);     json += ",";
  json += "\"soil\":";            json += String(soilPct, 2); json += ",";
  json += "\"light\":";           json += String(lightPct, 2);json += ",";
  json += "\"gas\":";             json += String(gasPct, 2);  json += ",";
  json += "\"risk\":";            json += String(risk, 3);
  json += "}";

  // 8. Print to Serial (for debugging / logging)
  Serial.println(json);

  // 9. Send to gateway (will fail until you set IP/port on gateway)
  sendToGateway(json);

  delay(2000); // 2 seconds between readings
}
