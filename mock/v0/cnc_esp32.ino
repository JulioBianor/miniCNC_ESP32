#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

const char* ssid = "CNC_WIFI";
const char* password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

int posX = 0, posY = 0, posZ = 0;

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    String msg = String((char*)data);

    if (msg == "home") posX = posY = posZ = 0;
    else if (msg == "x+") posX += 1;
    else if (msg == "x-") posX -= 1;
    else if (msg == "y+") posY += 1;
    else if (msg == "y-") posY -= 1;
    else if (msg == "z+") posZ += 1;
    else if (msg == "z-") posZ -= 1;

    String json = "{\"x\":" + String(posX) + ",\"y\":" + String(posY) + ",\"z\":" + String(posZ) + "}";
    ws.textAll(json);
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) handleWebSocketMessage(arg, data, len);
}

void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin()) {
    Serial.println("Erro ao montar LittleFS");
    return;
  }

  WiFi.softAP(ssid, password);
  Serial.println("Wi-Fi em modo AP iniciado");

  ws.onEvent(onEvent);
  server.addHandler(&ws);

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.begin();
  Serial.println("Servidor iniciado!");
}

void loop() {
  // Nada aqui por enquanto
}
