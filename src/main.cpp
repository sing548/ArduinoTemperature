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

#include <SensirionI2CScd4x.h>

#include "config.h"

ESP8266WebServer server(80);

const int oneWireBus = 5;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

Adafruit_BME280 bme;
SensirionI2CScd4x scd41;

bool scd41Available = false;
bool scd41DataValid = false;
uint16_t co2Ppm = 0;
float tempScd41 = 0.0f;
float humScd41 = 0.0f;

void restServerRouting();
void handleNotFound();
void handleGetCurrentTemperature();
void handleGetCurrentTempPressHum();
void handleGetAirQuality();

void setup() {
  Serial.begin(115200);
  sensors.begin();
  Wire.begin();

  Serial.println("");

  bool bmeStatus = bme.begin(0x76);
  if (!bmeStatus) {
    Serial.println("No BME280 sensor!");
  }

  scd41.begin(Wire);
  scd41.stopPeriodicMeasurement();
  delay(500);
  uint16_t scd41Error = scd41.startPeriodicMeasurement();
  scd41Available = (scd41Error == 0);
  if (!scd41Available) {
    Serial.println("No SCD41 sensor!");
  }

  WiFi.mode(WIFI_STA);
  WiFi.hostname(newHostName);
  WiFi.begin(ssid, password);

  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
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

  if (scd41Available) {
    bool dataReady = false;
    if (scd41.getDataReadyFlag(dataReady) == 0 && dataReady) {
      uint16_t co2;
      float temp, hum;
      if (scd41.readMeasurement(co2, temp, hum) == 0 && co2 != 0) {
        co2Ppm = co2;
        tempScd41 = temp;
        humScd41 = hum;
        scd41DataValid = true;
      }
    }
  }
}

void restServerRouting() {
  server.on("/", HTTP_GET, []() {
    server.send(200, F("text/html"), F("Sensor Node"));
  });
  server.on(F("/current_temperature"), HTTP_GET, handleGetCurrentTemperature);
  server.on(F("/c-path"), HTTP_GET, handleGetCurrentTempPressHum);
  server.on(F("/air-quality"), HTTP_GET, handleGetAirQuality);
}

void handleNotFound() {
  String message = "File not Found\n\nURI: ";
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
  Serial.println(result);
  server.send(200, F("text/json"), result);
}

void handleGetCurrentTempPressHum() {
  float temp  = bme.readTemperature();
  float press = bme.readPressure();
  float hum   = bme.readHumidity();

  String result = "{ \"temperatureInC\": ";
  result += temp;
  result += ", \"pressureInPa\": ";
  result += press;
  result += ", \"humidityPerc\": ";
  result += hum;
  result += " }";
  Serial.println(result);
  server.send(200, F("text/json"), result);
}

void handleGetAirQuality() {
  if (!scd41Available || !scd41DataValid) {
    server.send(503, F("text/json"), F("{\"error\": \"SCD41 not ready\"}"));
    return;
  }
  String result = "{ \"co2Ppm\": " + String(co2Ppm);
  result += ", \"temperatureInC\": " + String(tempScd41);
  result += ", \"humidityPerc\": " + String(humScd41) + " }";
  Serial.println(result);
  server.send(200, F("text/json"), result);
}
