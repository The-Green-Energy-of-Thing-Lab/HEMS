#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include "ModbusMaster.h"
#include "DHTesp.h"
#include <HTTPClient.h>

#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"


#define MAX485_DE      23
#define MAX485_RE_NEG  18
#define Slave_ID       1
#define RX_PIN      16 //RX2 
#define TX_PIN      17  //TX2

const char* ssid = "true_home2G_9X2";
const char* password = "FJdey62Y";

String webViewMsg = "Hi! ESP32.";

int lCnt = 0;
long lastMillis = 0;
long lastHttpMillis = 0;
long lastDhtMillis = 0;
uint8_t j, result;
uint16_t dataValue[6];
String V;
String I;
String P;
String E;
String TH;

WebServer server(80);
ModbusMaster node;
DHTesp dht;
SSD1306Wire display(0x3c, 4, 15);

void drawTextAlignmentDemo();

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

  dht.setup(19, DHTesp::DHT11);

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

  display.init();
  display.flipScreenVertically();
}


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
        dataValue[j] = node.getResponseBuffer(j);
      }
      V = String((float)dataValue[0] / 100); //volt
      I = String((float)dataValue[1] / 100); //current
      P = String((float)((dataValue[3] << 8) | (dataValue[2])) / 10);
      E = String((dataValue[5] << 8) | (dataValue[4]));

      webViewMsg = V + " " + I + " " + P + " " + E;
    } else {
      Serial.println("err");
    }

    TempAndHumidity newValues = dht.getTempAndHumidity();
    TH = "T=" + String(newValues.temperature) + "&H=" + String(newValues.humidity);

    drawTextAlignmentDemo();

    lastMillis = currentMillis;
  }

  //http get batt
  currentMillis = millis();
  if (currentMillis - lastHttpMillis > 10000)
  {
    if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
      HTTPClient http;
      String httpAddr = "http://192.168.1.34/getBatStatus?V=" + V + "&I=" + I + "&P=" + P + "&E=" + E;
      http.begin(httpAddr);

      int httpCode = http.GET();

      if (httpCode > 0) { //Check for the returning code
        String payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);
      } else {
        Serial.println("Error on HTTP request");
      }

      http.end(); //Free the resources
    }

    lastHttpMillis = currentMillis;
  }

  //http get weather
  currentMillis = millis();
  if (currentMillis - lastDhtMillis > 30000)
  {
    if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
      HTTPClient http;
      String httpAddr = "http://192.168.1.34/getWeather?" + TH;
      http.begin(httpAddr);

      int httpCode = http.GET();

      if (httpCode > 0) { //Check for the returning code
        String payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);
      } else {
        Serial.println("Error on HTTP request");
      }

      http.end(); //Free the resources
    }

    lastDhtMillis = currentMillis;
  }
}
void drawTextAlignmentDemo() {
  // Text alignment demo
  display.setFont(ArialMT_Plain_16);

  // The coordinates define the left starting point of the text
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 10, "Left aligned (0,10)");

  // The coordinates define the center of the text
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, "Center aligned (64,22)");

  // The coordinates define the right end of the text
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(128, 33, "Right aligned (128,33)");
  display.display();
}
