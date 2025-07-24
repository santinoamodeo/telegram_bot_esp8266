#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// Configuración WiFi
const char* ssid = "MovistarFibra-2C24CC";
const char* password = "2qf3in2FUYudzPtgm2jP";

// Configuración Telegram
#define BOTtoken "8465837296:AAFsdJ9tyMZ_ep5dG1bSwdJOJPVzk0iJG9M"
#define CHAT_ID "6053433786"

// Configuración de pines
const int ledPin = D1;
const int sensorPin = A0; // Pin analógico para sensor (opcional)

// Objetos principales
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Variables de tiempo
unsigned long lastTimeBotRan;
unsigned long startMillis;
unsigned long lastHeartbeat = 0;
const unsigned long heartbeatInterval = 300000; // 5 minutos

// Variables de estado
bool ledState = false;
bool autoMode = false;
int messageCount = 0;
float lastTemperature = 0.0;

void setup() {
  Serial.begin(115200);
  
  // Configuración de pines
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // Conexión WiFi con indicador visual
  connectToWiFi();
  
  // Configuración SSL
  client.setInsecure();
  
  // Mensaje de inicio con información del sistema
  sendStartupMessage();
  
  startMillis = millis();
}

void loop() {
  // Verificar conexión WiFi
  if (WiFi.status() != WL_CONNECTED) {
    reconnectWiFi();
  }
  
  // Procesar mensajes de Telegram
  if (millis() - lastTimeBotRan > 1000) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  
  // Heartbeat automático cada 5 minutos
  if (millis() - lastHeartbeat > heartbeatInterval) {
    sendHeartbeat();
    lastHeartbeat = millis();
  }
  
  // Modo automático (ejemplo: parpadeo LED)
  if (autoMode) {
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 1000) {
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
      lastBlink = millis();
    }
  }
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.print("🔗 Conectando a WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Conectado a WiFi");
    Serial.println("📡 IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n❌ Error al conectar WiFi");
    ESP.restart();
  }
}

void reconnectWiFi() {
  Serial.println("🔄 Reconectando WiFi...");
  WiFi.reconnect();
  delay(5000);
}

void sendStartupMessage() {
  String startupMsg = "🚀 **ESP8266 Bot Iniciado**\n\n";
  startupMsg += "📊 **Información del Sistema:**\n";
  startupMsg += "• IP: `" + WiFi.localIP().toString() + "`\n";
  startupMsg += "• MAC: `" + WiFi.macAddress() + "`\n";
  startupMsg += "• Chip ID: `" + String(ESP.getChipId()) + "`\n";
  startupMsg += "• Memoria libre: `" + String(ESP.getFreeHeap()) + " bytes`\n";
  startupMsg += "⌨️ Escribe /help para ver comandos disponibles";
  
  bot.sendMessage(CHAT_ID, startupMsg, "Markdown");
}

void sendHeartbeat() {
  String heartbeatMsg = "💓 **Sistema Activo**\n";
  heartbeatMsg += formatUptime();
  heartbeatMsg += "📊 Memoria libre: `" + String(ESP.getFreeHeap()) + " bytes`\n";
  heartbeatMsg += "📨 Mensajes procesados: `" + String(messageCount) + "`";
  
  bot.sendMessage(CHAT_ID, heartbeatMsg, "Markdown");
}

String formatUptime() {
  unsigned long uptimeSecs = (millis() - startMillis) / 1000;
  unsigned long days = uptimeSecs / 86400;
  unsigned long hours = (uptimeSecs % 86400) / 3600;
  unsigned long minutes = (uptimeSecs % 3600) / 60;
  unsigned long seconds = uptimeSecs % 60;
  
  String uptime = "⏱ **Uptime:** ";
  if (days > 0) uptime += String(days) + "d ";
  if (hours > 0) uptime += String(hours) + "h ";
  if (minutes > 0) uptime += String(minutes) + "m ";
  uptime += String(seconds) + "s\n";
  
  return uptime;
}

