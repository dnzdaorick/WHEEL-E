#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pin Mappings
#define MOTOR_IN4 0  // Left Motor Control (-)
#define MOTOR_IN3 1  // Left Motor Control (+)
#define MOTOR_IN2 2  // Right Motor Control (-)
#define MOTOR_IN1 3  // Right Motor Control (+)
#define OLED_SDA  8  // I2C SDA
#define OLED_SCL  9  // I2C SCL

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

IPAddress local_IP(192, 168, 67, 1);
IPAddress gateway(192, 168, 67, 1);
IPAddress subnet(255, 255, 255, 0);

const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// HTML Page with UI
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>WHEEL-E Dashboard</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; }
        body {
            background: radial-gradient(circle at center, #23120b, #0a0402);
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            color: #f5eae4;
            overflow: hidden;
        }
        .card {
            width: 90%;
            max-width: 380px;
            background: rgba(216, 122, 65, 0.03);
            border: 1px solid rgba(216, 122, 65, 0.12);
            border-radius: 28px;
            padding: 35px 25px;
            box-shadow: 0 25px 60px rgba(0, 0, 0, 0.65), inset 0 1px 1px rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(25px);
            -webkit-backdrop-filter: blur(25px);
            text-align: center;
        }
        h1 { 
            font-size: 2rem; 
            margin-bottom: 5px; 
            letter-spacing: 3px; 
            color: #e07a5f; 
            text-shadow: 0 0 12px rgba(224, 122, 95, 0.45); 
        }
        .status-bar { 
            font-size: 0.85rem; 
            color: #a8928a; 
            margin-bottom: 35px; 
            display: flex; 
            align-items: center; 
            justify-content: center; 
            gap: 8px; 
        }
        .dot { 
            width: 8px; 
            height: 8px; 
            background-color: #7c3a2d; 
            border-radius: 50%; 
            display: inline-block; 
            transition: background 0.3s; 
        }
        .dot.connected { 
            background-color: #e07a5f; 
            box-shadow: 0 0 8px #e07a5f; 
        }
        .control-grid {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 15px;
            margin-bottom: 25px;
        }
        .btn {
            background: rgba(216, 122, 65, 0.05);
            border: 1px solid rgba(216, 122, 65, 0.15);
            border-radius: 20px;
            aspect-ratio: 1;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 1.8rem;
            color: #f5eae4;
            cursor: pointer;
            transition: all 0.1s cubic-bezier(0.175, 0.885, 0.32, 1.275);
            touch-action: manipulation;
        }
        .btn:active {
            background: #e07a5f;
            color: #0c0604;
            border-color: #e07a5f;
            box-shadow: 0 0 25px rgba(224, 122, 95, 0.55);
            transform: scale(0.92);
        }
        .btn.empty { background: transparent; border: none; cursor: default; pointer-events: none; }
        .footer { font-size: 0.75rem; color: #6d564f; letter-spacing: 0.8px; }
    </style>
</head>
<body>
    <div class="card">
        <h1>WHEEL-E</h1>
        <div class="status-bar"><span class="dot" id="dot"></span> <span id="lbl">DISCONNECTED</span></div>
        <div class="control-grid">
            <div class="btn empty"></div>
            <div class="btn" id="up" data-cmd="FORWARD">▲</div>
            <div class="btn empty"></div>
            <div class="btn" id="left" data-cmd="LEFT">◀</div>
            <div class="btn empty"></div> <div class="btn" id="right" data-cmd="RIGHT">▶</div>
            <div class="btn empty"></div>
            <div class="btn" id="down" data-cmd="BACKWARD">▼</div>
            <div class="btn empty"></div>
        </div>
    </div>
    <script>
        let ws;
        const dot = document.getElementById('dot');
        const lbl = document.getElementById('lbl');
        
        function connect() {
            ws = new WebSocket('ws://' + window.location.hostname + ':81/');
            ws.onopen = () => {
                dot.classList.add('connected');
                lbl.innerText = 'CONNECTED';
            };
            ws.onclose = () => {
                dot.classList.remove('connected');
                lbl.innerText = 'DISCONNECTED';
                setTimeout(connect, 2000);
            };
        }
        
        function send(cmd) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(cmd);
            }
        }
        
        document.querySelectorAll('.btn:not(.empty)').forEach(btn => {
            const cmd = btn.getAttribute('data-cmd');
            btn.addEventListener('mousedown', () => send(cmd));
            btn.addEventListener('mouseup', () => { if(cmd !== 'STOP') send('STOP'); });
            btn.addEventListener('touchstart', (e) => { e.preventDefault(); send(cmd); });
            btn.addEventListener('touchend', (e) => { e.preventDefault(); if(cmd !== 'STOP') send('STOP'); });
        });
        
        connect();
    </script>
