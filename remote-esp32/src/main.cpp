#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>

// ── Uncomment to enable verbose debug output over USB serial ──────────────────
#define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
  #define DBG_PRINT(x)   Serial.print(x)
  #define DBG_PRINTLN(x) Serial.println(x)
#else
  #define DBG_PRINT(x)
  #define DBG_PRINTLN(x)
#endif

#define RXD2 16
#define TXD2 17

WebSocketsClient webSocket;
bool wsConnected = false;

// ── WebSocket Event Handler ───────────────────────────────────────────────────
void webSocketEvent(WStype_t type, uint8_t* payload, size_t /*length*/) {
    switch (type) {
        case WStype_DISCONNECTED:
            DBG_PRINTLN("[WS] Disconnected from WHEEL-E Core!");
            wsConnected = false;
            break;
        case WStype_CONNECTED:
            DBG_PRINTLN("[WS] Connected to WHEEL-E Core!");
            wsConnected = true;
            break;
        default:
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

    DBG_PRINTLN("Starting WHEEL-E Remote Wi-Fi Bridge...");

    // Reduce TX power — robot is at arm's length; 8.5 dBm is plenty.
    // Saves heat, extends battery life on the remote.
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.begin("WHEEL-E", "Willy.willy123");

    // Non-blocking WiFi connect with 10 s timeout and auto-retry
    DBG_PRINT("Connecting");
    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - wifiStart > 10000) {
            DBG_PRINTLN("\nWiFi timeout — retrying...");
            WiFi.disconnect(true);
            delay(500);
            WiFi.begin("WHEEL-E", "Willy.willy123");
            wifiStart = millis();
        }
        delay(250);
        DBG_PRINT(".");
    }
    DBG_PRINTLN("\nWiFi Connected to WHEEL-E!");

    // Point WebSocket client at the chassis subnetwork gateway
    webSocket.begin("192.168.67.1", 81, "/");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(2000);
    // Heartbeat keeps the connection alive through idle periods;
    // detects silent drops within ~15 s instead of waiting for timeout.
    webSocket.setHeartbeatInterval(15);
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    webSocket.loop();

    // Forward serial packets from the Uno to the WHEEL-E chassis
    if (Serial2.available() > 0) {
        char cmd = Serial2.read();

        DBG_PRINT("[DEBUG] Received from Uno: '");
        DBG_PRINT(cmd);
        DBG_PRINTLN("'");

        if (wsConnected) {
            switch (cmd) {
                case 'F': webSocket.sendTXT("FORWARD");  DBG_PRINTLN("  -> FORWARD");  break;
                case 'B': webSocket.sendTXT("BACKWARD"); DBG_PRINTLN("  -> BACKWARD"); break;
                case 'L': webSocket.sendTXT("LEFT");     DBG_PRINTLN("  -> LEFT");     break;
                case 'R': webSocket.sendTXT("RIGHT");    DBG_PRINTLN("  -> RIGHT");    break;
                case 'S': webSocket.sendTXT("STOP");     DBG_PRINTLN("  -> STOP");     break;
                default:
                    DBG_PRINT("  [WARN] Unknown byte: ");
                    DBG_PRINTLN((int)cmd);
                    break;
            }
        } else {
            DBG_PRINTLN("  [WARN] WebSocket not active. Command dropped!");
        }
    }
}