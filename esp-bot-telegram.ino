#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

const char* ssid = "MovistarFibra-2C24CC";
const char* password = "2qf3in2FUYudzPtgm2jP";

#define BOTtoken "8465837296:AAFsdJ9tyMZ_ep5dG1bSwdJOJPVzk0iJG9M"
#define CHAT_ID "6053433786"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

unsigned long lastTimeBotRan;
const int ledPin = D1;
unsigned long startMillis;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi");
  Serial.println(WiFi.localIP());

  client.setInsecure(); // Ignora certificado SSL
  bot.sendMessage(CHAT_ID, "🤖 ESP8266 conectado a WiFi", "");

  startMillis = millis();
}

void loop() {
  if (millis() - lastTimeBotRan > 1000) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String text = bot.messages[i].text;
    String chat_id = bot.messages[i].chat_id;
    String from_name = bot.messages[i].from_name;

    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "⛔ No tenés permiso para usar este bot.", "");
      return;
    }

    if (text == "/start") {
      String welcome = "Hola " + from_name + " 👋\n";
      welcome += "Comandos disponibles:\n";
      welcome += "/ledon - Encender LED 💡\n";
      welcome += "/ledoff - Apagar LED ❌\n";
      welcome += "/estado - Estado del sistema ✅\n";
      welcome += "/ip - Ver IP 📡\n";
      welcome += "/uptime - Tiempo encendido ⏱\n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/ledon") {
      digitalWrite(ledPin, HIGH);
      bot.sendMessage(chat_id, "LED encendido 💡", "");
    }

    if (text == "/ledoff") {
      digitalWrite(ledPin, LOW);
      bot.sendMessage(chat_id, "LED apagado ❌", "");
    }

    if (text == "/estado") {
      unsigned long uptimeSecs = (millis() - startMillis) / 1000;
      bot.sendMessage(chat_id, "✅ El sistema está online\nIP: " + WiFi.localIP().toString() + "\nUptime: " + String(uptimeSecs) + " segundos", "");
    }

    if (text == "/ip") {
      bot.sendMessage(chat_id, "📡 IP Local: " + WiFi.localIP().toString(), "");
    }

    if (text == "/uptime") {
      unsigned long uptimeSecs = (millis() - startMillis) / 1000;
      bot.sendMessage(chat_id, "⏱ Uptime: " + String(uptimeSecs) + " segundos", "");
    }
  }
}
