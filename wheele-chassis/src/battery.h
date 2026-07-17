#pragma once

#include "config.h"

// ── Battery Monitor State (defined in battery.cpp) ───────────────────────────
extern float         batteryVoltage;
extern unsigned long lastBatteryCheck;

// ── API ───────────────────────────────────────────────────────────────────────
// Call from loop(). Samples ADC every 5 s, updates isCharging / isBatteryLow /
// isFullCharge, and triggers the appropriate sound alerts.
void checkBatteryStatus();
