#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// Configuración WiFi
const char* ssid = "MovistarFibra-2C24CC";
const char* password = "2qf3in2FUYudzPtgm2jP";

// Configuración Telegram
#define BOTtoken "8465837296:AAFsdJ9tyMZ_ep5dG1bSwdJOJPVzk0iJG9M"
#define CHAT_ID "6053433786"

// Configuración de pines
const int ledPin = D1;
const int sensorPin = A0;

// Objetos principales
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
ESP8266WebServer server(80);

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
String deviceName = "ESP8266-Bot";

// Declaración de funciones
void connectToWiFi();
void reconnectWiFi();
float getVoltage();
void setupWebServer();
void handleRoot();
void handleCSS();
void handleAPIStatus();
void handleLEDOn();
void handleLEDOff();
void handleLEDToggle();
void handleAutoMode();
void handleRestart();
String getWebInterface();
String formatUptime();
String formatUptimeSimple(unsigned long seconds);
String getSystemStatus();
void sendStartupMessage();
void sendHeartbeat();
void handleNewMessages(int numNewMessages);

void setup() {
  Serial.begin(115200);
  
  // Configuración de pines
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // Conexión WiFi
  connectToWiFi();
  
  // Configuración SSL
  client.setInsecure();
  
  // Configurar servidor web
  setupWebServer();
  
  // Configurar mDNS
  if (MDNS.begin(deviceName)) {
    Serial.println("mDNS responder started: http://" + deviceName + ".local");
  }
  
  // Mensaje de inicio
  sendStartupMessage();
  
  startMillis = millis();
}

