# REMA - Red Estudiantil de Monitoreo Ambiental

Proyecto del curso MISW-4401 (IoT) - Universidad de los Andes

## Equipo 5 - Pareja 2

- Diego Jaramillo
- Zamir Narro

## Descripcion

Sistema IoT de monitoreo ambiental que permite recolectar y compartir datos de temperatura y humedad entre estudiantes geograficamente distribuidos, apoyando el monitoreo ambiental de ciudades y la toma de decisiones para reducir el impacto del consumo energetico.

El sistema se construye de forma incremental semana a semana, cubriendo desde el dispositivo IoT hasta la capa de aplicacion.

## Arquitectura

- **Dispositivo IoT:** ESP32 + Sensor DHT11
- **Comunicacion:** WiFi + MQTT (TLS)
- **Broker MQTT:** `iotlab.virtual.uniandes.edu.co:8082`
- **Topicos:**
  - `humedad/<ciudad>/<hostname>` - Humedad relativa (%)
  - `temperatura/<ciudad>/<hostname>` - Temperatura (C)
- **Formato de datos:** `{"value": <lectura>}`

## Configuracion

Antes de cargar el codigo, modificar en `rema-monitor.ino`:

1. `ssid` y `pass` - Credenciales WiFi
2. `HOSTNAME`, `MQTT_USER`, `MQTT_PASS` - Credenciales Uniandes (usuario sin @uniandes.edu.co)
3. Ciudad en los topicos MQTT segun corresponda

Se requiere un archivo `secrets.h` con la configuracion de seguridad TLS.

## Hardware

- ESP32
- Sensor DHT11

## Dependencias

- WiFi.h / WiFiClientSecure.h (ESP32 Arduino Core)
- [PubSubClient](https://github.com/knolleary/pubsubclient) - Cliente MQTT
- [DHT sensor library](https://github.com/adafruit/DHT-sensor-library) - Sensor DHT11
