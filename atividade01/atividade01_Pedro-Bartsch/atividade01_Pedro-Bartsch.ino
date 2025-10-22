#include <WiFi.h>          // ou <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ======== CONFIGURAÇÕES ========
const char* SSID = "Wellyson";
const char* PSWD = "12345678";
String brokerUrl = "test.mosquitto.org";
int port = 1883;
const char* topico1 = "duplaLWP/servo/comando";

#define LED_PIN 2
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define I2C_SCK 6
#define I2C_SDA 5

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ====== Estrutura ==============
struct SensorData {
  float distancia;
  bool movimento;
  int limiar;
};


// ======== SETUP ========
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCK);
  delay(4000);
  pinMode(LED_PIN, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Falha no display OLED");
    for (;;);
  }
  display.clearDisplay();
  display.display();

  connectWifi();
  connectBroker();
}

// ======== LOOP ========
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Conexão WiFi perdida! Tentando reconexão...");
    connectWifi();
  }
  if (!mqttClient.connected()) {
    Serial.println("MQTT desconectado! Tentando reconexão...");
    connectBroker();
  }

  mqttClient.loop();
}

// ======= FUNÇÃO DE DESERIALIZAÇÃO JSON =============

bool deserializeSensorData(byte* payload, unsigned int length, SensorData &data) {
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("[JSON] Erro ao desserializar: ");
    Serial.println(error.c_str());
    return false;
  }

  const char* distStr = doc["distancia_cm"];
  const char* movStr = doc["movimento"];
  const char* limiarStr = doc["limiar_pot"];
  
  data.distancia = atof(distStr);  
  data.movimento = doc["movimento"].as<bool>();
  data.limiar = atoi(limiarStr); 
  return true;
}

// ======== FUNÇÕES DE CONEXÃO ========
void connectWifi() {
  Serial.println("\n[WiFi] Conectando à rede...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PSWD);
  
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\n[WiFi] Conectado! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[WiFi] Falha ao conectar! Reiniciando...");
    delay(3000);
    ESP.restart();
  }
}

void callbackmsg(char* topic, byte* payload, unsigned int length) {
  
  SensorData data;
  String resposta = "";
  for (int i = 0; i < length; i++) {
    resposta += (char)payload[i];
  }
  Serial.println(resposta);

  if(!deserializeSensorData(payload, length, data)) {
    Serial.println("[ERRO] Falha ao interpretar dados JSON");
    return;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Distancia: ");
  display.print(data.distancia);
  display.println(" cm");

  display.print("Movimento: ");
  display.println(data.movimento ? "Detectado" : "Sem mov.");

  display.print("Limiar: ");
  display.println(data.limiar);
  display.display();

  if (data.distancia < 20 && data.movimento) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  Serial.print("[MQTT] Dados recebidos ->");
  Serial.print("Dist: "); Serial.print(data.distancia);
  Serial.print(" | Mov: "); Serial.print(data.movimento);
  Serial.print(" | Limiar: "); Serial.println(data.limiar);

}

void connectBroker() {
  Serial.println("\n[MQTT] Conectando ao broker...");
  Serial.println(brokerUrl);

  mqttClient.setServer(brokerUrl.c_str(), port);
  mqttClient.setCallback(callbackmsg);

  // gera um ID único a cada conexão
  String clientId = "ESP-PDRO-";
  clientId += String(random(0xffff), HEX);

  // tenta conexão até 10x
  for (int i = 0; i < 10 && !mqttClient.connected(); i++) {
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("[MQTT] Conectado com sucesso!");
      mqttClient.subscribe(topico1);
      return;
    } else {
      Serial.print("[MQTT] Falha (rc=");
      Serial.print(mqttClient.state());
      Serial.println("), tentando novamente...");
      delay(1000);
    }
  }

  Serial.println("[MQTT] Não foi possível conectar. Reiniciando...");
  delay(3000);
  ESP.restart();
}
