#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// variaveis

const byte pot_pin = 4;
const byte echo_pin = 18;
const byte trigg_pin = 9;
const byte pir_pin = 5;

String dist = "";
bool mov = false;
String pot = "";

// const char* SSID = "Redmi 8 do lucas";
// const char* PSWD = "12345678";

const char* SSID = "Wellyson";
const char* PSWD = "12345678";

const char* brokerUrl = "test.mosquitto.org";
const int port = 1883;

unsigned long ultimoTempoDist = 0;
unsigned long ultimoTempoPot = 0;
unsigned long ultimoTempoPir = 0;
const long intervaloDist = 1000;
const long intervaloPot = 1000;
const long intervaloPir = 1000;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

JsonDocument doc;

void connectToWiFi();
void connectToBroker();
void callback(char* topic, byte* payload, unsigned long length);
int lerDistancia();
int lerPot();
int lerPir();

void setup() {
  Serial.begin(115200);
  pinMode(echo_pin, INPUT);
  pinMode(trigg_pin, OUTPUT);
  pinMode(pot_pin, INPUT);
  pinMode(pir_pin, INPUT);

  connectToWiFi();
  connectToBroker();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectToWiFi();
  if (!mqttClient.connected()) connectToBroker();
  mqttClient.loop();

  unsigned long agora = millis();

  // --- Leitura da distância ---
  if (agora - ultimoTempoDist >= intervaloDist) {
    ultimoTempoDist = agora;
    int valor = lerDistancia();
    dist = String(valor);
    Serial.printf("Distância: %d cm\n", valor);
  }

  // --- Leitura do potenciômetro ---
  if (agora - ultimoTempoPot >= intervaloPot) {
    ultimoTempoPot = agora;
    int valor = lerPot();
    pot = String(valor);
  }

  // --- Leitura do Pir
  if (agora - ultimoTempoPir >= intervaloPir) {
    ultimoTempoPir = agora;
    int valor = lerPir();
    if (valor == 1) {
      mov = true;
    } else {
      mov = false;
    }
  }

  // --- Envio manual pelo Serial ---
  if (Serial.available() > 0) {
    String info = "";

    doc["distancia_cm"] = dist;
    doc["movimento"] = mov;
    doc["limiar_pot"] = pot;
    serializeJson(doc, info);

    String msg = Serial.readStringUntil('\n');
    mqttClient.publish("duplaLWP/estacao/dados", info.c_str());
  }
}

void connectToWiFi() {
  Serial.println("\nConectando Wi-Fi...");
  WiFi.begin(SSID, PSWD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(250);
  }
  Serial.println("\nWi-Fi conectado!");
}

void connectToBroker() {
  mqttClient.setServer(brokerUrl, port);
  mqttClient.setCallback(callback);
  while (!mqttClient.connected()) {
    String clientId = "ESP-CALIXTO-" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      mqttClient.subscribe("duplaLWP/estacao/dados");
      Serial.println("MQTT conectado!");
    } else {
      Serial.print(".");
      delay(500);
    }
  }
}

void callback(char* topic, byte* payload, unsigned long length) {
  String resposta = "";
  for (int i = 0; i < length; i++) {
    resposta += (char)payload[i];
  }

  Serial.println(resposta);
}

int lerDistancia() {
  digitalWrite(trigg_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigg_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigg_pin, LOW);

  unsigned long duracao = pulseIn(echo_pin, HIGH, 20000);
  if (duracao == 0) return -1;
  int distancia = duracao * 0.0343 / 2;
  return distancia;
}

int lerPot() {
  int valor = analogRead(pot_pin);
  Serial.printf("Pot: %d\n", valor);
  return valor;
}

int lerPir() {
  int presenca = digitalRead(pir_pin);
  Serial.printf("Pir: %d\n", presenca);
  Serial.print("");
  return presenca;
}
