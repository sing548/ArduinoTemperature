#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)

const char* ssid = "";
const char* password = "";
String newHostName = "ESP_With_Cable_BME280";

ESP8266WebServer server(80);

const int oneWireBus = 5;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

Adafruit_BME280 bme;

unsigned long sleepTime = 0;
bool sleepEnabled = false;

void getHelloArduino();

void restServerRouting();

void handleNotFound();

void handleGetCurrentTemperature();
void handleGetCurrentTempPressHum();

void setup() {
  Serial.begin(115200);
  sensors.begin();

  Serial.println("");

  bool bmeStatus;

  bmeStatus = bme.begin(0x76);
  if (!bmeStatus) {
    Serial.println("No BME280 sensor!");
  }

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
  if (sleepTime > 0 && millis() > sleepTime && sleepEnabled) {
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
  server.on(F("/c-path"), HTTP_GET, handleGetCurrentTempPressHum);
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
  Serial.println("Temperature value requested.");
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  String result = "{\"temperatureInC\": ";
  result += temperature;
  result += "}";
  Serial.println("Sending value:");
  Serial.println(result);
  server.send(200, F("text/json"), "" + result );
  Serial.println("Value sent.");
  sleepTime = millis() + 5000;
}

void handleGetCurrentTempPressHum() {
  Serial.println("BME values requested.");
  float temp = bme.readTemperature();
  float press = bme.readPressure();
  float hum = bme.readHumidity();

  String result = "{ \"temperatureInC\": ";
  result += temp;
  result += ", \"pressureInPa\": ";
  result += press;
  result += ", \"humidityPerc\": ";
  result += hum;
  result += " }";
  Serial.println("Sending values:");
  Serial.println(result);
  server.send(200, F("text/json"), "" + result);
  Serial.println("Values sent.");
  sleepTime = millis() + 5000;
}
