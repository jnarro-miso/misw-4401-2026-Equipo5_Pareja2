#include <WiFi.h>  // WiFi para ESP32
#include <WiFiClientSecure.h>
#include <time.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHTTYPE DHT11
#define dht_dpin 25  // GPIO25 de la placa ESP32
#define PHOTO_PIN 34  // GPIO34 para ESP32

DHT dht(dht_dpin, DHTTYPE);

#include "secrets.h"

// ============================================
// CONFIGURACIÓN WIFI
// ============================================
const char ssid[] = "CAMBIAR";
const char pass[] = "CAMBIAR";

// ============================================
// CONFIGURACIÓN MQTT UNIANDES
// ============================================
// Usuario uniandes SIN @uniandes.edu.co
#define HOSTNAME "CAMBIAR"

// Broker MQTT
const char MQTT_HOST[] = "iotlab.virtual.uniandes.edu.co";
const int MQTT_PORT = 8082;

// Credenciales MQTT
const char MQTT_USER[] = "CAMBIAR";
const char MQTT_PASS[] = "CAMBIAR";

// Tópicos MQTT
const char MQTT_SUB_TOPIC[] = HOSTNAME "/";
// Reemplazar "ciudad" por tu ciudad (minúsculas, sin espacios, sin acentos)
const char MQTT_PUB_TOPIC1[] = "humedad/lima/" HOSTNAME;
const char MQTT_PUB_TOPIC2[] = "temperatura/lima/" HOSTNAME;
const char MQTT_PUB_TOPIC3[] = "luminosidad/lima/" HOSTNAME;

// ============================================
// CONFIGURACIÓN SSL/TLS
// ============================================
#if (defined(CHECK_PUB_KEY) and defined(CHECK_CA_ROOT)) or (defined(CHECK_PUB_KEY) and defined(CHECK_FINGERPRINT)) or (defined(CHECK_FINGERPRINT) and defined(CHECK_CA_ROOT)) or (defined(CHECK_PUB_KEY) and defined(CHECK_CA_ROOT) and defined(CHECK_FINGERPRINT))
#error "cant have both CHECK_CA_ROOT and CHECK_PUB_KEY enabled"
#endif

WiFiClientSecure net;
PubSubClient client(net);

time_t now;
unsigned long lastMillis = 0;

int photoRaw = 0;
float luminosity = 0;

// ============================================
// FUNCIÓN: Leer Photoresistor
// ============================================
void readPhotoresistor() {
  photoRaw = analogRead(PHOTO_PIN);
  luminosity = map(photoRaw, 4095, 0, 0, 100);
}

// ============================================
// FUNCIÓN: Conectar a MQTT
// ============================================
void mqtt_connect() {
  while (!client.connected()) {
    Serial.print("Time: ");
    Serial.print(ctime(&now));
    Serial.print("MQTT connecting ... ");

    if (client.connect(HOSTNAME, MQTT_USER, MQTT_PASS)) {
      Serial.println("✓ Conectado a MQTT!");
    } else {
      Serial.println("✗ Error de conexión MQTT");
      Serial.print("  Código de error: ");
      Serial.println(client.state());

      // Códigos de error comunes:
      // -4: MQTT_CONNECTION_TIMEOUT
      // -3: MQTT_CONNECTION_LOST
      // -2: MQTT_CONNECT_FAILED
      //  1: MQTT_CONNECT_BAD_PROTOCOL
      //  2: MQTT_CONNECT_BAD_CLIENT_ID
      //  4: MQTT_CONNECT_BAD_CREDENTIALS
      //  5: MQTT_CONNECT_UNAUTHORIZED

      if (client.state() == MQTT_CONNECT_UNAUTHORIZED || client.state() == 4) {  // Bad credentials
        Serial.println("  ⚠️ Revisa MQTT_USER y MQTT_PASS");
        Serial.println("  ⚠️ Usuario SIN @uniandes.edu.co");
        Serial.println("  ⚠️ Password = código estudiante");
        // Usa delay largo porque ESP32 no tiene deepSleep
        delay(30000);
      }

      delay(5000);
    }
  }
}

