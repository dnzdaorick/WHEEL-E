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
#define OLED_SDA 8
#define OLED_SCL 9
#define BUZZER_PIN 10
#define BATTERY_PIN 4       // ADC for battery voltage divider
#define CHARGING_PIN 5      // Digital input for charging status (LOW = charging)

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

// Dashboard HTML interface
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
            padding: 40px 25px;
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
            transition: all 0.5s ease;
        }
        .card.music-mode-active h1 {
            color: #ffb703;
            text-shadow: 0 0 18px rgba(255, 183, 3, 0.7);
            letter-spacing: 5px;
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
            touch-action: none; 
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
            transition: background 0.08s ease, border-color 0.08s ease, box-shadow 0.08s ease;
        }
        .btn.sliding-active {
            background: #e07a5f;
            color: #0c0604;
            border-color: #e07a5f;
            box-shadow: 0 0 25px rgba(224, 122, 95, 0.55);
            transform: scale(0.94);
        }
        .btn.music-only-pad {
            opacity: 0;
            pointer-events: none;
            visibility: hidden;
            transition: opacity 0.3s ease;
        }
        .card.music-mode-active .btn {
            border-color: rgba(255, 183, 3, 0.3);
            background: rgba(255, 183, 3, 0.03);
            font-size: 1.3rem;
            font-weight: bold;
            color: #ffeeb2;
            opacity: 1;
            pointer-events: auto;
            visibility: visible;
        }
        .card.music-mode-active .btn.sliding-active {
            background: #ffb703;
            color: #023047;
            border-color: #ffb703;
            box-shadow: 0 0 25px rgba(255, 183, 3, 0.6);
        }
    </style>