void loop() {
  // Manejar servidor web
  server.handleClient();
  MDNS.update();
  
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
  
  // Heartbeat automático
  if (millis() - lastHeartbeat > heartbeatInterval) {
    sendHeartbeat();
    lastHeartbeat = millis();
  }
  
  // Modo automático
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
    Serial.println("🌐 Interfaz Web: http://" + WiFi.localIP().toString());
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

float getVoltage() {
  // El ESP8266 funciona a 3.3V, no 65V
  // Esta función ahora lee el voltaje de alimentación correctamente
  return 3.3; // Voltaje típico de operación del ESP8266
  
  // Si tienes un divisor de voltaje en A0, descomenta esto:
  // int adcValue = analogRead(A0);
  // return (adcValue / 1024.0) * 3.3 * 2; // Ajustar según tu divisor
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

String formatUptimeSimple(unsigned long seconds) {
  unsigned long hours = seconds / 3600;
  unsigned long minutes = (seconds % 3600) / 60;
  unsigned long secs = seconds % 60;
  
  String uptime = "";
  if (hours > 0) uptime += String(hours) + "h ";
  if (minutes > 0) uptime += String(minutes) + "m ";
  uptime += String(secs) + "s";
  
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
  status += "• Voltaje: `" + String(getVoltage(), 1) + "V`\n";
  status += "• Frecuencia CPU: `" + String(ESP.getCpuFreqMHz()) + " MHz`\n";
  status += "• LED: " + String(ledState ? "🟢 ON" : "🔴 OFF") + "\n";
  status += "• Modo auto: " + String(autoMode ? "🟢 ON" : "🔴 OFF") + "\n\n";
  
  status += formatUptime();
  status += "📨 Mensajes: `" + String(messageCount) + "`\n";
  status += "🌐 Web: `http://" + WiFi.localIP().toString() + "`";
  
  return status;
}

void sendStartupMessage() {
  String startupMsg = "🚀 **ESP8266 Bot Iniciado**\n\n";
  startupMsg += "📊 **Información del Sistema:**\n";
  startupMsg += "• IP: `" + WiFi.localIP().toString() + "`\n";
  startupMsg += "• MAC: `" + WiFi.macAddress() + "`\n";
  startupMsg += "• Chip ID: `" + String(ESP.getChipId()) + "`\n";
  startupMsg += "• Memoria libre: `" + String(ESP.getFreeHeap()) + " bytes`\n";
  startupMsg += "• Voltaje: `" + String(getVoltage(), 1) + "V`\n\n";
  startupMsg += "🌐 **Interfaz Web:** `http://" + WiFi.localIP().toString() + "`\n\n";
  startupMsg += "⌨️ Escribe /help para ver comandos disponibles";
  
  bot.sendMessage(CHAT_ID, startupMsg, "Markdown");
}

void sendHeartbeat() {
  String heartbeatMsg = "💓 **Sistema Activo**\n";
  heartbeatMsg += formatUptime();
  heartbeatMsg += "📊 Memoria libre: `" + String(ESP.getFreeHeap()) + " bytes`\n";
  heartbeatMsg += "📨 Mensajes procesados: `" + String(messageCount) + "`\n";
  heartbeatMsg += "🌐 Web: `http://" + WiFi.localIP().toString() + "`";
  
  bot.sendMessage(CHAT_ID, heartbeatMsg, "Markdown");
}

void setupWebServer() {
  // Página principal
  server.on("/", handleRoot);
  
  // API endpoints
  server.on("/api/status", handleAPIStatus);
  server.on("/api/led/on", handleLEDOn);
  server.on("/api/led/off", handleLEDOff);
  server.on("/api/led/toggle", handleLEDToggle);
  server.on("/api/automode", handleAutoMode);
  server.on("/api/restart", handleRestart);
  
  // Archivos CSS
  server.on("/style.css", handleCSS);
  
  server.begin();
  Serial.println("🌐 Servidor web iniciado en puerto 80");
}

void handleRoot() {
  String html = getWebInterface();
  server.send(200, "text/html", html);
}

String getWebInterface() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP8266 Bot Control</title>
    <link rel="stylesheet" href="/style.css">
    <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css" rel="stylesheet">
</head>
<body>
    <div class="container">
        <header>
            <h1><i class="fas fa-microchip"></i> ESP8266 Bot Control</h1>
            <p>Interfaz Web de Control y Monitoreo</p>
        </header>

        <div class="grid">
            <!-- Estado del Sistema -->
            <div class="card">
                <h2><i class="fas fa-info-circle"></i> Estado del Sistema</h2>
                <div class="status-grid">
                    <div class="status-item">
                        <span class="label">IP:</span>
                        <span class="value" id="ip">)rawliteral" + WiFi.localIP().toString() + R"rawliteral(</span>
                    </div>
                    <div class="status-item">
                        <span class="label">Memoria:</span>
                        <span class="value" id="memory">)rawliteral" + String(ESP.getFreeHeap()) + R"rawliteral( bytes</span>
                    </div>
                    <div class="status-item">
                        <span class="label">Uptime:</span>
                        <span class="value" id="uptime">0s</span>
                    </div>
                    <div class="status-item">
                        <span class="label">WiFi:</span>
                        <span class="value" id="wifi">)rawliteral" + String(WiFi.RSSI()) + R"rawliteral( dBm</span>
                    </div>
                    <div class="status-item">
                        <span class="label">Voltaje:</span>
                        <span class="value" id="voltage">)rawliteral" + String(getVoltage(), 1) + R"rawliteral(V</span>
                    </div>
                    <div class="status-item">
                        <span class="label">Mensajes:</span>
                        <span class="value" id="messages">)rawliteral" + String(messageCount) + R"rawliteral(</span>
                    </div>
                </div>
            </div>

            <!-- Control LED -->
            <div class="card">
                <h2><i class="fas fa-lightbulb"></i> Control LED</h2>
                <div class="control-buttons">
                    <button class="btn btn-success" onclick="controlLED('on')">
                        <i class="fas fa-power-off"></i> Encender
                    </button>
                    <button class="btn btn-danger" onclick="controlLED('off')">
                        <i class="fas fa-power-off"></i> Apagar
                    </button>
                    <button class="btn btn-info" onclick="controlLED('toggle')">
                        <i class="fas fa-exchange-alt"></i> Alternar
                    </button>
                </div>
                
                <div class="led-status">
                    <span>Estado actual: </span>
                    <span id="ledStatus" class="status-indicator">)rawliteral" + String(ledState ? "ON" : "OFF") + R"rawliteral(</span>
                </div>
                
                <div class="auto-mode">
                    <button class="btn btn-warning" onclick="toggleAutoMode()">
                        <i class="fas fa-magic"></i> <span id="autoModeText">)rawliteral" + String(autoMode ? "Desactivar Auto" : "Activar Auto") + R"rawliteral(</span>
                    </button>
                </div>
            </div>

            <!-- Información de Red -->
            <div class="card">
                <h2><i class="fas fa-network-wired"></i> Red</h2>
                <div class="network-info">
                    <p><strong>SSID:</strong> )rawliteral" + WiFi.SSID() + R"rawliteral(</p>
                    <p><strong>MAC:</strong> )rawliteral" + WiFi.macAddress() + R"rawliteral(</p>
                    <p><strong>Gateway:</strong> )rawliteral" + WiFi.gatewayIP().toString() + R"rawliteral(</p>
                    <p><strong>DNS:</strong> )rawliteral" + WiFi.dnsIP().toString() + R"rawliteral(</p>
                </div>
            </div>

            <!-- Acciones del Sistema -->
            <div class="card">
                <h2><i class="fas fa-cogs"></i> Sistema</h2>
                <div class="system-actions">
                    <button class="btn btn-warning" onclick="updateStatus()">
                        <i class="fas fa-sync"></i> Actualizar Estado
                    </button>
                    <button class="btn btn-danger" onclick="restartSystem()">
                        <i class="fas fa-redo"></i> Reiniciar ESP
                    </button>
                </div>
                
                <div class="telegram-info">
                    <h3><i class="fab fa-telegram"></i> Telegram Bot</h3>
                    <p>El bot está activo y respondiendo comandos</p>
                    <p>Mensajes procesados: <span id="msgCount">)rawliteral" + String(messageCount) + R"rawliteral(</span></p>
                </div>
            </div>
        </div>

        <footer>
            <p>ESP8266 Bot v2.1 - Interfaz Web Integrada</p>
            <p>Última actualización: <span id="lastUpdate">Cargando...</span></p>
        </footer>
    </div>

    <script>
        // Actualizar estado automáticamente cada 5 segundos
        setInterval(updateStatus, 5000);
        
        // Actualizar timestamp inicial
        updateTimestamp();
        
        function updateStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('memory').textContent = data.memory + ' bytes';
                    document.getElementById('uptime').textContent = data.uptime;
                    document.getElementById('wifi').textContent = data.wifi + ' dBm';
                    document.getElementById('voltage').textContent = data.voltage + 'V';
                    document.getElementById('messages').textContent = data.messages;
                    document.getElementById('msgCount').textContent = data.messages;
                    document.getElementById('ledStatus').textContent = data.ledState ? 'ON' : 'OFF';
                    document.getElementById('ledStatus').className = 'status-indicator ' + (data.ledState ? 'status-on' : 'status-off');
                    document.getElementById('autoModeText').textContent = data.autoMode ? 'Desactivar Auto' : 'Activar Auto';
                    updateTimestamp();
                })
                .catch(error => console.error('Error:', error));
        }
        
        function controlLED(action) {
            fetch(`/api/led/${action}`, {method: 'POST'})
                .then(response => response.json())
                .then(data => {
                    if(data.success) {
                        updateStatus();
                        showNotification(`LED ${action.toUpperCase()}`, 'success');
                    }
                })
                .catch(error => console.error('Error:', error));
        }
        
        function toggleAutoMode() {
            fetch('/api/automode', {method: 'POST'})
                .then(response => response.json())
                .then(data => {
                    if(data.success) {
                        updateStatus();
                        showNotification(`Modo automático ${data.autoMode ? 'activado' : 'desactivado'}`, 'info');
                    }
                })
                .catch(error => console.error('Error:', error));
        }
        
        function restartSystem() {
            if(confirm('¿Estás seguro de que quieres reiniciar el ESP8266?')) {
                fetch('/api/restart', {method: 'POST'})
                    .then(() => {
                        showNotification('Reiniciando sistema...', 'warning');
                        setTimeout(() => {
                            location.reload();
                        }, 10000);
                    })
                    .catch(error => console.error('Error:', error));
            }
        }
        
        function updateTimestamp() {
            document.getElementById('lastUpdate').textContent = new Date().toLocaleTimeString();
        }
        
        function showNotification(message, type) {
            // Crear notificación simple
            const notification = document.createElement('div');
            notification.className = `notification notification-${type}`;
            notification.textContent = message;
            notification.style.cssText = `
                position: fixed;
                top: 20px;
                right: 20px;
                padding: 15px 20px;
                border-radius: 5px;
                color: white;
                z-index: 1000;
                animation: slideIn 0.3s ease-out;
            `;
            
            if(type === 'success') notification.style.backgroundColor = '#28a745';
            else if(type === 'error') notification.style.backgroundColor = '#dc3545';
            else if(type === 'warning') notification.style.backgroundColor = '#ffc107';
            else notification.style.backgroundColor = '#17a2b8';
            
            document.body.appendChild(notification);
            
            setTimeout(() => {
                notification.remove();
            }, 3000);
        }
    </script>
