#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pin Mappings
#define MOTOR_IN4 0  
#define MOTOR_IN3 1  
#define MOTOR_IN2 2  
#define MOTOR_IN1 3  
#define OLED_SDA  8  
#define OLED_SCL  9  
#define BUZZER_PIN 10 
#define BATTERY_PIN 4   // ADC1_CH4 for battery voltage divider
#define CHARGING_PIN 5  // Digital input for charging status (LOW = Charging)

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Custom IP configurations on the 192.168.67.X subnetwork
IPAddress local_IP(192, 168, 67, 1);
IPAddress gateway(192, 168, 67, 1);
IPAddress subnet(255, 255, 255, 0);

const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Copper Glassmorphism Dashboard HTML
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
            <div class="btn empty"></div>
            <div class="btn" id="right" data-cmd="RIGHT">▶</div>
            <div class="btn empty"></div>
            <div class="btn" id="down" data-cmd="BACKWARD">▼</div>
            <div class="btn empty"></div>
        </div>
        <div class="footer">COPPER CIRCUIT SCULPTURE</div>
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

// Robot Face States
enum FaceState {
    FACE_BOOTING,
    FACE_CONNECTING,
    FACE_CONNECTED,
    FACE_DISCONNECTED,
    FACE_IDLE,
    FACE_FORWARD,
    FACE_BACKWARD,
    FACE_LEFT,
    FACE_RIGHT,
    FACE_CHARGING,
    FACE_FULL_CHARGE,
    FACE_LOW_BATTERY
};

FaceState currentFace = FACE_BOOTING;
String currentCmd = "STOP";

// --- DYNAMIC COMPANION MOOD ROTATION STATES ---
enum IdleActivity {
    ACT_RESTING,   
    ACT_SINGING,   
    ACT_GIGGLING,  
    ACT_LOOKING,   
    ACT_DIZZY,     
    ACT_POUTING,   
    ACT_SIGHING,   
    ACT_SNEEZING,  
    ACT_YAWNING,   
    ACT_NAPPING    
};

IdleActivity currentIdleAct = ACT_RESTING;
IdleActivity nextExpressiveMood = ACT_SINGING; 

// --- COHERENT PAUSABLE DELTA TIMERS ---
unsigned long lastLoopTime = 0;              
unsigned long idleActElapsedTime = 0;        
unsigned long idleActDuration = 5000;        
unsigned long sleepElapsedTime = 0;          
const unsigned long sleepThreshold = 120000; // 2 minutes

// Non-blocking OLED Animation Timers
unsigned long lastBlinkTime = 0;
bool isBlinking = false;
unsigned long blinkDuration = 180;     
unsigned long blinkInterval = 3500;     
unsigned long connectedAnimStart = 0;
const unsigned long connectedAnimDuration = 2000;

// Non-blocking Sound Sequencer Structure
struct Note {
    int frequency;
    unsigned long duration;
};

#define MAX_NOTES 16
Note currentMelody[MAX_NOTES];
int melodyLength = 0;
int currentNoteIndex = -1;
unsigned long noteStartTime = 0;
bool isPlayingSound = false;

// Battery System Variables
float batteryVoltage = 4.2;
bool isBatteryLow = false;
bool isCharging = false;
bool isFullCharge = false;
unsigned long lastBatteryCheck = 0;

// Rhythmic Buzzer Timers
unsigned long lastBackupBeep = 0;
bool backupBeepState = false;

// Helper to start playing a melody
void playMelody(int length) {
    melodyLength = length;
    currentNoteIndex = 0;
    noteStartTime = millis();
    isPlayingSound = true;
    if (currentMelody[0].frequency > 0) {
        tone(BUZZER_PIN, currentMelody[0].frequency);
    } else {
        noTone(BUZZER_PIN);
    }
}

// Sound Library
void triggerBootSound() {
    currentMelody[0] = {523, 100};  // C5
    currentMelody[1] = {659, 100};  // E5
    currentMelody[2] = {784, 100};  // G5
    currentMelody[3] = {1047, 180}; // C6
    playMelody(4);
}

void triggerConnectedSound() {
    currentMelody[0] = {880, 80};   // A5
    currentMelody[1] = {1047, 80};  // C6
    currentMelody[2] = {1319, 80};  // E6
    currentMelody[3] = {1568, 150}; // G6
    playMelody(4);
}