</head>
<body>
    <div class="card" id="dashboardCard">
        <h1 id="mainTitle">WHEEL-E</h1>
        <div class="status-bar"><span class="dot" id="dot"></span> <span id="lbl">DISCONNECTED</span></div>
        <div class="control-grid" id="controlGrid">
            <div class="btn music-only-pad" id="nw" data-cmd="NW"></div>
            <div class="btn" id="up" data-cmd="FORWARD">▲</div>
            <div class="btn music-only-pad" id="ne" data-cmd="NE"></div>
            <div class="btn" id="left" data-cmd="LEFT">◀</div>
            <div class="btn music-only-pad" id="center" data-cmd="CENTER"></div>
            <div class="btn" id="right" data-cmd="RIGHT">▶</div>
            <div class="btn music-only-pad" id="sw" data-cmd="SW"></div>
            <div class="btn" id="down" data-cmd="BACKWARD">▼</div>
            <div class="btn music-only-pad" id="se" data-cmd="SE"></div>
        </div>
    </div>
    <script>
        let ws;
        const dot = document.getElementById('dot');
        const lbl = document.getElementById('lbl');
        const card = document.getElementById('dashboardCard');
        const title = document.getElementById('mainTitle');
        const grid = document.getElementById('controlGrid');
        
        let isMouseDown = false;
        let lastSentCommandString = "STOP";
        let lastValidCommand = "STOP";
        let lastCommandTime = 0;
        const commandDebounce = 50;  // Minimum 50ms between command updates
        
        function connect() {
            ws = new WebSocket('ws://' + window.location.hostname + ':81/');
            ws.onopen = () => {
                dot.classList.add('connected');
                lbl.innerText = 'CONNECTED';
            };
            ws.onmessage = (evt) => {
                if (evt.data === "UI_MUSIC") setMusicUI(true);
                else if (evt.data === "UI_DRIVE") setMusicUI(false);
            };
            ws.onclose = () => {
                dot.classList.remove('connected');
                lbl.innerText = 'DISCONNECTED';
                setMusicUI(false);
                setTimeout(connect, 2000);
            };
        }
        
        function setMusicUI(isMusic) {
            if (isMusic) {
                card.classList.add('music-mode-active');
                title.innerText = "SYNTHESIZER";
                document.getElementById('nw').innerText = "C5";
                document.getElementById('up').innerText = "D5";
                document.getElementById('ne').innerText = "E5";
                document.getElementById('left').innerText = "F5";
                document.getElementById('center').innerText = "G5";
                document.getElementById('right').innerText = "A5";
                document.getElementById('sw').innerText = "B5";
                document.getElementById('down').innerText = "C6";
                document.getElementById('se').innerText = "D6";
            } else {
                card.classList.remove('music-mode-active');
                title.innerText = "WHEEL-E";
                document.getElementById('nw').innerText = "";
                document.getElementById('up').innerText = "▲";
                document.getElementById('ne').innerText = "";
                document.getElementById('left').innerText = "◀";
                document.getElementById('center').innerText = "";
                document.getElementById('right').innerText = "▶";
                document.getElementById('sw').innerText = "";
                document.getElementById('down').innerText = "▼";
                document.getElementById('se').innerText = "";
            }
        }
        
        function send(cmd) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(cmd);
            }
        }

        function evaluateInputPads(eventPoints, isEndEvent = false) {
            const now = Date.now();
            let activeCmds = [];
            let activeElements = [];
            const isMusic = card.classList.contains('music-mode-active');

            if (!isMusic) {
                // Drive mode: strict single command, reject multi-touch
                // If multiple touch points in drive mode, ignore all of them
                if (eventPoints.length > 1) {
                    // Multi-touch detected in drive mode - ignore
                    if (isEndEvent) {
                        activeCmds = [];
                        lastValidCommand = "STOP";
                    } else {
                        // Keep the last valid command during multi-touch
                        activeCmds = [lastValidCommand];
                    }
                } else if (eventPoints.length === 1) {
                    // Single touch - process normally
                    const element = document.elementFromPoint(eventPoints[0].x, eventPoints[0].y);
                    if (element && element.classList.contains('btn')) {
                        if (window.getComputedStyle(element).visibility !== 'hidden') {
                            const cmd = element.getAttribute('data-cmd');
                            activeCmds.push(cmd);
                            activeElements.push(element);
                            lastValidCommand = cmd;
                        }
                    } else if (!isEndEvent) {
                        // No button found during move - keep last valid command
                        activeCmds = [lastValidCommand];
                    }
                } else if (isEndEvent) {
                    // Touch released
                    activeCmds = [];
                    lastValidCommand = "STOP";
                } else {
                    // No points and not end event
                    activeCmds = [lastValidCommand];
                }
            } else {
                // Synthesizer mode: full polyphonic support with all touch points
                eventPoints.forEach(pt => {
                    const element = document.elementFromPoint(pt.x, pt.y);
                    if (element && element.classList.contains('btn')) {
                        if (window.getComputedStyle(element).visibility !== 'hidden') {
                            let cmd = element.getAttribute('data-cmd');
                            if (!activeCmds.includes(cmd)) {
                                activeCmds.push(cmd);
                                activeElements.push(element);
                            }
                        }
                    }
                });
            }

            document.querySelectorAll('.btn').forEach(b => {
                if (activeElements.includes(b)) b.classList.add('sliding-active');
                else b.classList.remove('sliding-active');
            });

            let commandString = activeCmds.length > 0 ? activeCmds.sort().join(",") : "STOP";
            
            // Always send STOP immediately, otherwise apply debounce
            if (commandString !== lastSentCommandString &&
                (commandString === "STOP" || now - lastCommandTime >= commandDebounce)) {
                lastSentCommandString = commandString;
                lastCommandTime = now;
                send(commandString);
            }
        }

        function releaseInputPads() {
            isMouseDown = false;
            lastValidCommand = "STOP";
            evaluateInputPads([], true);
            document.querySelectorAll('.btn').forEach(b => b.classList.remove('sliding-active'));
        }

        window.addEventListener('mousedown', (e) => {
            isMouseDown = true;
            updateMouseList(e);
        });
        window.addEventListener('mousemove', (e) => {
            if (isMouseDown) updateMouseList(e);
        });
        window.addEventListener('mouseup', releaseInputPads);
        window.addEventListener('mouseleave', releaseInputPads);
        window.addEventListener('blur', releaseInputPads);

        function updateMouseList(e) {
            evaluateInputPads([{ x: e.clientX, y: e.clientY }], false);
        }

        grid.addEventListener('touchstart', parseTouchList, { passive: false });
        grid.addEventListener('touchmove', parseTouchList, { passive: false });
        grid.addEventListener('touchend', parseTouchList, { passive: false });
        grid.addEventListener('touchcancel', parseTouchList, { passive: false });
        function parseTouchList(e) {
            e.preventDefault();
            let points = [];
            for (let i = 0; i < e.touches.length; i++) {
                points.push({ x: e.touches[i].clientX, y: e.touches[i].clientY });
            }
            const isEndEvent = e.type === 'touchend' || e.type === 'touchcancel';
            evaluateInputPads(points, isEndEvent);
        }
        
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
    FACE_LOW_BATTERY,
    FACE_MUSIC_MODE   
};

