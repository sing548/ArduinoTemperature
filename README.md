# ArduinoTemperature — Wiring & Configuration

Board: D1 Mini Lite (ESP8266). Sensors: DS18B20 (OneWire), BME280 (I2C), SCD41 (I2C).

## Pinout

| Sensor | Signal | D1 Mini pin | GPIO |
|---|---|---|---|
| DS18B20 | Data | D1 | GPIO5 |
| DS18B20 | VCC | 3V3 | — |
| DS18B20 | GND | GND | — |
| BME280 | SDA | D2 | GPIO4 |
| BME280 | SCL | D1 | GPIO5 |
| BME280 | VCC | 3V3 | — |
| BME280 | GND | GND | — |
| SCD41 | SDA | D2 | GPIO4 |
| SCD41 | SCL | D1 | GPIO5 |
| SCD41 | VCC | 3V3 | — |
| SCD41 | GND | GND | — |

BME280 and SCD41 sit on the **same I2C bus** (D2=SDA, D1=SCL) — wire them in parallel, no extra wiring needed for the second sensor. They're distinguished by I2C address:

- BME280 — `0x76`
- SCD41 — `0x62` (fixed, not configurable)

Most BME280/SCD41 breakout boards (Adafruit, Seeed, etc.) include their own SDA/SCL pull-up resistors — no external pull-ups needed unless you're using bare modules.

**Known quirk:** the DS18B20 data line and I2C SCL both use D1 (GPIO5). This works because OneWire and I2C transactions never run concurrently in the single-threaded `loop()` — but if you ever see intermittent I2C or temperature-probe failures, this shared pin is the first thing to suspect. DS18B20 also needs its own 4.7kΩ pull-up between data and 3V3 if the probe cable doesn't already have one built in.

## config.h

`include/config.h` is gitignored — create it before building:

```cpp
#pragma once
const char* ssid        = "your_ssid";
const char* password    = "your_password";
const char* newHostName = "ESP_With_Cable_BME280";
```

`newHostName` shows up as the device's WiFi hostname — useful for finding it on your router/DHCP leases. SCD41 is optional: if it's not soldered, `scd41.begin()` fails gracefully, `/air-quality` returns 503, and the other two endpoints are unaffected.