// ============================================
// CALLBACK: Mensajes recibidos
// ============================================
void receivedCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n========================================");
  Serial.println("ESP32 - MQTT Uniandes");
  Serial.println("========================================\n");

  // Inicializar DHT11
  dht.begin();
  Serial.println("✓ DHT11 inicializado");

  // Inicializar Photoresistor
  pinMode(PHOTO_PIN, INPUT);
  Serial.println("✓ Photoresistor inicializado");

  // Conectar a WiFi
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  int wifi_attempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_attempts < 20) {
    delay(500);
    Serial.print(".");
    wifi_attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n✗ Error: No se pudo conectar a WiFi");
    Serial.println("  Verifica SSID y contraseña");
    while (1) { delay(1000); }  // Detener ejecución
  }

  Serial.println("\n✓ WiFi conectado!");
  Serial.print("  IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("  Señal: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  // Sincronizar tiempo (NTP)
  Serial.print("\nSincronizando hora (NTP)...");
  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);

  int ntp_attempts = 0;
  while (now < 1510592825 && ntp_attempts < 20) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    ntp_attempts++;
  }

  if (now < 1510592825) {
    Serial.println("\n⚠️ Advertencia: No se pudo sincronizar hora");
  } else {
    Serial.println(" ✓");
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print("  Hora actual: ");
    Serial.print(asctime(&timeinfo));
  }

  // Configurar SSL/TLS
  // #ifdef CHECK_CA_ROOT
  //   BearSSL::X509List cert(digicert);
  //   net.setTrustAnchors(&cert);
  // #endif
  // #ifdef CHECK_PUB_KEY
  //   BearSSL::PublicKey key(pubkey);
  //   net.setKnownKey(&key);
  // #endif
  // #ifdef CHECK_FINGERPRINT
  //   net.setFingerprint(fp);
  // #endif
  // #if (!defined(CHECK_PUB_KEY) and !defined(CHECK_CA_ROOT) and !defined(CHECK_FINGERPRINT))
  //   net.setInsecure();
  // #endif
  
  // TODO: ESP32 con Arduino usa WiFiClientSecure (basado en mbedTLS), no BearSSL como el NodeMCU/ESP8266
  // Por el momento, se usa setInsecure()
  net.setInsecure();

  // Configurar MQTT
  Serial.println("\nConfigurando MQTT...");
  Serial.print("  Broker: ");
  Serial.println(MQTT_HOST);
  Serial.print("  Puerto: ");
  Serial.println(MQTT_PORT);
  Serial.print("  Usuario: ");
  Serial.println(MQTT_USER);
  Serial.print("  Hostname: ");
  Serial.println(HOSTNAME);

  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(receivedCallback);

  // Conectar a MQTT
  mqtt_connect();

  Serial.println("\n========================================");
  Serial.println("Sistema listo. Publicando cada 5 seg...");
  Serial.println("========================================\n");
}

// ============================================
// LOOP
// ============================================
void loop() {
  // Verificar WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi desconectado. Reconectando...");
    WiFi.begin(ssid, pass);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(" ✓");
    }
  }

  // Verificar MQTT
  if (!client.connected()) {
    mqtt_connect();
  } else {
    client.loop();
  }

  // Actualizar tiempo
  now = time(nullptr);

  // Leer sensor DHT11
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Leer Photoresistor
  readPhotoresistor();

  // Validar lecturas
  if (isnan(h) || isnan(t)) {
    Serial.println("⚠️ Error leyendo DHT11");
    delay(5000);
    return;
  }

  if (isnan(luminosity)) {
    Serial.println("⚠️ Error leyendo Photoresistor");
    delay(5000);
    return;
  }

  // Crear JSON para humedad
  String json1 = "{\"value\": " + String(h, 1) + "}";
  char payload1[json1.length() + 1];
  json1.toCharArray(payload1, json1.length() + 1);

  // Crear JSON para temperatura
  String json2 = "{\"value\": " + String(t, 1) + "}";
  char payload2[json2.length() + 1];
  json2.toCharArray(payload2, json2.length() + 1);

  // Crear JSON para luminosidad
  String json3 = "{\"value\": " + String(luminosity, 1) + "}";
  char payload3[json3.length() + 1];
  json3.toCharArray(payload3, json3.length() + 1);

  // Publicar en MQTT
  bool success1 = client.publish(MQTT_PUB_TOPIC1, payload1, false);
  bool success2 = client.publish(MQTT_PUB_TOPIC2, payload2, false);
  bool success3 = client.publish(MQTT_PUB_TOPIC3, payload3, false);

  // Mostrar en Monitor Serie
  Serial.print(success1 ? "✓ " : "✗ ");
  Serial.print(MQTT_PUB_TOPIC1);
  Serial.print(" → ");
  Serial.println(payload1);

  Serial.print(success2 ? "✓ " : "✗ ");
  Serial.print(MQTT_PUB_TOPIC2);
  Serial.print(" → ");
  Serial.println(payload2);

  Serial.print(success3 ? "✓ " : "✗ ");
  Serial.print(MQTT_PUB_TOPIC3);
  Serial.print(" → ");
  Serial.println(payload3);

  Serial.println("---");

  delay(5000);
}