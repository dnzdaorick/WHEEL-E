#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>

#define RXD2 16 
#define TXD2 17

WebSocketsClient webSocket;
bool wsConnected = false;

// Process WebSocket Client events
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("[WS] Disconnected from WHEEL-E Core!");
            wsConnected = false;
            break;
        case WStype_CONNECTED:
            Serial.println("[WS] Connected to WHEEL-E Core!");
            wsConnected = true;
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200); // USB Serial Monitor (to your Arch Linux machine)
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); // Link interface for Uno

    Serial.println("Starting WHEEL-E Remote Wi-Fi Bridge...");
    WiFi.begin("WHEEL-E", "Willy.willy123");

    // Block until Wi-Fi handshake completes
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected to WHEEL-E!");

    // Start WebSocket client pointing to the subnetwork gateway
    webSocket.begin("192.168.67.1", 81, "/");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(2000);
}

void loop() {
    webSocket.loop();

    // Check for incoming serial packets from the Uno
    if (Serial2.available() > 0) {
        char cmd = Serial2.read();
        
        Serial.print("[DEBUG] Received from Uno: '");
        Serial.print(cmd);
        Serial.println("'");

        if (wsConnected) {
            switch (cmd) {
                case 'F':
                    webSocket.sendTXT("FORWARD");
                    Serial.println("  -> Forwarded to WHEEL-E: FORWARD");
                    break;
                case 'B':
                    webSocket.sendTXT("BACKWARD");
                    Serial.println("  -> Forwarded to WHEEL-E: BACKWARD");
                    break;
                case 'L':
                    webSocket.sendTXT("LEFT");
                    Serial.println("  -> Forwarded to WHEEL-E: LEFT");
                    break;
                case 'R':
                    webSocket.sendTXT("RIGHT");
                    Serial.println("  -> Forwarded to WHEEL-E: RIGHT");
                    break;
                case 'S':
                    webSocket.sendTXT("STOP");
                    Serial.println("  -> Forwarded to WHEEL-E: STOP");
                    break;
                default:
                    Serial.print("  [WARN] Unknown raw byte received: ");
                    Serial.println((int)cmd);
                    break;
            }
        } else {
            Serial.println("  [WARN] WebSocket not active. Command dropped!");
        }
    }
}