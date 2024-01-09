#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <OneWire.h>
#include <DallasTemperature.h>

const char* ssid = "***";
const char* password = "***";
String newHostName = "ESP_Battery";

ESP8266WebServer server(80);

const int oneWireBus = 5;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

unsigned long sleepTime = 0;

void getHelloArduino();

void restServerRouting();

void handleNotFound();

void handleGetCurrentTemperature();

void setup() {
  Serial.begin(115200);
  sensors.begin();

  WiFi.mode(WIFI_STA);
  WiFi.hostname(newHostName.c_str());
  WiFi.begin(ssid, password);

  Serial.println("");

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  restServerRouting();
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  if (sleepTime > 0 && millis() > sleepTime) {
    Serial.println("Going to sleep now!");
    ESP.deepSleep(3570e6);
  }
}

void getHelloArduino() {
  server.send(200, "text/json", "{\"name\": \"Hello Arduino!\"}");
}

void restServerRouting() {
  server.on("/", HTTP_GET, []() {
    server.send(200, F("text/html"), F("Eine kleine, leere Website!"));
  });
  server.on(F("/helloArduino"), HTTP_GET, getHelloArduino);
  server.on(F("/current_temperature"), HTTP_GET, handleGetCurrentTemperature);
}

void handleNotFound() {
  String message = "File not Found \n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void handleGetCurrentTemperature() {
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  String result = "{\"temperatureInC\": ";
  result += temperature;
  result += "}";
  server.send(200, F("text/json"), "" + result );
  sleepTime = millis() + 5000;
}