String getSystemStatus() {
  String status = "🔧 **Estado del Sistema**\n\n";
  status += "📡 **Red:**\n";
  status += "• IP: `" + WiFi.localIP().toString() + "`\n";
  status += "• RSSI: `" + String(WiFi.RSSI()) + " dBm`\n";
  status += "• Canal: `" + String(WiFi.channel()) + "`\n\n";
  
  status += "💾 **Memoria:**\n";
  status += "• Libre: `" + String(ESP.getFreeHeap()) + " bytes`\n";
  status += "• Fragmentación: `" + String(ESP.getHeapFragmentation()) + "%`\n\n";
  
  status += "⚡ **Hardware:**\n";
  status += "• Frecuencia CPU: `" + String(ESP.getCpuFreqMHz()) + " MHz`\n";
  status += "• LED: " + String(ledState ? "🟢 ON" : "🔴 OFF") + "\n";
  status += "• Modo auto: " + String(autoMode ? "🟢 ON" : "🔴 OFF") + "\n\n";
  
  status += formatUptime();
  status += "📨 Mensajes: `" + String(messageCount) + "`";
  
  return status;
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String text = bot.messages[i].text;
    String chat_id = bot.messages[i].chat_id;
    String from_name = bot.messages[i].from_name;
    
    messageCount++;
    
    // Verificar permisos
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "⛔ **Acceso Denegado**\n\nNo tienes permisos para usar este bot.", "Markdown");
      return;
    }
    
    // Comandos disponibles
    if (text == "/start" || text == "/help") {
      String welcome = "👋 **Hola " + from_name + "!**\n\n";
      welcome += "🤖 **ESP8266 Telegram Bot v2.0**\n\n";
      welcome += "📋 **Comandos disponibles:**\n\n";
      welcome += "🔆 **Control LED:**\n";
      welcome += "/ledon - Encender LED\n";
      welcome += "/ledoff - Apagar LED\n";
      welcome += "/ledtoggle - Alternar LED\n";
      welcome += "/automode - Modo automático\n\n";
      welcome += "📊 **Información:**\n";
      welcome += "/status - Estado completo\n";
      welcome += "/ip - Dirección IP\n";
      welcome += "/uptime - Tiempo activo\n";
      welcome += "/memory - Uso de memoria\n";
      welcome += "/wifi - Info WiFi\n\n";
      welcome += "🔧 **Sistema:**\n";
      welcome += "/heartbeat - Toggle heartbeat\n";
      welcome += "/version - Información versión";
      
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
    
    else if (text == "/ledon") {
      digitalWrite(ledPin, HIGH);
      ledState = true;
      autoMode = false;
      bot.sendMessage(chat_id, "💡 **LED Encendido**\n\n✅ Estado actualizado correctamente", "Markdown");
    }
    
    else if (text == "/ledoff") {
      digitalWrite(ledPin, LOW);
      ledState = false;
      autoMode = false;
      bot.sendMessage(chat_id, "🔴 **LED Apagado**\n\n✅ Estado actualizado correctamente", "Markdown");
    }
    
    else if (text == "/ledtoggle") {
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
      autoMode = false;
      String msg = ledState ? "💡 **LED Encendido**" : "🔴 **LED Apagado**";
      msg += "\n\n🔄 Estado alternado correctamente";
      bot.sendMessage(chat_id, msg, "Markdown");
    }
    
    else if (text == "/automode") {
      autoMode = !autoMode;
      String msg = "🔄 **Modo Automático: " + String(autoMode ? "ACTIVADO" : "DESACTIVADO") + "**\n\n";
      if (autoMode) {
        msg += "🔆 El LED parpadeará automáticamente cada segundo";
      } else {
        msg += "⏹ Modo manual restaurado";
        digitalWrite(ledPin, LOW);
        ledState = false;
      }
      bot.sendMessage(chat_id, msg, "Markdown");
    }
    
    else if (text == "/status") {
      bot.sendMessage(chat_id, getSystemStatus(), "Markdown");
    }
    
    else if (text == "/ip") {
      String ipMsg = "📡 **Información de Red**\n\n";
      ipMsg += "• IP Local: `" + WiFi.localIP().toString() + "`\n";
      ipMsg += "• Gateway: `" + WiFi.gatewayIP().toString() + "`\n";
      ipMsg += "• DNS: `" + WiFi.dnsIP().toString() + "`\n";
      ipMsg += "• Máscara: `" + WiFi.subnetMask().toString() + "`";
      bot.sendMessage(chat_id, ipMsg, "Markdown");
    }
    
    else if (text == "/uptime") {
      String uptimeMsg = "⏱ **Tiempo de Actividad**\n\n";
      uptimeMsg += formatUptime();
      uptimeMsg += "🚀 Iniciado: " + String((millis() - startMillis) / 1000) + " segundos atrás";
      bot.sendMessage(chat_id, uptimeMsg, "Markdown");
    }
    
    else if (text == "/memory") {
      String memMsg = "💾 **Información de Memoria**\n\n";
      memMsg += "• Memoria libre: `" + String(ESP.getFreeHeap()) + " bytes`\n";
      memMsg += "• Fragmentación: `" + String(ESP.getHeapFragmentation()) + "%`\n";
      memMsg += "• Tamaño máximo bloque: `" + String(ESP.getMaxFreeBlockSize()) + " bytes`";
      bot.sendMessage(chat_id, memMsg, "Markdown");
    }
    
    else if (text == "/wifi") {
      String wifiMsg = "📶 **Información WiFi**\n\n";
      wifiMsg += "• SSID: `" + WiFi.SSID() + "`\n";
      wifiMsg += "• RSSI: `" + String(WiFi.RSSI()) + " dBm`\n";
      wifiMsg += "• Canal: `" + String(WiFi.channel()) + "`\n";
      wifiMsg += "• MAC: `" + WiFi.macAddress() + "`\n";
      wifiMsg += "• Modo: `" + String(WiFi.getMode()) + "`";
      bot.sendMessage(chat_id, wifiMsg, "Markdown");
    }
    
    else if (text == "/version") {
      String versionMsg = "ℹ️ **Información del Sistema**\n\n";
      versionMsg += "• Bot Version: `2.0`\n";
      versionMsg += "• Chip ID: `" + String(ESP.getChipId()) + "`\n";
      versionMsg += "• SDK Version: `" + String(ESP.getSdkVersion()) + "`\n";
      versionMsg += "• Core Version: `" + String(ESP.getCoreVersion()) + "`\n";
      versionMsg += "• Boot Version: `" + String(ESP.getBootVersion()) + "`";
      bot.sendMessage(chat_id, versionMsg, "Markdown");
    }
    
    else {
      String unknownMsg = "❓ **Comando no reconocido**\n\n";
      unknownMsg += "Escribe `/help` para ver los comandos disponibles.\n\n";
      unknownMsg += "📝 Comando recibido: `" + text + "`";
      bot.sendMessage(chat_id, unknownMsg, "Markdown");
    }
  }
}
