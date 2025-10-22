#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== CONFIGURAÇÕES WIFI/MQTT =====
const char* SSID = "iPhone";
const char* PSWD = "123456789";
const char* BROKER = "test.mosquitto.org";
const int PORT = 1883;
const char* TOPICO = "duplaLWP/acesso/alerta";

// PINOS
 
#define SERVO_PIN 9
#define LED_PIN 2
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define I2C_SCK 5
#define I2C_SDA 6

// OBJETOS
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Servo servo;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// TÓPICOS MQTT
const char* TOPICO_ALERTA = "duplaLWP/acesso/alerta";

String estadoAtual = "Area Livre";
int limiarAtual = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCK);
  servo.attach(SERVO_PIN);
  pinMode(LED_PIN, OUTPUT);

 if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Falha no OLED");
    for (;;);
  }

  display.clearDisplay();
  display.display();

  connectWifi();
  connectBroker();

  servo.write(0);
  atualizarDisplay();
}

void loop() {
  if(!mqttClient.connected()) connectBroker();
  mqttClient.loop();

}

// FUNÇÕES
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

void connectBroker() {
  Serial.println("\n[MQTT] Conectando ao broker...");
  Serial.println(BROKER);

  mqttClient.setServer(BROKER, PORT);
  mqttClient.setCallback(callbackMsg);

  // gera um ID único a cada conexão
  String clientId = "ESP-PDRO-";
  clientId += String(random(0xffff), HEX);

  // tenta conexão até 10x
  for (int i = 0; i < 10 && !mqttClient.connected(); i++) {
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("[MQTT] Conectado com sucesso!");
      mqttClient.subscribe(TOPICO);
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

void atualizarDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(" CONTROLE DE ACESSO ");
  display.setCursor(0, 20);
  display.print("Estado: ");
  display.println(estadoAtual);
  display.setCursor(0, 40);
  display.print("Limiar: ");
  display.print(limiarAtual);
  display.println(" cm");
  display.display();
}

void callbackMsg(char* topic, byte* payload, unsigned int length) {

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("[JSON] Erro ao deserializar: ");
    Serial.println(error.c_str());
    return;
  }
  String status = doc["alerta"].as<String>();  
  int distancia = doc["distancia"].as<int>(); 
  limiarAtual = doc["distancia_pot"].as<int>();

  Serial.println("Status: " + status);
  Serial.print("Distancia: "); Serial.println(distancia);
  Serial.print("Limiar: "); Serial.println(limiarAtual);

  if (status == "proximidade_detectada") {
    
      servo.write(90);            // Fecha
      digitalWrite(LED_PIN, HIGH);
      estadoAtual = "Acesso FECHADO";
  }
  else if (status == "area_livre") {
      servo.write(0);             // Abre
      digitalWrite(LED_PIN, LOW);
      estadoAtual = "Acesso ABERTO";
  }
  atualizarDisplay();
}

