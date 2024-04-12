// WiFi
#include <WiFi.h>
#include <PubSubClient.h>

// JSON
#include <ArduinoJson.h>

//DHT11
#include "DHT.h"
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Update these with values suitable for your network.
const char* ssid = "achtst";
const char* password = "123567890";
const char* mqtt_server = "broker.mqtt-dashboard.com";
const char* temp_topic = "/esp32-mqtt/temp-topic";
const char* humi_topic = "/esp32-mqtt/humi-topic";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }

  randomSeed(micros());

  Serial.println(F(""));
  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print(F("Message arrived ["));
  Serial.print(topic);
  Serial.print(F("] "));
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print(F("Attempting MQTT connection..."));
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println(F("connected"));
      client.subscribe("/esp32/mqtt/in");
    } else {
      Serial.print(F("failed, rc="));
      Serial.print(client.state());
      Serial.println(F(" try again in 5 seconds"));
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void createJsonData(char* tempData, char* humiData, StaticJsonDocument<200> doc) {
  doc["Temperature"] = tempData;
  doc["Humidity"] = humiData;

  String strJson;
  serializeJson(doc, strJson);

  Serial.println(strJson);
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  dht.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  // mengirim data setiap 2 detik
  if (now - lastMsg > 2000) {
    lastMsg = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    // Convert the value to a char array
    char tempString[8];
    dtostrf(t, 2, 2, tempString);
    client.publish(temp_topic, tempString);

    // Convert the value to a char array
    char humString[8];
    dtostrf(h, 2, 2, humString);
    client.publish(humi_topic, humString);

    StaticJsonDocument<200> doc;
    createJsonData(tempString, humString, doc);
  }
}