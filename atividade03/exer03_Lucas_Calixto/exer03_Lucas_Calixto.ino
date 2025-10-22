#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

// pinos
const byte pot_pin = 4;
const byte echo_pin = 18;
const byte trigg_pin = 9;

// variaveis
String dist = "";
int pot = 0;
int valor = 0;
int valordist = 0;

unsigned long ultimoTempoDist = 0;
unsigned long ultimoTempoPot = 0;
const long intervaloDist = 1000;
const long intervaloPot = 1000;

unsigned long ultimoTempoDeteccao = 0;
unsigned long intervalo = 3000;

const String SSID = "iPhone";
const String PSWD = "123456789";

const String brokerUrl = "test.mosquitto.org";  // URL do broker   (servidor)
const int port = 1883;                          // Porta do broker (servidor)

String message = "";

WiFiClient espClient;                // Criando Cliente WiFi
PubSubClient mqttClient(espClient);  // Criando Cliente MQTT

// SSID = NOME
// RSSI = Intesidade do sinal

JsonDocument doc;

void connectToWiFi();
void connectToBroker();
void callback(char* topic, byte* payload, unsigned long length);
int lerDistancia();
int lerPot();

void setup() {
  Serial.begin(115200);
  pinMode(echo_pin, INPUT);
  pinMode(trigg_pin, OUTPUT);
  pinMode(pot_pin, INPUT);

  connectToWiFi();
  connectToBroker();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nConexão WiFi perdida");
    connectToWiFi();
  }
  delay(500);
  if (!mqttClient.connected()) {
    Serial.println("\nConexão MQTT perdida");
    connectToBroker();
  }
  delay(500);

  unsigned long agora = millis();

  // --- Leitura da distância ---
  if (agora - ultimoTempoDist >= intervaloDist) {
    ultimoTempoDist = agora;
    valordist = lerDistancia();
    Serial.println("");
    Serial.printf("Distância: %d cm\n", valordist);
    if (valordist <= pot) {
      dist = "proximidade_detectada";
      ultimoTempoDeteccao = millis();
    } else {
      if (millis() - ultimoTempoDeteccao > intervalo) {
        dist = "area_livre";
      }
    }
  }

  // --- Leitura do potenciômetro ---
  if (agora - ultimoTempoPot >= intervaloPot) {
    ultimoTempoPot = agora;
    pot = lerPot();
  }

  if (Serial.available() > 0) {
    String info = "";

    doc["alerta"] = dist;
    doc["distancia"] = valordist;
    doc["distancia_pot"] = pot;
    serializeJson(doc, info);

    message = Serial.readStringUntil('\n');
    //{ "alerta": "proximidade_detectada", "distancia": <valor> }

    // mqttClient.publish("AulaIoTSul/Chat", message.c_str());  // envia mensagem
    mqttClient.publish("duplaLWP/acesso/alerta", info.c_str());
    delay(1000);
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
    mqttClient.subscribe("duplaLWP/acesso/alerta");
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

// le a distância do ultrassonico
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

// le o valor potenciometro
int lerPot() {
  valor = analogRead(pot_pin);
  Serial.printf("Pot: %d\n", valor);
  valor = map(valor, 0, 3240, 0, 100);
  Serial.printf("distancia do pot: %d\n", valor);
  return valor;
}