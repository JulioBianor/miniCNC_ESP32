#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

const char* ssid = "TNT EQUIPADORA";
const char* password = "patostnt";

// Servidor HTTP e WebSocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Controle de execução do G-code
File gcodeFile;
bool executandoGcode = false;

// Posição atual dos eixos
float posAtual[4] = {0, 0, 0, 0};
float destino[4] = {0, 0, 0, 0};
bool emMovimento = false;
float feedRateAtual = 600.0; // mm/min
float feedRateAngularAtual = 1800.0; // graus/min
unsigned long ultimoTempo = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Inicializa LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("Erro ao montar LittleFS!");
    return;
  }

  // Conecta ao Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado! IP: ");
  Serial.println(WiFi.localIP());

  // Configura WebSocket
  ws.onEvent(onWebSocketEvent);
  //ws.enableHeartbeat(15000, 3000, 2); // ping a cada 15s, timeout em 3s, tolera 2 falhas
  server.addHandler(&ws);

  // Página principal
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Servir arquivos estáticos
  server.serveStatic("/", LittleFS, "/");

  // Upload de arquivos
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200);
  }, handleUpload);

  // Listar arquivos
  server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request) {
    String output;
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file) {
      output += String(file.name()) + "\n";
      file = root.openNextFile();
    }
    request->send(200, "text/plain", output);
  });

  // Executar G-code
  server.on("/run", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("file")) {
      String filename = request->getParam("file")->value();
      gcodeFile = LittleFS.open("/" + filename, "r");
      if (!gcodeFile || gcodeFile.isDirectory()) {
        request->send(404, "text/plain", "Arquivo não encontrado!");
      } else {
        executandoGcode = true;
        request->send(200, "text/plain", "Execução iniciada!");
      }
    } else {
      request->send(400, "text/plain", "Parâmetro 'file' ausente!");
    }
  });

  // Inicia servidor
  server.begin();
}

// Eventos WebSocket
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("Cliente conectado: ID %u\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("Cliente desconectado: ID %u\n", client->id());
  }
}

void loop() {
  unsigned long agora = millis();
  float deltaT = (agora - ultimoTempo) / 1000.0;
  ultimoTempo = agora;

  // G-code processamento
  if (executandoGcode && gcodeFile) {
    if (!emMovimento && gcodeFile.available()) {
      String linha = gcodeFile.readStringUntil('\n');
      linha.trim();
      if (linha.length() > 0) {
        interpretarLinhaGcode(linha);
      }
    } else if (!gcodeFile.available()) {
      gcodeFile.close();
      executandoGcode = false;
      Serial.println("Execução do G-code finalizada.");
    }
  }

  // Interpolação de movimentos
  if (emMovimento) {
    bool chegou = true;
    for (int i = 0; i < 4; i++) {
      float distancia = destino[i] - posAtual[i];
      float velocidadeUsar = (i == 3) ? (feedRateAngularAtual / 60.0) : (feedRateAtual / 60.0);
      if (abs(distancia) > velocidadeUsar * deltaT) {
        posAtual[i] += (distancia > 0 ? 1 : -1) * velocidadeUsar * deltaT;
        chegou = false;
      } else {
        posAtual[i] = destino[i];
      }
    }
    if (chegou) emMovimento = false;
  }

  // Atualizar posições para o navegador
  static unsigned long lastSend = 0;
  static float lastPos[4] = {0, 0, 0, 0};

  if (millis() - lastSend > 200) {  // Envio a cada 200ms (5Hz)
    bool mudou = false;
    for (int i = 0; i < 4; i++) {
      if (abs(posAtual[i] - lastPos[i]) > 0.01) {
        mudou = true;
        break;
      }
    }

    if (mudou && ws.count() > 0) {
      String json = "{\"x\":" + String(posAtual[0], 2) +
                    ",\"y\":" + String(posAtual[1], 2) +
                    ",\"z\":" + String(posAtual[2], 2) +
                    ",\"a\":" + String(posAtual[3], 2) + "}";
      ws.textAll(json);

      for (int i = 0; i < 4; i++) lastPos[i] = posAtual[i];
    }
    lastSend = millis();
  }
}

// Função de interpretação de G-code
void interpretarLinhaGcode(String linha) {
  linha.toUpperCase();

  if (linha.startsWith("G0") || linha.startsWith("G1")) {
    destino[0] = posAtual[0];
    destino[1] = posAtual[1];
    destino[2] = posAtual[2];
    destino[3] = posAtual[3];

    int idx;
    if ((idx = linha.indexOf('X')) >= 0) destino[0] = linha.substring(idx + 1).toFloat();
    if ((idx = linha.indexOf('Y')) >= 0) destino[1] = linha.substring(idx + 1).toFloat();
    if ((idx = linha.indexOf('Z')) >= 0) destino[2] = linha.substring(idx + 1).toFloat();
    if ((idx = linha.indexOf('A')) >= 0) destino[3] = linha.substring(idx + 1).toFloat();
    if ((idx = linha.indexOf('F')) >= 0) {
      float novoFeed = linha.substring(idx + 1).toFloat();
      if (novoFeed > 0) {
        feedRateAtual = novoFeed;
        feedRateAngularAtual = novoFeed;
      }
    }

    emMovimento = true;
    Serial.printf("Movendo para: X%.1f Y%.1f Z%.1f A%.1f F%.1f\n", destino[0], destino[1], destino[2], destino[3], feedRateAtual);
  }
  else if (linha.startsWith("M0") || linha.startsWith("M2") || linha.startsWith("M30")) {
    Serial.println("Comando de parada recebido.");
    gcodeFile.close();
    executandoGcode = false;
  }
}

// Upload de arquivos G-code
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  static File uploadFile;

  if (!index) {
    Serial.printf("Iniciando upload: %s\n", filename.c_str());
    if (LittleFS.exists("/" + filename)) LittleFS.remove("/" + filename);
    uploadFile = LittleFS.open("/" + filename, FILE_WRITE);
  }
  if (uploadFile) {
    uploadFile.write(data, len);
  }
  if (final) {
    Serial.printf("Upload finalizado: %s (%u bytes)\n", filename.c_str(), index + len);
    uploadFile.close();
  }
}