</body>
</html>
)rawliteral";

// WHEEL-E Animation & Face States
enum FaceState {
    FACE_BOOTING,
    FACE_CONNECTING,
    FACE_CONNECTED,
    FACE_DISCONNECTED,
    FACE_IDLE,
    FACE_FORWARD,
    FACE_BACKWARD,
    FACE_LEFT,
    FACE_RIGHT
};

FaceState currentFace = FACE_BOOTING;
String currentCmd = "STOP";

// Non-blocking Animation Timers
unsigned long lastBlinkTime = 0;
bool isBlinking = false;
unsigned long blinkDuration = 180;     // Sleep eye duration (ms)
unsigned long blinkInterval = 3500;     // Randomized blink window
unsigned long connectedAnimStart = 0;
const unsigned long connectedAnimDuration = 2000;

// Helper to draw text-only status strings during initial boot sequencing
void displayStatusLine(String text) {
    display.clearDisplay();
    display.setCursor(0, 16);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.println("=== WHEEL-E ===");
    display.setCursor(0, 36);
    display.println(text);
    display.display();
}

// Render dynamic, screen-filling facial frames
void updateFaceAnimation() {
    display.clearDisplay();
    unsigned long now = millis();

    // Handle organic blinking intervals
    if (currentFace == FACE_IDLE || currentFace == FACE_FORWARD) {
        if (!isBlinking && (now - lastBlinkTime >= blinkInterval)) {
            isBlinking = true;
            lastBlinkTime = now;
            blinkInterval = random(2500, 6000);
        }
        if (isBlinking && (now - lastBlinkTime >= blinkDuration)) {
            isBlinking = false;
        }
    }

    switch (currentFace) {
        case FACE_BOOTING: {
            display.setCursor(16, 24);
            display.setTextSize(2);
            display.setTextColor(SSD1306_WHITE);
            display.println("BOOTING...");
            break;
        }
        case FACE_CONNECTING: {
            display.drawRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
            display.drawRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);
            
            int pupilOffset = sin(now * 0.004) * 6;
            display.fillRoundRect(24 + pupilOffset, 12, 16, 20, 4, SSD1306_WHITE);
            display.fillRoundRect(88 + pupilOffset, 12, 16, 20, 4, SSD1306_WHITE);
            
            display.drawCircle(64, 44, 4, SSD1306_WHITE);
            break;
        }
        case FACE_CONNECTED: {
            display.fillRoundRect(16, 12, 32, 20, 6, SSD1306_WHITE);
            display.fillRoundRect(80, 12, 32, 20, 6, SSD1306_WHITE);
            
            display.drawLine(14, 22, 50, 22, SSD1306_BLACK);
            display.drawLine(78, 22, 114, 22, SSD1306_BLACK);

            // Rosy dimples
            display.fillCircle(10, 42, 4, SSD1306_WHITE);
            display.fillCircle(118, 42, 4, SSD1306_WHITE);

            int bounce = sin(now * 0.02) * 2;
            display.fillTriangle(52, 38, 76, 38, 64, 48 + bounce, SSD1306_WHITE);
            break;
        }
        case FACE_DISCONNECTED: {
            display.drawRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
            display.drawRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);
            
            display.drawLine(18, 8, 46, 36, SSD1306_WHITE);
            display.drawLine(46, 8, 18, 36, SSD1306_WHITE);
            display.drawLine(82, 8, 110, 36, SSD1306_WHITE);
            display.drawLine(110, 8, 82, 36, SSD1306_WHITE);
            
            display.drawLine(54, 45, 59, 42, SSD1306_WHITE);
            display.drawLine(59, 42, 64, 45, SSD1306_WHITE);
            display.drawLine(64, 45, 69, 42, SSD1306_WHITE);
            display.drawLine(69, 42, 74, 45, SSD1306_WHITE);
            break;
        }
        case FACE_IDLE: {
            // Rosy dimples
            display.fillCircle(10, 42, 4, SSD1306_WHITE);
            display.fillCircle(118, 42, 4, SSD1306_WHITE);

            display.drawPixel(64, 45, SSD1306_WHITE);
            display.drawLine(56, 42, 60, 45, SSD1306_WHITE);
            display.drawLine(60, 45, 64, 42, SSD1306_WHITE);
            display.drawLine(64, 42, 68, 45, SSD1306_WHITE);
            display.drawLine(68, 45, 72, 42, SSD1306_WHITE);

            if (isBlinking) {
                display.fillRoundRect(16, 18, 32, 8, 4, SSD1306_WHITE);
                display.fillRoundRect(80, 18, 32, 8, 4, SSD1306_WHITE);
            } else {
                display.fillRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
                display.fillRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);
                
                display.fillCircle(38, 14, 4, SSD1306_BLACK);
                display.fillCircle(24, 26, 2, SSD1306_BLACK);
                display.fillCircle(102, 14, 4, SSD1306_BLACK);
                display.fillCircle(88, 26, 2, SSD1306_BLACK);
            }
            break;
        }
        case FACE_FORWARD: {
            // Rosy dimples
            display.fillCircle(10, 42, 4, SSD1306_WHITE);
            display.fillCircle(118, 42, 4, SSD1306_WHITE);

            display.fillRoundRect(50, 36, 28, 16, 6, SSD1306_WHITE);

            if (isBlinking) {
                display.fillRoundRect(16, 18, 32, 8, 4, SSD1306_WHITE);
                display.fillRoundRect(80, 18, 32, 8, 4, SSD1306_WHITE);
            } else {
                display.fillRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
                display.fillRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);
                
                display.fillCircle(38, 14, 4, SSD1306_BLACK);
                display.fillCircle(24, 26, 2, SSD1306_BLACK);
                display.fillCircle(102, 14, 4, SSD1306_BLACK);
                display.fillCircle(88, 26, 2, SSD1306_BLACK);

                display.drawLine(18, 2, 34, 1, SSD1306_WHITE);
                display.drawLine(34, 1, 46, 3, SSD1306_WHITE);
                display.drawLine(82, 3, 94, 1, SSD1306_WHITE);
                display.drawLine(94, 1, 110, 2, SSD1306_WHITE);
            }
            break;
        }
        case FACE_BACKWARD: {
            // Rosy dimples
            display.fillCircle(10, 42, 4, SSD1306_WHITE);
            display.fillCircle(118, 42, 4, SSD1306_WHITE);

            display.drawRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
            display.drawRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);
            
            int shake = (now / 40) % 2 == 0 ? 1 : -1;
            display.fillRoundRect(24 + shake, 18, 16, 16, 4, SSD1306_WHITE);
            display.fillRoundRect(88 + shake, 18, 16, 16, 4, SSD1306_WHITE);

            display.drawLine(48, 4, 48, 8, SSD1306_WHITE);
            display.drawLine(112, 4, 112, 8, SSD1306_WHITE);

            display.drawRoundRect(52, 40, 24, 14, 6, SSD1306_WHITE);
            break;
        }
        case FACE_LEFT: {
            display.drawRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
            display.drawRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);
            
            display.fillRoundRect(19, 10, 18, 24, 6, SSD1306_WHITE);
            display.fillRoundRect(83, 10, 18, 24, 6, SSD1306_WHITE);
            
            display.fillCircle(10, 42, 4, SSD1306_WHITE);
            display.fillCircle(118, 42, 4, SSD1306_WHITE);

            display.drawLine(52, 43, 56, 46, SSD1306_WHITE);
            display.drawLine(56, 46, 60, 43, SSD1306_WHITE);
            display.drawLine(60, 43, 64, 46, SSD1306_WHITE);
            display.drawLine(64, 46, 68, 43, SSD1306_WHITE);
            break;
        }
        case FACE_RIGHT: {
            display.drawRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
            display.drawRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);
            
            display.fillRoundRect(27, 10, 18, 24, 6, SSD1306_WHITE);
            display.fillRoundRect(91, 10, 18, 24, 6, SSD1306_WHITE);
            
            display.fillCircle(10, 42, 4, SSD1306_WHITE);
            display.fillCircle(118, 42, 4, SSD1306_WHITE);

            display.drawLine(60, 43, 64, 46, SSD1306_WHITE);
            display.drawLine(64, 46, 68, 43, SSD1306_WHITE);
            display.drawLine(68, 43, 72, 46, SSD1306_WHITE);
            display.drawLine(72, 46, 76, 43, SSD1306_WHITE);
            break;
        }
    }
    display.display();
}