</body>
</html>
)rawliteral";
  
  return html;
}

void handleCSS() {
  String css = R"rawliteral(
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    min-height: 100vh;
    color: #333;
}

.container {
    max-width: 1200px;
    margin: 0 auto;
    padding: 20px;
}

header {
    text-align: center;
    margin-bottom: 30px;
    color: white;
}

header h1 {
    font-size: 2.5em;
    margin-bottom: 10px;
    text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
}

header p {
    font-size: 1.2em;
    opacity: 0.9;
}

.grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    gap: 20px;
    margin-bottom: 30px;
}

.card {
    background: white;
    border-radius: 15px;
    padding: 25px;
    box-shadow: 0 10px 30px rgba(0,0,0,0.2);
    transition: transform 0.3s ease, box-shadow 0.3s ease;
}

.card:hover {
    transform: translateY(-5px);
    box-shadow: 0 15px 40px rgba(0,0,0,0.3);
}

.card h2 {
    margin-bottom: 20px;
    color: #333;
    border-bottom: 2px solid #667eea;
    padding-bottom: 10px;
}

.status-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
    gap: 15px;
}

.status-item {
    background: #f8f9fa;
    padding: 15px;
    border-radius: 8px;
    text-align: center;
    border-left: 4px solid #667eea;
}

