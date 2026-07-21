#include "battery.h"
#include "sound.h"

// ── Battery Monitor State ─────────────────────────────────────────────────────
float         batteryVoltage  = -1.0f;  // -1 = not yet sampled; seeded on first read
unsigned long lastBatteryCheck = 0;

// ─────────────────────────────────────────────────────────────────────────────
void checkBatteryStatus() {
    unsigned long now = millis();
    if (now - lastBatteryCheck < 5000) return;
    lastBatteryCheck = now;

    // Average 8 ADC samples to reduce noise from the boost converter.
    // ESP32-C3 ADC: 11 dB attenuation set in setup() → full 0–3.3 V range.
    // The voltage divider on BATTERY_PIN (two equal resistors) halves the
    // battery voltage before the ADC pin, so multiply by VDIVIDER_RATIO (2).
    long sum = 0;
    for (int i = 0; i < 8; i++) {
        sum += analogRead(BATTERY_PIN);
        delayMicroseconds(200);
    }
    float measured = ((float)(sum / 8) / ADC_MAX_VAL) * VREF * VDIVIDER_RATIO;

    // Exponential moving average — smooths residual ADC noise.
    // On the very first sample seed directly from the real reading so the
    // display never shows a bogus 100% on cold start.
    if (batteryVoltage < 0.0f) {
        batteryVoltage = measured;
    } else {
        batteryVoltage = (batteryVoltage * 0.8f) + (measured * 0.2f);
    }

    // ── Low-battery alert ────────────────────────────────────────────────────
    if (batteryVoltage <= 3.45f) {
        if (!isBatteryLow) {
            isBatteryLow = true;
            triggerLowBatterySound();
        }
    } else {
        isBatteryLow = false;
    }
}