FaceState currentFace = FACE_BOOTING;
String currentCmd = "STOP";

// Idle activity states for companion mood rotation
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

// Timing variables for animation and activity states
unsigned long lastLoopTime = 0;
unsigned long idleActElapsedTime = 0;
unsigned long idleActDuration = 5000;
unsigned long sleepElapsedTime = 0;
const unsigned long sleepThreshold = 120000; 

// Music mode state and combo gesture tracking
bool isMusicMode = false;
unsigned long lastComboTapTime = 0;
int comboStateStep = 0; 

// Polyphonic audio frequency tracking
int activeFrequencies[9];
int activeFreqCount = 0;
unsigned long lastArpeggioFrameTime = 0;
int arpeggioIndexPointer = 0;

unsigned long musicNoteOnTime = 0;
bool musicNotePendingStop = false;
unsigned long musicInactivityElapsedTime = 0; 

// Display animation timing
unsigned long lastBlinkTime = 0;
bool isBlinking = false;
unsigned long blinkDuration = 180;
unsigned long blinkInterval = 3500;
unsigned long connectedAnimStart = 0;
const unsigned long connectedAnimDuration = 2000;

// Sound note definition
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

void triggerMovementNote(int frequency) {
    currentMelody[0] = {frequency, 140}; 
    playMelody(1);
}

void triggerMusicModeUnlockSound() {
    currentMelody[0] = {523, 60};   
    currentMelody[1] = {659, 60};   
    currentMelody[2] = {784, 60};   
    currentMelody[3] = {1047, 60};  
    currentMelody[4] = {1319, 60};  
    currentMelody[5] = {1568, 200}; 
    playMelody(6);
}

