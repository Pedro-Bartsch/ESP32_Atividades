#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// pinos
#define SERVO_PIN 9
const byte sensor_pin = 5;
const byte pot_pin = 4;

// variaveis
String movimento = "";
int tempoEnvio = 1000;

unsigned long ultimoTempoSensor = 0;
const long intervaloSensor = 100;
unsigned long ultimoTempoPot = 0;
const long intervaloPot = 1000;
unsigned long ultimoEnvio = 0;

const String SSID = "iPhone";
const String PSWD = "123456789";

const String brokerUrl = "test.mosquitto.org";  // URL do broker   (servidor)
const int port = 1883;                          // Porta do broker (servidor)

String message = "";

Servo servo;
WiFiClient espClient;                // Criando Cliente WiFi
PubSubClient mqttClient(espClient);  // Criando Cliente MQTT

// SSID = NOME
// RSSI = Intesidade do sinal

JsonDocument doc;

void connectToWiFi();
void connectToBroker();
void callback(char* topic, byte* payload, unsigned long length);
int lerSensor();
int lerPot();

void setup() {
  Serial.begin(115200);
  servo.attach(SERVO_PIN);
  pinMode(sensor_pin, INPUT);
  pinMode(pot_pin, INPUT);

  connectToWiFi();
  connectToBroker();

  servo.write(0);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nConexão WiFi perdida");
    connectToWiFi();
  }
  if (!mqttClient.connected()) {
    Serial.println("\nConexão MQTT perdida");
    connectToBroker();
  }

  unsigned long agora = millis();

 // --- Leitura do pot
  if (agora - ultimoTempoPot >= intervaloPot) {
    tempoEnvio = lerPot();
    ultimoTempoPot = agora;
  }


  // --- Leitura do Sensor
  if (agora - ultimoTempoSensor >= intervaloSensor) {
    ultimoTempoSensor = agora;
    int valor = lerSensor();
    if (valor == 1) {
      movimento = "na_linha";
      servo.write(0);

    } else {
      movimento = "fora_da_linha";
      servo.write(90);
    }
  }

  if (agora - ultimoEnvio >= tempoEnvio) {
    ultimoEnvio = agora;

    String info = "";
    doc["status"] = movimento;
    serializeJson(doc, info);
    //{ "status": "valor" }
    mqttClient.publish("duplaLWP/robo/status", info.c_str());
  }

  mqttClient.loop();
}

// Conexão WiFi
void connectToWiFi() {
  Serial.println("\nIniciando conexão com rede WiFi");
  WiFi.begin(SSID, PSWD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }
  Serial.print("\nWi-Fi Conectado!");
}

// Conexão Broker
void connectToBroker() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  Serial.println("\nConectando ao broker");
  mqttClient.setServer(brokerUrl.c_str(), port);
  String userId = "ESP-CALIXTO";
  userId += String(random(0xfff), HEX);
  while (!mqttClient.connected()) {
    mqttClient.connect(userId.c_str());
    Serial.print(".");
    delay(500);

    mqttClient.subscribe("duplaLWP/robo/status");
    mqttClient.setCallback(callback);
  }
  Serial.println("\nConectado com sucesso!");
}

// recebe resposta
void callback(char* topic, byte* payload, unsigned long length) {
  String resposta = "";
  for (int i = 0; i < length; i++) {
    resposta += (char)payload[i];
  }

  Serial.println(resposta);
}

// le o valor do pot
int lerPot() {
  int valor = analogRead(pot_pin);
  Serial.printf("Pot: %d\n", valor);
  int tempo = map(valor, 0, 3240, 500, 2500);
  Serial.printf("tempo: %d\n", tempo);
  return tempo;
}

// le o valor Sensor de linha
int lerSensor() {
  int presenca = digitalRead(sensor_pin);
  Serial.println("");
  Serial.printf("Sensor: %d\n", presenca);
  Serial.print("");
  return presenca;
}
