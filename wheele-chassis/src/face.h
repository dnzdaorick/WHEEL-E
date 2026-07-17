#pragma once

#include "config.h"

// ── Blink Animation State (defined in face.cpp) ───────────────────────────────
extern bool          isBlinking;
extern unsigned long lastBlinkTime;
extern unsigned long blinkDuration;
extern unsigned long blinkInterval;
extern unsigned long connectedAnimStart;
extern const unsigned long connectedAnimDuration;

// ── API ───────────────────────────────────────────────────────────────────────
void displayStatusLine(const char* text);
void updateFaceAnimation();