void triggerDisconnectedSound() {
    currentMelody[0] = {784, 150};  
    currentMelody[1] = {622, 150};  
    currentMelody[2] = {494, 250};  
    playMelody(3);
}

void triggerBlinkSound() {
    currentMelody[0] = {2000, 15};
    playMelody(1);
}

void triggerYawnSound() {
    currentMelody[0] = {700, 70};
    currentMelody[1] = {600, 70};
    currentMelody[2] = {500, 70};
    currentMelody[3] = {400, 70};
    currentMelody[4] = {300, 150};
    currentMelody[5] = {350, 150};
    playMelody(6);
}

void triggerSnoozeSound() {
    currentMelody[0] = {160, 150};
    currentMelody[1] = {190, 250};
    currentMelody[2] = {160, 150};
    playMelody(3);
}

void triggerSingingSound() {
    currentMelody[0] = {523, 80};   
    currentMelody[1] = {659, 80};   
    currentMelody[2] = {784, 80};   
    currentMelody[3] = {1047, 80};  
    currentMelody[4] = {1319, 80};  
    currentMelody[5] = {1568, 150}; 
    playMelody(6);
}

void triggerGiggleSound() {
    currentMelody[0] = {1300, 60};
    currentMelody[1] = {1600, 60};
    currentMelody[2] = {1400, 60};
    currentMelody[3] = {1800, 60};
    currentMelody[4] = {1500, 60};
    currentMelody[5] = {1900, 120};
    playMelody(6);
}

void triggerLookingSound() {
    currentMelody[0] = {2100, 15};
    currentMelody[1] = {0, 100};   
    currentMelody[2] = {2300, 15};
    playMelody(3);
}

void triggerDizzySound() {
    currentMelody[0] = {900, 80};
    currentMelody[1] = {700, 80};
    currentMelody[2] = {500, 80};
    currentMelody[3] = {600, 80};
    currentMelody[4] = {800, 80};
    currentMelody[5] = {1000, 150};
    playMelody(6);
}

void triggerPoutingSound() {
    currentMelody[0] = {220, 150};
    currentMelody[1] = {180, 150};
    currentMelody[2] = {200, 250};
    playMelody(3);
}

void triggerSighSound() {
    currentMelody[0] = {440, 100};
    currentMelody[1] = {380, 100};
    currentMelody[2] = {311, 100};
    currentMelody[3] = {220, 120};
    currentMelody[4] = {147, 250};
    playMelody(5);
}

void triggerSneezeSound() {
    currentMelody[0] = {400, 150};
    currentMelody[1] = {500, 150};
    currentMelody[2] = {600, 150};
    currentMelody[3] = {700, 200};
    currentMelody[4] = {0, 150};   
    currentMelody[5] = {1800, 80};  
    currentMelody[6] = {300, 200};
    playMelody(7);
}

void triggerActionBlip() {
    currentMelody[0] = {1100, 50}; 
    playMelody(1);
}

void triggerChargingPluggedSound() {
    currentMelody[0] = {523, 100};  
    currentMelody[1] = {784, 100};  
    currentMelody[2] = {1047, 150}; 
    playMelody(3);
}

void triggerLowBatterySound() {
    currentMelody[0] = {220, 200};  
    currentMelody[1] = {0, 150};
    currentMelody[2] = {220, 200};
    playMelody(3);
}

void triggerFullChargeSound() {
    currentMelody[0] = {523, 80};   
    currentMelody[1] = {659, 80};   
    currentMelody[2] = {784, 80};   
    currentMelody[3] = {988, 80};   
    currentMelody[4] = {1047, 250}; 
    playMelody(5);
}