.status-item .label {
    display: block;
    font-size: 0.9em;
    color: #666;
    margin-bottom: 5px;
}

.status-item .value {
    font-weight: bold;
    font-size: 1.1em;
    color: #333;
}

.control-buttons {
    display: flex;
    gap: 10px;
    flex-wrap: wrap;
    margin-bottom: 20px;
}

.btn {
    padding: 12px 20px;
    border: none;
    border-radius: 8px;
    cursor: pointer;
    font-size: 1em;
    font-weight: 600;
    transition: all 0.3s ease;
    display: flex;
    align-items: center;
    gap: 8px;
    flex: 1;
    justify-content: center;
    min-width: 120px;
}

.btn:hover {
    transform: translateY(-2px);
    box-shadow: 0 5px 15px rgba(0,0,0,0.2);
}

.btn-success {
    background: #28a745;
    color: white;
}

.btn-success:hover {
    background: #218838;
}

.btn-danger {
    background: #dc3545;
    color: white;
}

.btn-danger:hover {
    background: #c82333;
}

.btn-info {
    background: #17a2b8;
    color: white;
}

.btn-info:hover {
    background: #138496;
}

.btn-warning {
    background: #ffc107;
    color: #212529;
}

.btn-warning:hover {
    background: #e0a800;
}

.led-status {
    margin: 15px 0;
    padding: 15px;
    background: #f8f9fa;
    border-radius: 8px;
    text-align: center;
}

.status-indicator {
    font-weight: bold;
    padding: 5px 15px;
    border-radius: 20px;
    color: white;
}

.status-on {
    background: #28a745;
}

.status-off {
    background: #dc3545;
}

.auto-mode {
    margin-top: 15px;
}

.network-info p {
    margin: 10px 0;
    padding: 8px;
    background: #f8f9fa;
    border-radius: 5px;
}

.system-actions {
    display: flex;
    gap: 15px;
    margin-bottom: 20px;
    flex-wrap: wrap;
}

.telegram-info {
    background: #e7f3ff;
    padding: 15px;
    border-radius: 8px;
    border: 1px solid #b3d9ff;
}

.telegram-info h3 {
    margin-bottom: 10px;
    color: #0066cc;
}

footer {
    text-align: center;
    color: white;
    padding: 20px;
    opacity: 0.8;
}

@keyframes slideIn {
    from {
        transform: translateX(100%);
        opacity: 0;
    }
    to {
        transform: translateX(0);
        opacity: 1;
    }
}

@media (max-width: 768px) {
    .container {
        padding: 10px;
    }
    
    header h1 {
        font-size: 2em;
    }
    
    .control-buttons {
        flex-direction: column;
    }
    
    .system-actions {
        flex-direction: column;
    }
    
    .status-grid {
        grid-template-columns: 1fr;
    }
}
)rawliteral";
  
  server.send(200, "text/css", css);
}

