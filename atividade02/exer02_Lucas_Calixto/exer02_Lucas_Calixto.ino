#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// variaveis

const byte pot_pin = 4;

int pot = 0;

// const char* SSID = "Redmi 8 do lucas";
// const char* PSWD = "12345678";

const char* SSID = "iPhone";
const char* PSWD = "123456789";

const char* brokerUrl = "test.mosquitto.org";
const int port = 1883;

unsigned long ultimoTempoPot = 0;
const long intervaloPot = 1000;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

JsonDocument doc;

void connectToWiFi();
void connectToBroker();
void callback(char* topic, byte* payload, unsigned long length);
int lerPot();

void setup() {
  Serial.begin(115200);
  pinMode(pot_pin, INPUT);

  connectToWiFi();
  connectToBroker();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectToWiFi();
  if (!mqttClient.connected()) connectToBroker();
  mqttClient.loop();

  unsigned long agora = millis();

  // --- Leitura do potenciômetro ---
  if (agora - ultimoTempoPot >= intervaloPot) {
    ultimoTempoPot = agora;
    pot = lerPot();
  }

  // --- Envio manual pelo Serial ---
  if (Serial.available() > 0) {
  String info = "";

  doc["angulo"] = pot;
  serializeJson(doc, info);

  String msg = Serial.readStringUntil('\n');

  // Msg enviada:
  //{ "angulo": <valor> }
  mqttClient.publish("duplaLWP/servo/comando", info.c_str());
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
      mqttClient.subscribe("duplaLWP/servo/comando");
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


int lerPot() {
  int valor = analogRead(pot_pin);
  Serial.printf("Pot: %d\n", valor);
  valor = map(valor, 0, 3240, 0, 180);
  Serial.printf("Angulo: %d\n", valor);
  return valor;
}
