#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include "ModbusMaster.h"
//#include "DHTesp.h"

#define MAX485_DE      23
#define MAX485_RE_NEG  18
#define Slave_ID       1
#define RX_PIN      16 //RX2 
#define TX_PIN      17  //TX2

const char* ssid = "true_home2G_9X2";
const char* password = "FJdey62Y";

String webViewMsg = "Hi! ESP32.";

WebServer server(80);
ModbusMaster node;
//DHTesp dht;

void preTransmission()
{
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}
void setup(void) {
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  // Init in receive mode
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);

  //dht.setup(19, DHTesp::DHT11);

  Serial.begin(115200);// Modbus communication runs at 115200 baud
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
    //webViewMsg = "Hi! ESP32.";
    server.send(200, "text/plain", webViewMsg);
  });

  ElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  Serial2.begin(9600, SERIAL_8N2, RX_PIN, TX_PIN);
  node.begin(Slave_ID, Serial2);
  // Callbacks allow us to configure the RS485 transceiver correctly
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
}

long lastMillis = 0;
uint8_t j, result;
uint16_t data[6];

void loop(void) {

  //web handle
  server.handleClient();

  //meter handle
  long currentMillis = millis();
  if (currentMillis - lastMillis > 10000)
  {
    result = node.readInputRegisters(0x00, 8);
    if (result == node.ku8MBSuccess)
    {
      Serial.println("ok");
      for (j = 0; j < 8; j++)
      {
        data[j] = node.getResponseBuffer(j);
      }
      webViewMsg = String(data[0] / 100); //volt
    } else {
      Serial.println("err");
    }

//    TempAndHumidity newValues = dht.getTempAndHumidity();
//    Serial.println(" T:" + String(newValues.temperature) + " H:" + String(newValues.humidity));

    lastMillis = currentMillis;
  }
}
