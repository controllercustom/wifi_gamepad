/*
MIT License

Copyright (c) 2025 controllercustom@myyahoo.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define DEBUG_ON  0
#define DEBUG_WEBSOCK  0

#if DEBUG_ON
#define DBG_begin(...)    Serial.begin(__VA_ARGS__)
#define DBG_print(...)    Serial.print(__VA_ARGS__); Serial.flush()
#define DBG_println(...)  Serial.println(__VA_ARGS__); Serial.flush()
#define DBG_printf(...)   Serial.printf(__VA_ARGS__); Serial.flush()
#else
#define DBG_begin(...)
#define DBG_print(...)
#define DBG_println(...)
#define DBG_printf(...)
#endif

#include <WiFi.h>
#if defined(ESP32)
#include <ESPmDNS.h>
#elif defined(ARDUINO_ARCH_RP2040)
#include <LEAmDNS.h>
#endif
#include <WiFiClient.h>
#include <WebSocketsServer.h> // Install WebSockets by Markus Sattler from IDE Library manager
#include <WebServer.h>
#include <ArduinoJson.h>  // Install from IDE Library manager
#include "index_html.h"
#include "secrets.h"

#ifndef STASSID
#define STASSID "myssid"
#define STAPSK "mypassphrase"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  switch(type) {
    case WStype_DISCONNECTED:
      DBG_printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        DBG_printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:
      {
#if DEBUG_WEBSOCK
        DBG_printf("[%u] get Text: [%d] %s \r\n", num, length, payload);
#endif

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload, length);

        if (error) {
          DBG_print("json deserializeJson() failed: ");
          DBG_println(error.f_str());
          return;
        }
        const char *event = doc["event"];
        if ((strlen(event) >= 3) && (memcmp(event, "key", 3) == 0)) {
          const char* code = doc["code"];
          const char* key = doc["key"];
          int charCode = doc["charCode"];
          int keyCode = doc["keyCode"];
          bool altKey = doc["altKey"];
          bool ctrlKey = doc["ctrlKey"];
          bool metaKey = doc["metaKey"];
          bool shiftKey = doc["shiftKey"];
          DBG_printf("%lu:%s, code:%s, key:%s, charCode:%d, keyCode:%d "
              "Alt:%d, Ctrl:%d, Meta:%d, Shift:%d\r\n",
              millis(), event, code, key, charCode, keyCode,
              altKey, ctrlKey, metaKey, shiftKey);
        }
        else if ((strcmp(event, "gamepad") == 0)) {
          int index = doc["index"];

          DBG_printf("%lu:%s, index:%d\r\n", millis(), event, index);
          JsonArray axes = doc["axes"];
          if (axes) {
            unsigned int i = 0;
            for (JsonVariant axis : axes) {
              DBG_printf("%d:%d,", i, axis.as<const int>());
              i++;
            }
            DBG_println();
          }
          JsonArray buttons = doc["buttons"];
          if (buttons) {
            unsigned int i = 0;
            for (JsonVariant btn : buttons) {
              DBG_printf("%d:%d,", i, btn.as<const int>());
              i++;
            }
            DBG_println();
          }
        }
        else if ((strcmp(event, "gamepad_connected") == 0)) {
          int index = doc["index"];
          const char* id = doc["id"];
          DBG_printf("%s, index=%d, id=%s\r\n", event, index, id);
        }
        else if ((strcmp(event, "gamepad_disconnected") == 0)) {
          int index = doc["index"];
          DBG_printf("%s, index=%d\r\n", event, index);
        }
        else {
          DBG_print("Ignoring event: ");
          DBG_println(event);
        }
      }
      break;
    case WStype_BIN:
      DBG_printf("[%u] get binary length: %u\r\n", num, length);
      break;
    default:
      DBG_printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup_server() {
  server.on("/", handleRoot);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("websocket server started");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
#if defined(ESP32)
    ESP.restart();
#elif defined(ARDUINO_ARCH_RP2040)
    rp2040.restart();
#endif
  }

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("WiFiGamepad")) {
    Serial.println("MDNS responder started");
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
  }
  setup_server();
}

void loop() {
  webSocket.loop();
  server.handleClient();
#ifndef ESP32
  MDNS.update();
#endif
}