// Background sound engine process
void updateSoundEngine() {
    if (currentCmd == "BACKWARD") {
        isPlayingSound = false;
        currentNoteIndex = -1;
        
        unsigned long now = millis();
        if (now - lastBackupBeep >= 250) { 
            lastBackupBeep = now;
            backupBeepState = !backupBeepState;
            if (backupBeepState) {
                tone(BUZZER_PIN, 1000); 
            } else {
                noTone(BUZZER_PIN);
            }
        }
        return;
    } else {
        if (backupBeepState) {
            noTone(BUZZER_PIN);
            backupBeepState = false;
        }
    }

    if (!isPlayingSound) return;

    unsigned long now = millis();
    if (now - noteStartTime >= currentMelody[currentNoteIndex].duration) {
        currentNoteIndex++;
        if (currentNoteIndex < melodyLength) {
            noteStartTime = now;
            if (currentMelody[currentNoteIndex].frequency > 0) {
                tone(BUZZER_PIN, currentMelody[currentNoteIndex].frequency);
            } else {
                noTone(BUZZER_PIN);
            }
        } else {
            noTone(BUZZER_PIN);
            isPlayingSound = false;
            currentNoteIndex = -1;
        }
    }
}

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

    // Trigger synchronized blink click sounds during RESTING, FORWARD, LEFT, or RIGHT states
    if (currentFace == FACE_FORWARD || currentFace == FACE_LEFT || currentFace == FACE_RIGHT || (currentFace == FACE_IDLE && currentIdleAct == ACT_RESTING)) {
        if (!isBlinking && (now - lastBlinkTime >= blinkInterval)) {
            isBlinking = true;
            lastBlinkTime = now;
            blinkInterval = random(2500, 6000);
            triggerBlinkSound(); 
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
        case FACE_FORWARD: {
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
            display.fillCircle(10, 42, 4, SSD1306_WHITE);
            display.fillCircle(118, 42, 4, SSD1306_WHITE);

            display.drawLine(52, 43, 56, 46, SSD1306_WHITE);
            display.drawLine(56, 46, 60, 43, SSD1306_WHITE);
            display.drawLine(60, 43, 64, 46, SSD1306_WHITE);
            display.drawLine(64, 46, 68, 43, SSD1306_WHITE);

            if (isBlinking) {
                display.fillRoundRect(16, 18, 32, 8, 4, SSD1306_WHITE);
                display.fillRoundRect(80, 18, 32, 8, 4, SSD1306_WHITE);
            } else {
                display.drawRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
                display.drawRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);
                
                display.fillRoundRect(19, 10, 18, 24, 6, SSD1306_WHITE);
                display.fillRoundRect(83, 10, 18, 24, 6, SSD1306_WHITE);
            }
            break;
        }
        case FACE_RIGHT: {
            display.fillCircle(10, 42, 4, SSD1306_WHITE);
            display.fillCircle(118, 42, 4, SSD1306_WHITE);

            display.drawLine(60, 43, 64, 46, SSD1306_WHITE);
            display.drawLine(64, 46, 68, 43, SSD1306_WHITE);
            display.drawLine(68, 43, 72, 46, SSD1306_WHITE);
            display.drawLine(72, 46, 76, 43, SSD1306_WHITE);

            if (isBlinking) {
                display.fillRoundRect(16, 18, 32, 8, 4, SSD1306_WHITE);
                display.fillRoundRect(80, 18, 32, 8, 4, SSD1306_WHITE);
            } else {
                display.drawRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
                display.drawRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);
                
                display.fillRoundRect(27, 10, 18, 24, 6, SSD1306_WHITE);
                display.fillRoundRect(91, 10, 18, 24, 6, SSD1306_WHITE);
            }
            break;
        }
        case FACE_CHARGING: {
            display.fillCircle(10, 42, 4, SSD1306_WHITE);
            display.fillCircle(118, 42, 4, SSD1306_WHITE);

            display.drawPixel(64, 43, SSD1306_WHITE);
            display.drawLine(58, 40, 60, 42, SSD1306_WHITE);
            display.drawLine(60, 42, 64, 43, SSD1306_WHITE);
            display.drawLine(64, 43, 68, 42, SSD1306_WHITE);
            display.drawLine(68, 42, 70, 40, SSD1306_WHITE);

            int breath = sin(now * 0.003) * 2 + 4;
            display.fillRoundRect(16, 18, 32, breath, 2, SSD1306_WHITE);
            display.fillRoundRect(80, 18, 32, breath, 2, SSD1306_WHITE);

            display.drawRoundRect(48, 51, 32, 11, 2, SSD1306_WHITE);
            display.fillRect(80, 54, 2, 5, SSD1306_WHITE); 
            
            int fillWidth = ((now / 200) % 6) * 5; 
            display.fillRect(50, 53, fillWidth, 7, SSD1306_WHITE);
            break;
        }
        case FACE_FULL_CHARGE: {
            display.fillCircle(10, 42, 4, SSD1306_WHITE);
            display.fillCircle(118, 42, 4, SSD1306_WHITE);
            display.fillRoundRect(50, 36, 28, 15, 5, SSD1306_WHITE);

            display.fillRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
            display.fillRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);
            display.fillCircle(38, 14, 4, SSD1306_BLACK);
            display.fillCircle(24, 26, 2, SSD1306_BLACK);
            display.fillCircle(102, 14, 4, SSD1306_BLACK);
            display.fillCircle(88, 26, 2, SSD1306_BLACK);

            display.drawRoundRect(48, 51, 32, 11, 2, SSD1306_WHITE);
            display.fillRect(80, 54, 2, 5, SSD1306_WHITE);
            display.fillRect(50, 53, 28, 7, SSD1306_WHITE);
            break;
        }
        case FACE_LOW_BATTERY: {
            int jitter = (now / 30) % 2 == 0 ? 1 : -1;
            display.drawRoundRect(16 + jitter, 6, 32, 32, 8, SSD1306_WHITE);
            display.drawRoundRect(80 + jitter, 6, 32, 32, 8, SSD1306_WHITE);

            display.drawLine(24 + jitter, 14, 40 + jitter, 30, SSD1306_WHITE);
            display.drawLine(40 + jitter, 14, 24 + jitter, 30, SSD1306_WHITE);
            display.drawLine(88 + jitter, 14, 104 + jitter, 30, SSD1306_WHITE);
            display.drawLine(104 + jitter, 14, 88 + jitter, 30, SSD1306_WHITE);

            display.drawLine(58, 45, 70, 45, SSD1306_WHITE);
            display.drawLine(58, 45, 56, 47, SSD1306_WHITE);
            display.drawLine(70, 45, 72, 47, SSD1306_WHITE);

            if ((now / 350) % 2 == 0) {
                display.drawRoundRect(48, 51, 32, 11, 2, SSD1306_WHITE);
                display.fillRect(80, 54, 2, 5, SSD1306_WHITE);
                display.fillRect(52, 53, 6, 7, SSD1306_WHITE); 
            }
            break;
        }
        case FACE_IDLE: {
            display.fillCircle(10, 42, 4, SSD1306_WHITE);
            display.fillCircle(118, 42, 4, SSD1306_WHITE);

            switch (currentIdleAct) {
                case ACT_RESTING: {
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
                case ACT_SINGING: {
                    display.drawCircle(32, 28, 12, SSD1306_WHITE);
                    display.fillRect(16, 28, 32, 14, SSD1306_BLACK);
                    display.drawCircle(96, 28, 12, SSD1306_WHITE);
                    display.fillRect(80, 28, 32, 14, SSD1306_BLACK);

                    int singingMouth = sin(now * 0.015) * 3 + 7;
                    display.fillCircle(64, 44, singingMouth, SSD1306_WHITE);

                    int noteOsc = sin(now * 0.01) * 4;
                    int nX = 54;
                    int nY = 16 + noteOsc;
                    display.fillCircle(nX, nY, 3, SSD1306_WHITE);
                    display.drawLine(nX + 2, nY, nX + 2, nY - 8, SSD1306_WHITE);
                    display.drawLine(nX + 2, nY - 8, nX + 6, nY - 6, SSD1306_WHITE);
                    break;
                }
                case ACT_GIGGLING: {
                    int giggleShake = (now / 75) % 2 == 0 ? 2 : -2;
                    display.drawCircle(32, 26 + giggleShake, 10, SSD1306_WHITE);
                    display.fillRect(16, 26 + giggleShake, 32, 12, SSD1306_BLACK);
                    display.drawCircle(96, 26 + giggleShake, 10, SSD1306_WHITE);
                    display.fillRect(80, 26 + giggleShake, 32, 12, SSD1306_BLACK);

                    if ((now / 150) % 2 == 0) {
                        display.fillCircle(64, 44, 6, SSD1306_WHITE); 
                    } else {
                        display.drawLine(58, 44, 70, 44, SSD1306_WHITE); 
                    }
                    break;
                }
                case ACT_LOOKING: {
                    display.drawRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
                    display.drawRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);
                    
                    int lookDir = idleActElapsedTime < 2000 ? -1 : 1; 
                    int pupilX1 = 32 + lookDir * 6;
                    int pupilX2 = 96 + lookDir * 6;
                    display.fillRoundRect(pupilX1 - 8, 12, 16, 20, 4, SSD1306_WHITE);
                    display.fillRoundRect(pupilX2 - 8, 12, 16, 20, 4, SSD1306_WHITE);

                    display.fillCircle(pupilX1 + 4, 16, 2, SSD1306_BLACK);
                    display.fillCircle(pupilX2 + 4, 16, 2, SSD1306_BLACK);

                    int shift = lookDir * 2;
                    display.drawPixel(64 + shift, 45, SSD1306_WHITE);
                    display.drawLine(56 + shift, 42, 60 + shift, 45, SSD1306_WHITE);
                    display.drawLine(60 + shift, 45, 64 + shift, 42, SSD1306_WHITE);
                    display.drawLine(64 + shift, 42, 68 + shift, 45, SSD1306_WHITE);
                    display.drawLine(68 + shift, 45, 72 + shift, 42, SSD1306_WHITE);
                    break;
                }
                case ACT_DIZZY: {
                    display.drawRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
                    display.drawRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);

                    float rotAngle = now * 0.01;
                    int dx1 = cos(rotAngle) * 8;
                    int dy1 = sin(rotAngle) * 8;
                    int dx2 = cos(rotAngle + 1.57) * 8;
                    int dy2 = sin(rotAngle + 1.57) * 8;

                    display.drawLine(32 - dx1, 22 - dy1, 32 + dx1, 22 + dy1, SSD1306_WHITE);
                    display.drawLine(32 - dx2, 22 - dy2, 32 + dx2, 22 + dy2, SSD1306_WHITE);
                    display.drawLine(96 - dx1, 22 - dy1, 96 + dx1, 22 + dy1, SSD1306_WHITE);
                    display.drawLine(96 - dx2, 22 - dy2, 96 + dx2, 22 + dy2, SSD1306_WHITE);

                    int starX = 64 + cos(now * 0.005) * 20;
                    int starY = 12 + sin(now * 0.005) * 4;
                    display.drawLine(starX - 3, starY, starX + 3, starY, SSD1306_WHITE);
                    display.drawLine(starX, starY - 3, starX, starY + 3, SSD1306_WHITE);

                    display.drawLine(54, 43, 58, 46, SSD1306_WHITE);
                    display.drawLine(58, 46, 62, 43, SSD1306_WHITE);
                    display.drawLine(62, 43, 66, 46, SSD1306_WHITE);
                    display.drawLine(66, 46, 70, 43, SSD1306_WHITE);
                    break;
                }
                case ACT_POUTING: {
                    display.fillRoundRect(16, 10, 32, 28, 8, SSD1306_WHITE);
                    display.fillRoundRect(80, 10, 32, 28, 8, SSD1306_WHITE);
                    
                    display.fillCircle(32, 24, 4, SSD1306_BLACK);
                    display.fillCircle(96, 24, 4, SSD1306_BLACK);

                    display.drawLine(14, 4, 46, 12, SSD1306_WHITE);
                    display.drawLine(14, 5, 46, 13, SSD1306_WHITE);
                    display.drawLine(114, 4, 82, 12, SSD1306_WHITE);
                    display.drawLine(114, 5, 82, 13, SSD1306_WHITE);

                    display.drawLine(56, 46, 72, 46, SSD1306_WHITE);
                    display.drawLine(56, 46, 54, 49, SSD1306_WHITE);
                    display.drawLine(72, 46, 74, 49, SSD1306_WHITE);

                    int crossX = 112;
                    int crossY = 10;
                    display.drawLine(crossX - 4, crossY, crossX + 4, crossY, SSD1306_WHITE);
                    display.drawLine(crossX, crossY - 4, crossX, crossY + 4, SSD1306_WHITE);
                    display.drawPixel(crossX - 2, crossY - 2, SSD1306_WHITE);
                    display.drawPixel(crossX + 2, crossY + 2, SSD1306_WHITE);
                    break;
                }
                case ACT_SIGHING: {
                    display.fillRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
                    display.fillRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);
                    display.fillRect(16, 6, 32, 12, SSD1306_BLACK);
                    display.fillRect(80, 6, 32, 12, SSD1306_BLACK);
                    display.drawRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
                    display.drawRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);

                    display.drawLine(58, 45, 70, 45, SSD1306_WHITE);
                    display.drawLine(58, 45, 56, 47, SSD1306_WHITE);
                    display.drawLine(70, 45, 72, 47, SSD1306_WHITE);
                    break;
                }
                case ACT_SNEEZING: {
                    unsigned long elapsed = idleActElapsedTime; 
                    if (elapsed < 2500) {
                        int squeeze = (elapsed / 250); 
                        display.fillRoundRect(16, 6 + squeeze / 2, 32, 32 - squeeze, 8, SSD1306_WHITE);
                        display.fillRoundRect(80, 6 + squeeze / 2, 32, 32 - squeeze, 8, SSD1306_WHITE);
                        
                        int shake = (now / 30) % 2 == 0 ? 1 : -1;
                        display.fillCircle(32 + shake, 22, 3, SSD1306_BLACK);
                        display.fillCircle(96 + shake, 22, 3, SSD1306_BLACK);
                        
                        display.drawCircle(64, 44, 3 + squeeze / 3, SSD1306_WHITE);
                    } else {
                        display.drawLine(14, 22, 30, 14, SSD1306_WHITE);
                        display.drawLine(30, 14, 46, 22, SSD1306_WHITE);
                        display.drawLine(14, 23, 30, 15, SSD1306_WHITE);
                        display.drawLine(30, 15, 46, 23, SSD1306_WHITE);
                        
                        display.drawLine(82, 22, 98, 14, SSD1306_WHITE);
                        display.drawLine(98, 14, 114, 22, SSD1306_WHITE);
                        display.drawLine(82, 23, 98, 15, SSD1306_WHITE);
                        display.drawLine(98, 15, 114, 23, SSD1306_WHITE);
                        
                        display.fillRoundRect(52, 36, 24, 20, 6, SSD1306_WHITE);
                        
                        display.fillCircle(40, 48, 4, SSD1306_WHITE);
                        display.fillCircle(88, 48, 4, SSD1306_WHITE);
                        display.fillCircle(64, 56, 3, SSD1306_WHITE);
                    }
                    break;
                }
                case ACT_YAWNING: {
                    display.drawCircle(32, 26, 10, SSD1306_WHITE);
                    display.fillRect(16, 26, 32, 12, SSD1306_BLACK); 
                    display.drawCircle(96, 26, 10, SSD1306_WHITE);
                    display.fillRect(80, 26, 32, 12, SSD1306_BLACK); 

                    int yawnOsc = sin(now * 0.008) * 4 + 8;
                    display.fillRoundRect(58, 43 - yawnOsc / 2, 12, yawnOsc, 6, SSD1306_WHITE);
                    break;
                }
                case ACT_NAPPING: {
                    display.fillRoundRect(16, 20, 32, 6, 3, SSD1306_WHITE);
                    display.fillRoundRect(80, 20, 32, 6, 3, SSD1306_WHITE);
                    display.drawLine(60, 44, 68, 44, SSD1306_WHITE);

                    int zCycle = (now / 400) % 3;
                    int zX = 108 + zCycle * 4;
                    int zY = 16 - zCycle * 6;
                    display.setTextSize(1);
                    display.setTextColor(SSD1306_WHITE);
                    display.setCursor(zX, zY);
                    display.print("Z");
                    if (zCycle > 0) {
                        display.setCursor(zX - 8, zY + 6);
                        display.print("z");
                    }

                    static unsigned long lastSnoreTime = 0;
                    if (now - lastSnoreTime >= 4000) { 
                        lastSnoreTime = now;
                        triggerSnoozeSound();
                    }
                    break;
                }
            }
            break;
        }
    }
    display.display();
}

