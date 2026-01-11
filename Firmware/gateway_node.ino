#define BLYNK_TEMPLATE_ID   "TMPL3J-yOlCa0"
#define BLYNK_TEMPLATE_NAME "Edge Gateway"
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ArduinoJson.h>

// ====== Blynk Auth Token ======
char auth[] = "HmQ3jZFId1btYqKRLMjxpPYc8QTmuUK1";

// ====== WiFi credentials ======
char ssid[] = "Rayids24";
char pass[] = "fazzae69";

// ====== TCP server config ======
const uint16_t GATEWAY_PORT = 5000;
WiFiServer server(GATEWAY_PORT);

// ====== simple storage to print last values (optional) ======
struct NodeData {
  float temp  = 0;
  float hum   = 0;
  float soil  = 0;
  float risk  = 0;
};

NodeData tomatoData;
NodeData carrotData;

void printNodeData(const char* label, const NodeData& d) {
  Serial.print(label);
  Serial.print(" -> T: ");  Serial.print(d.temp);
  Serial.print("  H: ");    Serial.print(d.hum);
  Serial.print("  Soil: "); Serial.print(d.soil);
  Serial.print("  Risk: "); Serial.println(d.risk);
}

// ====== setup ======
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("=== Edge Gateway Boot (ESP8266) ===");

  // Connect to WiFi + Blynk
  Serial.println("Connecting to WiFi + Blynk...");
  Blynk.begin(auth, ssid, pass);  // blocks until WiFi + Blynk are connected

  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());

  // Start TCP server
  server.begin();
  Serial.print("TCP server listening on port ");
  Serial.println(GATEWAY_PORT);
}

// ====== function: handle incoming TCP clients ======
void handleClients() {
  WiFiClient client = server.available();
  if (!client) return;

  // Read one line of JSON (sent by leaf node with println)
  String line = client.readStringUntil('\n');
  line.trim();

  if (line.length() == 0) {
    client.stop();
    return;
  }

  Serial.print("Received: ");
  Serial.println(line);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, line);
  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    client.stop();
    return;
  }

  const char* node = doc["node"];
  float temp = doc["temp"] | 0.0;
  float hum  = doc["hum"]  | 0.0;
  float soil = doc["soil"] | 0.0;
  float risk = doc["risk"] | 0.0;   // 0–1 from leaf

  if (!node) {
    Serial.println("No 'node' field in JSON, ignoring.");
    client.stop();
    return;
  }

  // Scale risk to 0–100 for Blynk display
  float riskPct = risk * 100.0;

  if (strcmp(node, "tomato") == 0) {
    tomatoData.temp = temp;
    tomatoData.hum  = hum;
    tomatoData.soil = soil;
    tomatoData.risk = riskPct;

    // Update Blynk virtual pins
    Blynk.virtualWrite(V1, temp);
    Blynk.virtualWrite(V2, hum);
    Blynk.virtualWrite(V3, soil);
    Blynk.virtualWrite(V4, riskPct);

    printNodeData("Tomato", tomatoData);
  }
  else if (strcmp(node, "carrot") == 0) {
    carrotData.temp = temp;
    carrotData.hum  = hum;
    carrotData.soil = soil;
    carrotData.risk = riskPct;

    Blynk.virtualWrite(V5, temp);
    Blynk.virtualWrite(V6, hum);
    Blynk.virtualWrite(V7, soil);
    Blynk.virtualWrite(V8, riskPct);

    printNodeData("Carrot", carrotData);
  }
  else {
    Serial.print("Unknown node: ");
    Serial.println(node);
  }

  client.stop();
}

// ====== loop ======
void loop() {
  Blynk.run();
  handleClients();
}
