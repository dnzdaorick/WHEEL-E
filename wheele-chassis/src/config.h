#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <DNSServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Pin Assignments (DO NOT CHANGE) ───────────────────────────────────────────
#define MOTOR_IN4     0
#define MOTOR_IN3     1
#define MOTOR_IN2     2
#define MOTOR_IN1     3
#define OLED_SDA      8
#define OLED_SCL      9
#define BUZZER_PIN   10
#define BATTERY_PIN   4    // ADC for battery voltage divider

// ── Display ───────────────────────────────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64

// ── Network ───────────────────────────────────────────────────────────────────
#define DNS_PORT    53
#define AP_SSID     "WHEEL-E"   // Wi-Fi Access Point name
#define AP_PASSWORD "12345678"     // Min 8 chars required by WPA2

// ── ADC / Battery ─────────────────────────────────────────────────────────────
#define ADC_MAX_VAL     4095
#define VDIVIDER_RATIO  2.0f   // Voltage divider ratio on BATTERY_PIN
#define VREF            3.3f   // ESP32-C3 ADC reference voltage

// ── Face & Idle State Enumerations ────────────────────────────────────────────
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
    FACE_LOW_BATTERY,
    FACE_MUSIC_MODE
};

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

// ── Shared Hardware Objects (defined in main.cpp) ─────────────────────────────
extern Adafruit_SSD1306 display;
extern WebSocketsServer webSocket;

// ── Shared Runtime State (defined in main.cpp) ────────────────────────────────
extern char         currentCmd[16];      // Active movement/music command
extern bool         isMusicMode;
extern bool         isBatteryLow;
extern FaceState    currentFace;
extern IdleActivity currentIdleAct;
extern unsigned long idleActElapsedTime;
extern unsigned long idleActDuration;
extern unsigned long sleepElapsedTime;
extern unsigned long musicInactivityElapsedTime;
