#pragma once

#include "config.h"

// ── Battery Monitor State (defined in battery.cpp) ───────────────────────────
extern float         batteryVoltage;
extern unsigned long lastBatteryCheck;

// ── API ───────────────────────────────────────────────────────────────────────
// Call from loop(). Samples ADC every 5 s (8-sample averaged), updates
// isBatteryLow, and triggers the low-battery sound alert.
void checkBatteryStatus();