void triggerMusicModeExitSound() {
    currentMelody[0] = {1047, 80};  
    currentMelody[1] = {784, 80};   
    currentMelody[2] = {659, 80};   
    currentMelody[3] = {523, 200};  
    playMelody(4);
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
    unsigned long now = millis();

    // 1. CHIPTUNE CHORD MULTIPLEXER (Rapid square-wave multiplexing runs strictly for chords)
    if (isMusicMode && activeFreqCount > 1) {
        if (now - lastArpeggioFrameTime >= 18) { 
            lastArpeggioFrameTime = now;
            arpeggioIndexPointer = (arpeggioIndexPointer + 1) % activeFreqCount;
            tone(BUZZER_PIN, activeFrequencies[arpeggioIndexPointer]);
        }
        return; 
    }

    // Envelope release for single note taps
    if (isMusicMode && musicNotePendingStop) {
        if (now - musicNoteOnTime >= 150) { 
            noTone(BUZZER_PIN);
            musicNotePendingStop = false;
        }
    }

    // Process automated sound sequences
    if (isPlayingSound) {
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

    // Continuous reverse warning beep
    if (currentCmd == "BACKWARD" && !isMusicMode) {
        if (!isPlayingSound) { 
            if (now - lastBackupBeep >= 250) { 
                lastBackupBeep = now;
                backupBeepState = !backupBeepState;
                if (backupBeepState) {
                    tone(BUZZER_PIN, 349); 
                } else {
                    noTone(BUZZER_PIN);
                }
            }
        }
    } else {
        if (backupBeepState) {
            noTone(BUZZER_PIN);
            backupBeepState = false;
        }
    }
}

// Display boot status text
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
        case FACE_MUSIC_MODE: {
            display.fillCircle(10, 42, 4, SSD1306_WHITE);
            display.fillCircle(118, 42, 4, SSD1306_WHITE);

            int singBounce = sin(now * 0.012) * 2;
            display.drawCircle(32, 28 + singBounce, 12, SSD1306_WHITE);
            display.fillRect(16, 28 + singBounce, 32, 14, SSD1306_BLACK);
            display.drawCircle(96, 28 + singBounce, 12, SSD1306_WHITE);
            display.fillRect(80, 28 + singBounce, 32, 14, SSD1306_BLACK);

            int dynamicMouth = (activeFreqCount > 0) ? 14 : (sin(now * 0.015) * 3 + 8);
            display.fillCircle(64, 44, dynamicMouth, SSD1306_WHITE);

            for (int i = 0; i < 2; i++) {
                int noteShift = sin((now + (i * 2000)) * 0.008) * 5;
                int noteX = 14 + (i * 96) + noteShift / 2;
                int noteY = 18 + noteShift;
                display.fillCircle(noteX, noteY, 2, SSD1306_WHITE);
                display.drawLine(noteX + 2, noteY, noteX + 2, noteY - 6, SSD1306_WHITE);
                display.drawLine(noteX + 2, noteY - 6, noteX + 5, noteY - 5, SSD1306_WHITE);
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

                    int crossX = 112; float crossY = 10;
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
                    break;
                }
            }
            break;
        }
    }
    display.display();
}

