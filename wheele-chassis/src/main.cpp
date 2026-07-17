#include "config.h"
#include "web_ui.h"
#include "sound.h"
#include "face.h"
#include "movement.h"
#include "battery.h"

// ── Hardware Objects ──────────────────────────────────────────────────────────
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

IPAddress local_IP(192, 168, 67, 1);
IPAddress gateway(192, 168, 67, 1);
IPAddress subnet(255, 255, 255, 0);

DNSServer        dnsServer;
WebServer        server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// ── Shared Runtime State ──────────────────────────────────────────────────────
char         currentCmd[16]            = "STOP";
bool         isMusicMode               = false;
bool         isCharging                = false;
bool         isFullCharge              = false;
bool         isBatteryLow             = false;
FaceState    currentFace               = FACE_BOOTING;
IdleActivity currentIdleAct            = ACT_RESTING;
IdleActivity nextExpressiveMood        = ACT_SINGING;
unsigned long idleActElapsedTime       = 0;
unsigned long idleActDuration          = 5000;
unsigned long sleepElapsedTime         = 0;
unsigned long musicInactivityElapsedTime = 0;
const unsigned long sleepThreshold     = 120000;

// ── Loop Timing ───────────────────────────────────────────────────────────────
static unsigned long lastLoopTime = 0;

