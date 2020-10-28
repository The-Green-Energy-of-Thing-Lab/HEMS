#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>

const char* ssid = "chuwi10";
const char* password = "1234567890";

String webViewMsg;

WebServer server(80);


void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin("esp32")) {
    Serial.println("Error setting up MDNS responder!");
  }

  server.on("/", []() {
    webViewMsg = "Hi! ESP32.";
    server.send(200, "text/plain", webViewMsg);
  });

  ElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}

void loop(void) {
  server.handleClient();
}