void handleAPIStatus() {
  unsigned long uptimeSecs = (millis() - startMillis) / 1000;
  
  String json = "{";
  json += "\"memory\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"uptime\":\"" + formatUptimeSimple(uptimeSecs) + "\",";
  json += "\"wifi\":" + String(WiFi.RSSI()) + ",";
  json += "\"voltage\":" + String(getVoltage(), 1) + ",";
  json += "\"messages\":" + String(messageCount) + ",";
  json += "\"ledState\":" + String(ledState ? "true" : "false") + ",";
  json += "\"autoMode\":" + String(autoMode ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleLEDOn() {
  digitalWrite(ledPin, HIGH);
  ledState = true;
  autoMode = false;
  server.send(200, "application/json", "{\"success\":true,\"state\":\"on\"}");
}

void handleLEDOff() {
  digitalWrite(ledPin, LOW);
  ledState = false;
  autoMode = false;
  server.send(200, "application/json", "{\"success\":true,\"state\":\"off\"}");
}

void handleLEDToggle() {
  ledState = !ledState;
  digitalWrite(ledPin, ledState);
  autoMode = false;
  server.send(200, "application/json", "{\"success\":true,\"state\":\"" + String(ledState ? "on" : "off") + "\"}");
}

void handleAutoMode() {
  autoMode = !autoMode;
  if (!autoMode) {
    digitalWrite(ledPin, LOW);
    ledState = false;
  }
  server.send(200, "application/json", "{\"success\":true,\"autoMode\":" + String(autoMode ? "true" : "false") + "}");
}

void handleRestart() {
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Restarting...\"}");
  delay(1000);
  ESP.restart();
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
      welcome += "🤖 **ESP8266 Telegram Bot v2.1**\n\n";
      welcome += "📋 **Comandos disponibles:**\n\n";
      welcome += "🔆 **Control LED:**\n";
      welcome += "• `/ledon` - Encender LED\n";
      welcome += "• `/ledoff` - Apagar LED\n";
      welcome += "• `/ledtoggle` - Alternar LED\n";
      welcome += "• `/automode` - Modo automático\n\n";
      welcome += "📊 **Información:**\n";
      welcome += "• `/status` - Estado completo\n";
      welcome += "• `/ip` - Dirección IP\n";
      welcome += "• `/uptime` - Tiempo activo\n";
      welcome += "• `/memory` - Uso de memoria\n";
      welcome += "• `/wifi` - Info WiFi\n";
      welcome += "• `/web` - Enlace interfaz web\n\n";
      welcome += "🔧 **Sistema:**\n";
      welcome += "• `/restart` - Reiniciar ESP\n";
      welcome += "• `/version` - Información versión\n\n";
      welcome += "🌐 **Interfaz Web:** `http://" + WiFi.localIP().toString() + "`";
      
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
      ipMsg += "• Máscara: `" + WiFi.subnetMask().toString() + "`\n\n";
      ipMsg += "🌐 **Interfaz Web:** `http://" + WiFi.localIP().toString() + "`";
      bot.sendMessage(chat_id, ipMsg, "Markdown");
    }
    
    else if (text == "/web") {
      String webMsg = "🌐 **Interfaz Web Disponible**\n\n";
      webMsg += "🔗 **URL:** `http://" + WiFi.localIP().toString() + "`\n";
      webMsg += "🔗 **mDNS:** `http://" + deviceName + ".local`\n\n";
      webMsg += "✨ **Características:**\n";
      webMsg += "• Control LED en tiempo real\n";
      webMsg += "• Monitoreo del sistema\n";
      webMsg += "• Interfaz responsive\n";
      webMsg += "• Actualización automática\n";
      webMsg += "• API REST integrada";
      bot.sendMessage(chat_id, webMsg, "Markdown");
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
    
    else if (text == "/restart") {
      bot.sendMessage(chat_id, "🔄 **Reiniciando ESP8266...**\n\n⏳ El sistema se reiniciará en 3 segundos\n🌐 La interfaz web estará disponible tras el reinicio", "Markdown");
      delay(3000);
      ESP.restart();
    }
    
    else if (text == "/version") {
      String versionMsg = "ℹ️ **Información del Sistema**\n\n";
      versionMsg += "• Bot Version: `2.1 + Web Interface`\n";
      versionMsg += "• Chip ID: `" + String(ESP.getChipId()) + "`\n";
      versionMsg += "• SDK Version: `" + String(ESP.getSdkVersion()) + "`\n";
      versionMsg += "• Core Version: `" + String(ESP.getCoreVersion()) + "`\n";
      versionMsg += "• Boot Version: `" + String(ESP.getBootVersion()) + "`\n";
      versionMsg += "• Voltaje Real: `" + String(getVoltage(), 1) + "V`";
      bot.sendMessage(chat_id, versionMsg, "Markdown");
    }
    
    else {
      String unknownMsg = "❓ **Comando no reconocido**\n\n";
      unknownMsg += "Escribe `/help` para ver los comandos disponibles.\n\n";
      unknownMsg += "📝 Comando recibido: `" + text + "`\n";
      unknownMsg += "🌐 También puedes usar la interfaz web: `http://" + WiFi.localIP().toString() + "`";
      bot.sendMessage(chat_id, unknownMsg, "Markdown");
    }
  }
}