// Process movement commands to motor controller
void processMovement(String command) {
    unsigned long now = millis();

    // Wake up from sleep/yawn on movement command
    if (command != "STOP" && (currentIdleAct == ACT_NAPPING || currentIdleAct == ACT_YAWNING)) {
        currentIdleAct = ACT_RESTING;
        idleActElapsedTime = 0;
        idleActDuration = random(2000, 10001); 
        triggerConnectedSound(); 
    }

    // Ensure single command in drive mode (filter out compound commands)
    if (!isMusicMode && command.indexOf(',') >= 0) {
        command = command.substring(0, command.indexOf(','));
    }

    // Music mode unlock combo: LEFT -> RIGHT -> LEFT -> RIGHT
    if (command != "STOP" && command != currentCmd && command.indexOf(",") < 0) {
        if (now - lastComboTapTime > 2000) {
            comboStateStep = 0; 
        }
        lastComboTapTime = now;

        if (comboStateStep == 0 && command == "LEFT") comboStateStep = 1;
        else if (comboStateStep == 1 && command == "RIGHT") comboStateStep = 2;
        else if (comboStateStep == 2 && command == "LEFT") comboStateStep = 3;
        else if (comboStateStep == 3 && command == "RIGHT") {
            comboStateStep = 0; 
            
            if (!isMusicMode) {
                isMusicMode = true;
                currentFace = FACE_MUSIC_MODE;
                triggerMusicModeUnlockSound(); 
                webSocket.broadcastTXT("UI_MUSIC"); 
            } else {
                isMusicMode = false;
                currentFace = FACE_IDLE;
                currentIdleAct = ACT_RESTING;
                idleActElapsedTime = 0;
                idleActDuration = random(2000, 10001);
                sleepElapsedTime = 0; 
                triggerMusicModeExitSound(); 
                webSocket.broadcastTXT("UI_DRIVE"); 
            }
            currentCmd = "STOP";
            activeFreqCount = 0;
            musicInactivityElapsedTime = 0;
            noTone(BUZZER_PIN);
            return;
        } else {
            comboStateStep = (command == "LEFT") ? 1 : 0;
        }
    }

    // Music mode note envelope and frequency processing
    if (isMusicMode) {
        if (command == "STOP") {
            // Release note after envelope duration
            if (now - musicNoteOnTime >= 150) {
                noTone(BUZZER_PIN);
                musicNotePendingStop = false;
            } else {
                musicNotePendingStop = true;
            }
            activeFreqCount = 0; 
        } else {
            activeFreqCount = 0;
            if (command.indexOf("NW") >= 0)      activeFrequencies[activeFreqCount++] = 523;
            if (command.indexOf("FORWARD") >= 0) activeFrequencies[activeFreqCount++] = 587;
            if (command.indexOf("NE") >= 0)      activeFrequencies[activeFreqCount++] = 659;
            if (command.indexOf("LEFT") >= 0)     activeFrequencies[activeFreqCount++] = 698;
            if (command.indexOf("CENTER") >= 0)   activeFrequencies[activeFreqCount++] = 784;
            if (command.indexOf("RIGHT") >= 0)    activeFrequencies[activeFreqCount++] = 880;
            if (command.indexOf("SW") >= 0)      activeFrequencies[activeFreqCount++] = 988;
            if (command.indexOf("BACKWARD") >= 0) activeFrequencies[activeFreqCount++] = 1047;
            if (command.indexOf("SE") >= 0)      activeFrequencies[activeFreqCount++] = 1175; 

            if (activeFreqCount > 0) {
                isPlayingSound = false;
                currentNoteIndex = -1;
                musicNoteOnTime = now;
                musicNotePendingStop = false;

                // Play single note immediately without chord arpeggiation
                if (activeFreqCount == 1) {
                    tone(BUZZER_PIN, activeFrequencies[0]);
                }
            }
        }
        currentCmd = command;
        digitalWrite(MOTOR_IN1, LOW); digitalWrite(MOTOR_IN2, LOW);
        digitalWrite(MOTOR_IN3, LOW); digitalWrite(MOTOR_IN4, LOW);
        return; 
    }

    // Drive mode movement with directional audio feedback
    if (command != currentCmd && command != "STOP") {
        if (command == "FORWARD")       triggerMovementNote(523); 
        else if (command == "BACKWARD")  triggerMovementNote(587); 
        else if (command == "LEFT")      triggerMovementNote(659); 
        else if (command == "RIGHT")     triggerMovementNote(698); 
    }
    
    currentCmd = command;
    if (command == "FORWARD") {
        digitalWrite(MOTOR_IN1, HIGH);
        digitalWrite(MOTOR_IN2, LOW);
        digitalWrite(MOTOR_IN3, LOW);
        digitalWrite(MOTOR_IN4, HIGH);
    } else if (command == "BACKWARD") {
        digitalWrite(MOTOR_IN1, LOW);
        digitalWrite(MOTOR_IN2, HIGH);
        digitalWrite(MOTOR_IN3, HIGH);
        digitalWrite(MOTOR_IN4, LOW);
    } else if (command == "LEFT") {
        digitalWrite(MOTOR_IN1, HIGH);
        digitalWrite(MOTOR_IN2, LOW);
        digitalWrite(MOTOR_IN3, HIGH);
        digitalWrite(MOTOR_IN4, LOW);
    } else if (command == "RIGHT") {
        digitalWrite(MOTOR_IN1, LOW);
        digitalWrite(MOTOR_IN2, HIGH);
        digitalWrite(MOTOR_IN3, LOW);
        digitalWrite(MOTOR_IN4, HIGH);
    } else {
        digitalWrite(MOTOR_IN1, LOW);
        digitalWrite(MOTOR_IN2, LOW);
        digitalWrite(MOTOR_IN3, LOW);
        digitalWrite(MOTOR_IN4, LOW);
    }
}