// Drive outputs to MX1508
void processMovement(String command) {
    currentCmd = command;
    if (command == "FORWARD") {
        // Both wheels spin forward: Right Forward, Left Forward
        digitalWrite(MOTOR_IN1, HIGH); digitalWrite(MOTOR_IN2, LOW);
        digitalWrite(MOTOR_IN3, LOW);  digitalWrite(MOTOR_IN4, HIGH);
    } else if (command == "BACKWARD") {
        // Both wheels spin backward: Right Backward, Left Backward 
        digitalWrite(MOTOR_IN1, LOW);  digitalWrite(MOTOR_IN2, HIGH);
        digitalWrite(MOTOR_IN3, HIGH); digitalWrite(MOTOR_IN4, LOW);
    } else if (command == "LEFT") {
        // Pivot Left: Right Forward, Left Backward 
        digitalWrite(MOTOR_IN1, HIGH); digitalWrite(MOTOR_IN2, LOW);
        digitalWrite(MOTOR_IN3, HIGH); digitalWrite(MOTOR_IN4, LOW);
    } else if (command == "RIGHT") {
        // Pivot Right: Right Backward, Left Forward 
        digitalWrite(MOTOR_IN1, LOW);  digitalWrite(MOTOR_IN2, HIGH);
        digitalWrite(MOTOR_IN3, LOW);  digitalWrite(MOTOR_IN4, HIGH);
    } else { // STOP
        digitalWrite(MOTOR_IN1, LOW);  digitalWrite(MOTOR_IN2, LOW);
        digitalWrite(MOTOR_IN3, LOW);  digitalWrite(MOTOR_IN4, LOW);
        currentCmd = "STOP";
    }
}

