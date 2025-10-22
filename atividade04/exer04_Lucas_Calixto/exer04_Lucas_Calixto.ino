#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

// pinos
const byte pir_pin = 5;
const byte led_pin = 1;

// variaveis
String movimento = "";
bool mov = false;
int quantmov = 0;
bool resetFlag = false;

unsigned long ultimoTempoPir = 0;
unsigned long ultimoMov = 0;
const long intervaloPir = 1000;
const long janelaMovimento = 5000;

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
int lerPir();

void setup() {
  Serial.begin(115200);
  pinMode(pir_pin, INPUT);
  pinMode(led_pin, OUTPUT);

  connectToWiFi();
  connectToBroker();
}

void loop() {
  if (resetFlag) {
    digitalWrite(led_pin, LOW);  // Apaga LED
    movimento = "";              // limpa movimento
    quantmov = 0;                // zera contagem
    resetFlag = false;           // reseta flag
    Serial.println("RESET EXECUTADO");
  }

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

  // --- Leitura do Pir
  if (agora - ultimoTempoPir >= intervaloPir) {
    unsigned long tempo = millis();
    ultimoTempoPir = agora;
    int valor = lerPir();
    if (valor == 1) {
      quantmov++;
      ultimoMov = agora;

      // se tiver mais de 3 mov e tempo menor que 5s
      if (quantmov >= 3 && (agora - ultimoMov) <= janelaMovimento) {
        movimento = "movimento_insistente";
      } else {
        movimento = "movimento";
      }
      for (int i = 0; i <= 3; i++) {
        digitalWrite(led_pin, LOW);
        delay(100);
        digitalWrite(led_pin, HIGH);
        delay(100);
      }

    } else {
      mov = false;
      quantmov = 0;
      movimento = "";
    }
  }

  if (Serial.available() > 0) {
    String info = "";

    doc["evento"] = movimento;
    serializeJson(doc, info);

    message = Serial.readStringUntil('\n');
    //{ "evento": "movimento" }

    mqttClient.publish("AulaIoTSul/Chat", message.c_str());  // envia mensagem
    mqttClient.publish("duplaLWP/seguranca/eventos", info.c_str());
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
    //mqttClient.subscribe("duplaLWP/seguranca/eventos");
    mqttClient.subscribe("duplaLWP/seguranca/controle");
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

  StaticJsonDocument<200> doc;
  DeserializationError erro = deserializeJson(doc, resposta);

  if (erro) {
    Serial.print("Erro ao ler JSON: ");
    Serial.println(erro.c_str());
    return;
  }

  const char* comando = doc["comando"];

  if (comando && strcmp(comando, "reset") == 0) {
    resetFlag = true;
  }
}

// le o valor Pir
int lerPir() {
  int presenca = digitalRead(pir_pin);
  Serial.printf("Pir: %d\n", presenca);
  Serial.print("");
  return presenca;
}
