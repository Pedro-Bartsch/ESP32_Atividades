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
const char* TOPICO_CONTROLE = "duplaLWP/seguranca/controle";

// PINOS
#define LED_PIN 10
#define POT_PIN 1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define I2C_SCK 5
#define I2C_SDA 6

// OBJETOS
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// VARIAVEIS
bool alertaAtivo = false;
bool piscar = false;
unsigned long tempoAlerta = 0;
unsigned long tempoReset = 0;


// SETUP
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCK);
  delay(1000);
  pinMode(LED_PIN, OUTPUT);

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

  if (alertaAtivo && piscar) {
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink >= 200) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastBlink = millis();
    }
  }
  if (alertaAtivo && millis() - tempoAlerta >= tempoReset){
    enviarReset();
  }
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

void callbackMsg(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  StaticJsonDocument<64> doc;
  deserializeJson(doc,msg);
  String evento = doc["evento"] | "";

  if (evento == "movimento" || evento == "movimento_insistente") {
  
  alertaAtivo = true;
  piscar = (evento == "movimento_insistente");

  if (!piscar) digitalWrite (LED_PIN, HIGH);

  int potValue = analogRead(POT_PIN);
  tempoReset = map(potValue, 0, 4095, 0, 30000);
  tempoAlerta = millis();

  mostrarAlerta(evento == "movimento" ?
  "ALERTA: Movimento Detectado!" :
  "ALERTA: Movimento Insistente!");

  Serial.printf("[ALERTA] Evento: %s | Reset em %.1f s\n",
        evento.c_str(), tempoReset / 1000.0);
  }
}
void mostrarAlerta(const char* texto){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println(texto);
  display.display();  
}

void limparDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println("Sistema em espera...");
  display.display();
}

void enviarReset() {
StaticJsonDocument<64> doc;
  doc["comando"] = "reset";
  char buffer[64];
  serializeJson(doc, buffer);
  mqttClient.publish(TOPICO_CONTROLE, buffer);
  Serial.println("[MQTT] Enviado comando de reset");

  alertaAtivo = false;
  piscar = false;
  digitalWrite(LED_PIN, LOW);
  limparDisplay();
}