// WebSocket client packet handler
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
        String cmd = String((char*)payload);
        processMovement(cmd);
    }
}

void setup() {
    Serial.begin(115200);
    
    // Configure MX1508 Pin directions
    pinMode(MOTOR_IN1, OUTPUT);
    pinMode(MOTOR_IN2, OUTPUT);
    pinMode(MOTOR_IN3, OUTPUT);
    pinMode(MOTOR_IN4, OUTPUT);
    processMovement("STOP");

    // Start I2C bus
    Wire.begin(OLED_SDA, OLED_SCL);
    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        display.clearDisplay();
        displayStatusLine("BOOTING AP...");
    }

    // Assign custom subnetwork settings
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP("WHEEL-E", "Willy.willy123");
    
    // Launch Captive DNS Portal routing
    dnsServer.start(DNS_PORT, "*", local_IP);

    // Serve Dashboard
    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html", INDEX_HTML);
    });

    // Captive Portal browser hooks
    server.onNotFound([]() {
        server.sendHeader("Location", String("http://192.168.67.1/"), true);
        server.send(302, "text/plain", "");
    });
    
    server.begin();
    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);
    
    currentFace = FACE_CONNECTING;
}

void loop() {
    dnsServer.processNextRequest();
    server.handleClient();
    webSocket.loop();

    static int lastActiveClients = 0;
    int activeClients = webSocket.connectedClients();

    if (activeClients > lastActiveClients) {
        currentFace = FACE_CONNECTED;
        connectedAnimStart = millis();
    } else if (activeClients == 0) {
        if (currentFace != FACE_BOOTING && currentFace != FACE_CONNECTING && currentFace != FACE_DISCONNECTED) {
            currentFace = FACE_DISCONNECTED;
        }
    } else {
        if (currentFace == FACE_CONNECTED) {
            if (millis() - connectedAnimStart >= connectedAnimDuration) {
                currentFace = FACE_IDLE;
            }
        } else {
            if (currentCmd == "FORWARD") currentFace = FACE_FORWARD;
            else if (currentCmd == "BACKWARD") currentFace = FACE_BACKWARD;
            else if (currentCmd == "LEFT") currentFace = FACE_LEFT;
            else if (currentCmd == "RIGHT") currentFace = FACE_RIGHT;
            else currentFace = FACE_IDLE;
        }
    }
    
    lastActiveClients = activeClients;
    updateFaceAnimation();
}