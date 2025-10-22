#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// CONFIGURAÇÕES WIFI/MQTT 
const char* SSID = "iPhone";
const char* PSWD = "123456789";
const char* BROKER = "test.mosquitto.org";
const int PORT = 1883;

// TOPICOS
const char* TOPICO_EVENTOS = "duplaLWP/seguranca/eventos";

// PINOS
#define LED_AMARELO 10
#define LED_VERDE 11
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define I2C_SCK 5
#define I2C_SDA 6

// OBJETOS
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// SETUP
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCK);
  delay(1000);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Falha no OLED");
    for (;;);
  }
  display.clearDisplay();
  display.display();

  connectWifi();
  connectBroker();
}

// LOOP
void loop() {
  if (!mqttClient.connected()) connectBroker();
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
      mqttClient.subscribe(TOPICO_EVENTOS);
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