// ── WebSocket Event Handler ───────────────────────────────────────────────────
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t /*length*/) {
    if (type == WStype_TEXT) {
        processMovement((char*)payload);
    } else if (type == WStype_CONNECTED) {
        webSocket.sendTXT(num, isMusicMode ? "UI_MUSIC" : "UI_DRIVE");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    // ── ADC ─────────────────────────────────────────────────────────────────
    // Set 11 dB attenuation for full 0–3.3 V input range on BATTERY_PIN
    analogSetAttenuation(ADC_11db);

    // ── Motors ───────────────────────────────────────────────────────────────
    pinMode(MOTOR_IN1, OUTPUT);
    pinMode(MOTOR_IN2, OUTPUT);
    pinMode(MOTOR_IN3, OUTPUT);
    pinMode(MOTOR_IN4, OUTPUT);
    processMovement("STOP");

    // ── Buzzer ───────────────────────────────────────────────────────────────
    pinMode(BUZZER_PIN, OUTPUT);
    noTone(BUZZER_PIN);

    // ── Charging Pin ─────────────────────────────────────────────────────────
    pinMode(CHARGING_PIN, INPUT_PULLUP);

    // ── I2C + OLED ───────────────────────────────────────────────────────────
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(400000);   // Fast-mode: 400 kHz — 4× faster display frames
    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        display.clearDisplay();
        displayStatusLine("BOOTING AP...");
    }

    // ── Wi-Fi Access Point ───────────────────────────────────────────────────
    WiFi.softAPConfig(local_IP, gateway, subnet);
    // Limit to 1 simultaneous station — reduces AP overhead & interference
    WiFi.softAP("WHEEL-E_AP", "12345678", 1, 0, 1);

    // ── Captive DNS Portal ────────────────────────────────────────────────────
    dnsServer.start(DNS_PORT, "*", local_IP);

    // ── HTTP Server ───────────────────────────────────────────────────────────
    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html", INDEX_HTML);
    });
    server.onNotFound([]() {
        server.sendHeader("Location", "http://192.168.67.1/", true);
        server.send(302, "text/plain", "");
    });
    server.begin();

    // ── WebSocket Server ─────────────────────────────────────────────────────
    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);

    currentFace = FACE_CONNECTING;
    triggerBootSound();
    lastLoopTime = millis();
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    unsigned long now = millis();
    unsigned long dt  = now - lastLoopTime;
    lastLoopTime = now;

    // ── Idle / Sleep Timers ───────────────────────────────────────────────────
    if (strcmp(currentCmd, "STOP") == 0 && !isMusicMode) {
        idleActElapsedTime += dt;
        sleepElapsedTime   += dt;
    } else {
        sleepElapsedTime = 0;
    }

    // ── Music Mode Inactivity Auto-Exit (10 s) ────────────────────────────────
    if (isMusicMode) {
        if (strcmp(currentCmd, "STOP") == 0) {
            musicInactivityElapsedTime += dt;
            if (musicInactivityElapsedTime >= 10000) {
                isMusicMode                = false;
                musicInactivityElapsedTime = 0;
                currentFace                = FACE_IDLE;
                currentIdleAct             = ACT_RESTING;
                idleActElapsedTime         = 0;
                idleActDuration            = random(2000, 10000);
                sleepElapsedTime           = 0;
                triggerMusicModeExitSound();
                webSocket.broadcastTXT("UI_DRIVE");
            }
        } else {
            musicInactivityElapsedTime = 0;
        }
    } else {
        musicInactivityElapsedTime = 0;
    }

    // ── Network / Service Polling ─────────────────────────────────────────────
    dnsServer.processNextRequest();
    server.handleClient();
    webSocket.loop();

    // ── Subsystem Updates ─────────────────────────────────────────────────────
    updateSoundEngine();
    checkBatteryStatus();

    // ── Connection State Machine ──────────────────────────────────────────────
    static int lastActiveClients = 0;
    int activeClients = webSocket.connectedClients();

    if (activeClients > lastActiveClients) {
        // New client connected (0→1 transition with AP limited to 1 client)
        currentFace        = isMusicMode ? FACE_MUSIC_MODE : FACE_CONNECTED;
        connectedAnimStart = now;
        triggerConnectedSound();
    } else if (activeClients == 0) {
        if (currentFace != FACE_BOOTING &&
            currentFace != FACE_CONNECTING &&
            currentFace != FACE_DISCONNECTED) {
            currentFace = FACE_DISCONNECTED;
            isMusicMode = false;
            triggerDisconnectedSound();
        }
    } else {
        if (currentFace == FACE_CONNECTED && !isMusicMode) {
            if (now - connectedAnimStart >= connectedAnimDuration) {
                currentFace        = FACE_IDLE;
                currentIdleAct     = ACT_RESTING;
                idleActElapsedTime = 0;
                idleActDuration    = random(2000, 10000);
            }
        } else if (isMusicMode) {
            currentFace = FACE_MUSIC_MODE;
        } else if (strcmp(currentCmd, "STOP") != 0) {
            if      (strcmp(currentCmd, "FORWARD")  == 0) currentFace = FACE_FORWARD;
            else if (strcmp(currentCmd, "BACKWARD") == 0) currentFace = FACE_BACKWARD;
            else if (strcmp(currentCmd, "LEFT")     == 0) currentFace = FACE_RIGHT;
            else if (strcmp(currentCmd, "RIGHT")    == 0) currentFace = FACE_LEFT;
        } else {
            // Stopped — show battery / idle face
            if (isCharging) {
                currentFace = isFullCharge ? FACE_FULL_CHARGE : FACE_CHARGING;
            } else if (isBatteryLow) {
                currentFace = FACE_LOW_BATTERY;
                static unsigned long lastLowBatBeep = 0;
                if (now - lastLowBatBeep >= 10000) {
                    lastLowBatBeep = now;
                    triggerLowBatterySound();
                }
            } else {
                if (currentFace != FACE_IDLE) {
                    currentFace        = FACE_IDLE;
                    currentIdleAct     = ACT_RESTING;
                    idleActElapsedTime = 0;
                    idleActDuration    = random(2000, 10000);
                }
            }
        }
    }

    // ── Sleep Transition (2 min inactivity) ──────────────────────────────────
    if (currentFace == FACE_IDLE && strcmp(currentCmd, "STOP") == 0 &&
        !isCharging && !isBatteryLow && !isMusicMode) {
        if (sleepElapsedTime >= sleepThreshold) {
            if (currentIdleAct != ACT_NAPPING && currentIdleAct != ACT_YAWNING) {
                currentIdleAct  = ACT_YAWNING;
                idleActElapsedTime = 0;
                idleActDuration = 2500;
                triggerYawnSound();
            }
        }
    }

    // ── Idle Activity Scheduler ───────────────────────────────────────────────
    if (currentFace == FACE_IDLE && !isMusicMode) {
        // Locked in sleep state
        if (sleepElapsedTime >= sleepThreshold && currentIdleAct == ACT_NAPPING) {
            // intentionally empty
        } else if (idleActElapsedTime >= idleActDuration) {
            idleActElapsedTime = 0;

            if (currentIdleAct == ACT_RESTING) {
                currentIdleAct = nextExpressiveMood;
                switch (currentIdleAct) {
                    case ACT_SINGING:  idleActDuration = 3500; triggerSingingSound(); break;
                    case ACT_GIGGLING: idleActDuration = 3000; triggerGiggleSound();  break;
                    case ACT_LOOKING:  idleActDuration = 4000; triggerLookingSound(); break;
                    case ACT_DIZZY:    idleActDuration = 3500; triggerDizzySound();   break;
                    case ACT_POUTING:  idleActDuration = 4000; triggerPoutingSound(); break;
                    case ACT_SIGHING:  idleActDuration = 3000; triggerSighSound();    break;
                    case ACT_SNEEZING: idleActDuration = 3500; triggerSneezeSound();  break;
                    case ACT_YAWNING:  idleActDuration = 2500; triggerYawnSound();    break;
                    case ACT_NAPPING:  idleActDuration = 12000; break;
                    default:           currentIdleAct = ACT_RESTING; break;
                }
            } else {
                IdleActivity finished = currentIdleAct;
                switch (finished) {
                    case ACT_SINGING:  nextExpressiveMood = ACT_GIGGLING; break;
                    case ACT_GIGGLING: nextExpressiveMood = ACT_LOOKING;  break;
                    case ACT_LOOKING:  nextExpressiveMood = ACT_DIZZY;    break;
                    case ACT_DIZZY:    nextExpressiveMood = ACT_POUTING;  break;
                    case ACT_POUTING:  nextExpressiveMood = ACT_SIGHING;  break;
                    case ACT_SIGHING:  nextExpressiveMood = ACT_SNEEZING; break;
                    case ACT_SNEEZING: nextExpressiveMood = ACT_YAWNING;  break;
                    case ACT_YAWNING:  nextExpressiveMood = ACT_NAPPING;  break;
                    case ACT_NAPPING:  nextExpressiveMood = ACT_SINGING;  break;
                    default:           nextExpressiveMood = ACT_SINGING;  break;
                }
                if (finished == ACT_YAWNING) {
                    currentIdleAct  = ACT_NAPPING;
                    idleActDuration = 12000;
                } else {
                    currentIdleAct  = ACT_RESTING;
                    idleActDuration = random(2000, 10000);
                }
            }
        }
    }

    lastActiveClients = activeClients;

    // ── Display at 30 FPS ─────────────────────────────────────────────────────
    static unsigned long lastDisplayUpdate = 0;
    if (now - lastDisplayUpdate >= 33) {
        lastDisplayUpdate = now;
        updateFaceAnimation();
    }
}