// Handle WebSocket events from dashboard
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
        String cmd = String((char*)payload);
        processMovement(cmd);
    } else if (type == WStype_CONNECTED) {
        if (isMusicMode) {
            webSocket.sendTXT(num, "UI_MUSIC");
        } else {
            webSocket.sendTXT(num, "UI_DRIVE");
        }
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
    // Calculate delta time for frame updates
    unsigned long now = millis();
    unsigned long dt = now - lastLoopTime;
    lastLoopTime = now;

    // Accumulate idle time when stopped
    if (currentCmd == "STOP" && !isMusicMode) {
        idleActElapsedTime += dt;
        sleepElapsedTime += dt;
    } else {
        sleepElapsedTime = 0; 
    }

    // Auto-exit music mode after 10 seconds of inactivity
    if (isMusicMode) {
        if (currentCmd == "STOP") {
            musicInactivityElapsedTime += dt;
            if (musicInactivityElapsedTime >= 10000) { 
                isMusicMode = false;
                musicInactivityElapsedTime = 0;
                currentFace = FACE_IDLE;
                currentIdleAct = ACT_RESTING;
                idleActElapsedTime = 0;
                idleActDuration = random(2000, 10001);
                sleepElapsedTime = 0; 
                triggerMusicModeExitSound(); 
                webSocket.broadcastTXT("UI_DRIVE"); 
            }
        } else {
            musicInactivityElapsedTime = 0; 
        }
    } else {
        musicInactivityElapsedTime = 0;
    }

    dnsServer.processNextRequest(); 
    server.handleClient();
    webSocket.loop();
    updateSoundEngine(); 
    checkBatteryStatus(); 

    static int lastActiveClients = 0;
    int activeClients = webSocket.connectedClients();

    if (activeClients > lastActiveClients) {
        currentFace = isMusicMode ? FACE_MUSIC_MODE : FACE_CONNECTED;
        connectedAnimStart = now;
        triggerConnectedSound(); 
    } else if (activeClients == 0) {
        if (currentFace != FACE_BOOTING && currentFace != FACE_CONNECTING && currentFace != FACE_DISCONNECTED) {
            currentFace = FACE_DISCONNECTED;
            isMusicMode = false; 
            triggerDisconnectedSound(); 
        }
    } else {
        if (currentFace == FACE_CONNECTED && !isMusicMode) {
            if (now - connectedAnimStart >= connectedAnimDuration) {
                currentFace = FACE_IDLE;
                currentIdleAct = ACT_RESTING;
                idleActElapsedTime = 0;
                idleActDuration = random(2000, 10001); 
            }
        } else {
            if (isMusicMode) {
                currentFace = FACE_MUSIC_MODE;
            } else if (currentCmd != "STOP") {
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

    // Enter sleep mode after 2 minutes of inactivity
    if (currentFace == FACE_IDLE && currentCmd == "STOP" && !isCharging && !isBatteryLow && !isMusicMode) {
        if (sleepElapsedTime >= sleepThreshold) {
            if (currentIdleAct != ACT_NAPPING && currentIdleAct != ACT_YAWNING) {
                currentIdleAct = ACT_YAWNING;
                idleActElapsedTime = 0;
                idleActDuration = 2500;
                triggerYawnSound();
            }
        }
    }

    // Idle activity animation scheduler
    if (currentFace == FACE_IDLE && !isMusicMode) {
        if (sleepElapsedTime >= sleepThreshold && currentIdleAct == ACT_NAPPING) {
            // LOCKED SLEEP STATE
        } 
        else if (idleActElapsedTime >= idleActDuration) {
            idleActElapsedTime = 0; 
            
            if (currentIdleAct == ACT_RESTING) {
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
                IdleActivity finishedAct = currentIdleAct;

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
                
                if (finishedAct == ACT_YAWNING) {
                    currentIdleAct = ACT_NAPPING;
                    idleActDuration = 12000; 
                } else {
                    currentIdleAct = ACT_RESTING;
                    idleActDuration = random(2000, 10001); 
                }
            }
        }
    }
    
    lastActiveClients = activeClients;

    // Update display at 30 FPS
    static unsigned long lastDisplayUpdate = 0;
    if (now - lastDisplayUpdate >= 33) {
        lastDisplayUpdate = now;
        updateFaceAnimation();
    }
}