// Drive outputs to MX1508 H-Bridge with software Left Motor inversion
void processMovement(String command) {
    unsigned long now = millis();

    // Wake Up Trigger: If napping/yawning and receives a command -> instantly wake up!
    if (command != "STOP" && (currentIdleAct == ACT_NAPPING || currentIdleAct == ACT_YAWNING)) {
        currentIdleAct = ACT_RESTING;
        idleActElapsedTime = 0;
        idleActDuration = random(2000, 10001); 
        triggerConnectedSound(); 
    }

    if (command != currentCmd && command != "STOP" && command != "BACKWARD") {
        triggerActionBlip(); 
    }
    
    currentCmd = command;
    if (command == "FORWARD") {
        digitalWrite(MOTOR_IN1, HIGH); digitalWrite(MOTOR_IN2, LOW);
        digitalWrite(MOTOR_IN3, LOW);  digitalWrite(MOTOR_IN4, HIGH);
    } else if (command == "BACKWARD") {
        digitalWrite(MOTOR_IN1, LOW);  digitalWrite(MOTOR_IN2, HIGH);
        digitalWrite(MOTOR_IN3, HIGH); digitalWrite(MOTOR_IN4, LOW);
    } else if (command == "LEFT") {
        digitalWrite(MOTOR_IN1, HIGH); digitalWrite(MOTOR_IN2, LOW);
        digitalWrite(MOTOR_IN3, HIGH); digitalWrite(MOTOR_IN4, LOW);
    } else if (command == "RIGHT") {
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

// Battery voltage and charging pin monitoring task
void checkBatteryStatus() {
    unsigned long now = millis();
    if (now - lastBatteryCheck < 5000) return; 
    lastBatteryCheck = now;

    int rawAdc = analogRead(BATTERY_PIN);
    float measuredValue = (rawAdc / 4095.0) * 3.3 * 2.0;

    batteryVoltage = (batteryVoltage * 0.8) + (measuredValue * 0.2);

    bool chargePinState = (digitalRead(CHARGING_PIN) == LOW);

    if (chargePinState && !isCharging) {
        isCharging = true;
        triggerChargingPluggedSound();
    } else if (!chargePinState && isCharging) {
        isCharging = false;
        isFullCharge = false;
    }

    if (isCharging) {
        if (batteryVoltage >= 4.12) {
            if (!isFullCharge) {
                isFullCharge = true;
                triggerFullChargeSound();
            }
        } else {
            isFullCharge = false;
        }
        isBatteryLow = false;
    } else {
        isFullCharge = false;
        if (batteryVoltage <= 3.45) {
            if (!isBatteryLow) {
                isBatteryLow = true;
                triggerLowBatterySound();
            }
        } else {
            isBatteryLow = false;
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    pinMode(BUZZER_PIN, OUTPUT);
    noTone(BUZZER_PIN);
    pinMode(CHARGING_PIN, INPUT_PULLUP);

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
    WiFi.softAP("WHEEL-E_AP", "12345678");
    
    // Launch Captive DNS Portal routing
    dnsServer.start(DNS_PORT, "*", local_IP);

    // Serve custom Copper Dashboard
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
    triggerBootSound(); 
    
    lastLoopTime = millis(); 
}

void loop() {
    // --- COMPUTE GLOBAL TIME DELTA ---
    unsigned long now = millis();
    unsigned long dt = now - lastLoopTime;
    lastLoopTime = now;

    // --- ACCUMULATE ELAPSED TIME STRICTLY IF WE ARE STOPPED ---
    if (currentCmd == "STOP") {
        idleActElapsedTime += dt;
        sleepElapsedTime += dt;
    } else {
        sleepElapsedTime = 0; 
    }

    dnsServer.processNextRequest(); 
    server.handleClient();
    webSocket.loop();
    updateSoundEngine(); 
    checkBatteryStatus(); 

    static int lastActiveClients = 0;
    int activeClients = webSocket.connectedClients();

    if (activeClients > lastActiveClients) {
        currentFace = FACE_CONNECTED;
        connectedAnimStart = now;
        triggerConnectedSound(); 
    } else if (activeClients == 0) {
        if (currentFace != FACE_BOOTING && currentFace != FACE_CONNECTING && currentFace != FACE_DISCONNECTED) {
            currentFace = FACE_DISCONNECTED;
            triggerDisconnectedSound(); 
        }
    } else {
        if (currentFace == FACE_CONNECTED) {
            if (now - connectedAnimStart >= connectedAnimDuration) {
                currentFace = FACE_IDLE;
                currentIdleAct = ACT_RESTING;
                idleActElapsedTime = 0;
                idleActDuration = random(2000, 10001); 
            }
        } else {
            if (currentCmd != "STOP") {
                if (currentCmd == "FORWARD") currentFace = FACE_FORWARD;
                else if (currentCmd == "BACKWARD") currentFace = FACE_BACKWARD;
                else if (currentCmd == "LEFT") currentFace = FACE_LEFT;
                else if (currentCmd == "RIGHT") currentFace = FACE_RIGHT;
            } else {
                if (isCharging) {
                    if (isFullCharge) {
                        currentFace = FACE_FULL_CHARGE;
                    } else {
                        currentFace = FACE_CHARGING;
                    }
                } else if (isBatteryLow) {
                    currentFace = FACE_LOW_BATTERY;
                    
                    static unsigned long lastLowBatBeep = 0;
                    if (now - lastLowBatBeep >= 10000) { 
                        lastLowBatBeep = now;
                        triggerLowBatterySound();
                    }
                } else {
                    if (currentFace != FACE_IDLE) {
                        currentFace = FACE_IDLE;
                        currentIdleAct = ACT_RESTING;
                        idleActElapsedTime = 0;
                        idleActDuration = random(2000, 10001); 
                    }
                }
            }
        }
    }

    // --- SLEEP MODE OVERRIDE (2-Minute Inactivity Monitor) ---
    if (currentFace == FACE_IDLE && currentCmd == "STOP" && !isCharging && !isBatteryLow) {
        if (sleepElapsedTime >= sleepThreshold) {
            if (currentIdleAct != ACT_NAPPING && currentIdleAct != ACT_YAWNING) {
                currentIdleAct = ACT_YAWNING;
                idleActElapsedTime = 0;
                idleActDuration = 2500;
                triggerYawnSound();
            }
        }
    }

    // --- DYNAMIC SEQUENTIAL IDLE SCHEDULER (PAUSABLE) ---
    if (currentFace == FACE_IDLE) {
        if (sleepElapsedTime >= sleepThreshold && currentIdleAct == ACT_NAPPING) {
            // LOCKED SLEEP STATE: Infinitely loops here safely until a web/remote button is pressed.
        } 
        else if (idleActElapsedTime >= idleActDuration) {
            idleActElapsedTime = 0; 
            
            if (currentIdleAct == ACT_RESTING) {
                // Transition cleanly into the queued expressive mood
                currentIdleAct = nextExpressiveMood;
                
                switch (currentIdleAct) {
                    case ACT_SINGING:  idleActDuration = 3500; triggerSingingSound(); break;
                    case ACT_GIGGLING: idleActDuration = 3000; triggerGiggleSound(); break;
                    case ACT_LOOKING:  idleActDuration = 4000; triggerLookingSound(); break;
                    case ACT_DIZZY:    idleActDuration = 3500; triggerDizzySound(); break;
                    case ACT_POUTING:  idleActDuration = 4000; triggerPoutingSound(); break;
                    case ACT_SIGHING:  idleActDuration = 3000; triggerSighSound(); break;
                    case ACT_SNEEZING: idleActDuration = 3500; triggerSneezeSound(); break;
                    case ACT_YAWNING:  idleActDuration = 2500; triggerYawnSound(); break;
                    case ACT_NAPPING:  idleActDuration = 12000; break; 
                    default:           currentIdleAct = ACT_RESTING; break;
                }
            } 
            else {
                // Save what just ran so we can determine structural hand-offs
                IdleActivity finishedAct = currentIdleAct;

                // Advance track timeline queue pointer
                switch (finishedAct) {
                    case ACT_SINGING:  nextExpressiveMood = ACT_GIGGLING; break;
                    case ACT_GIGGLING: nextExpressiveMood = ACT_LOOKING; break;
                    case ACT_LOOKING:  nextExpressiveMood = ACT_DIZZY; break;
                    case ACT_DIZZY:    nextExpressiveMood = ACT_POUTING; break;
                    case ACT_POUTING:  nextExpressiveMood = ACT_SIGHING; break;
                    case ACT_SIGHING:  nextExpressiveMood = ACT_SNEEZING; break;
                    case ACT_SNEEZING: nextExpressiveMood = ACT_YAWNING; break;
                    case ACT_YAWNING:  nextExpressiveMood = ACT_NAPPING; break;
                    case ACT_NAPPING:  nextExpressiveMood = ACT_SINGING; break;
                    default:           nextExpressiveMood = ACT_SINGING; break;
                }
                
                // CRITICAL FIX: If a yawn finished, step directly into deep sleep (ACT_NAPPING).
                // Do NOT drop back to ACT_RESTING first, which breaks the threshold logic filter!
                if (finishedAct == ACT_YAWNING) {
                    currentIdleAct = ACT_NAPPING;
                    idleActDuration = 12000; 
                } else {
                    // Otherwise, safely take a quiet resting pause for 2-10 seconds
                    currentIdleAct = ACT_RESTING;
                    idleActDuration = random(2000, 10001); 
                }
            }
        }
    }
    
    lastActiveClients = activeClients;

    // --- 30 FPS DISPLAY FRAME LIMITER ---
    static unsigned long lastDisplayUpdate = 0;
    if (now - lastDisplayUpdate >= 33) { 
        lastDisplayUpdate = now;
        updateFaceAnimation();
    }
}