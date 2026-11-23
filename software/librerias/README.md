# Librerías y versiones

Versiones exactas usadas para compilar y desplegar el proyecto (verificadas el 22-11-2026).

## Firmware ESP32

| Componente | Versión | Cómo se instala |
|---|---|---|
| Arduino IDE | 2.3.10 | https://www.arduino.cc/en/software |
| Paquete de placas `esp32` (Espressif Systems) | 3.3.12 | Herramientas → Placa → Gestor de placas → "esp32" |
| Placa seleccionada | ESP32 Dev Module (`esp32:esp32:esp32`) | Herramientas → Placa |
| ArduinoJson (Benoit Blanchon) | 7.4.3 | Herramientas → Administrar bibliotecas → "ArduinoJson" |
| WiFi, WiFiMulti, WiFiClientSecure, HTTPClient, WebServer, ESPmDNS | Incluidas en el paquete esp32 3.3.12 | — |

Uso de memoria al compilar: programa 1.101.264 bytes (84 % de 1.310.720) y variables
globales 52.456 bytes (16 %).

## Servidor

| Componente | Versión |
|---|---|
| Node.js en Railway | 18.20.8 (`engines.node >= 18`) |
| express | ^4.21.0 |
| pg | ^8.13.0 |
| dotenv | ^16.4.5 |

## Página web

| Componente | Versión |
|---|---|
| Chart.js (jsDelivr) | 4.4.1 |

## Base de datos

| Componente | Versión |
|---|---|
| PostgreSQL en Aiven, plan Free-1-1gb | 18 |
