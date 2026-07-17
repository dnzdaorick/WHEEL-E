#include "battery.h"
#include "sound.h"

// ── Battery Monitor State ─────────────────────────────────────────────────────
float         batteryVoltage  = 4.2f;
unsigned long lastBatteryCheck = 0;

// ─────────────────────────────────────────────────────────────────────────────
void checkBatteryStatus() {
    unsigned long now = millis();
    if (now - lastBatteryCheck < 5000) return;
    lastBatteryCheck = now;

    // ESP32-C3 ADC: attenuation is set to ADC_11db in setup() so the full
    // 0–3.3 V range maps to 0–ADC_MAX_VAL (4095). The voltage divider on
    // BATTERY_PIN halves the battery voltage before the ADC pin.
    int rawAdc = analogRead(BATTERY_PIN);
    float measured = ((float)rawAdc / ADC_MAX_VAL) * VREF * VDIVIDER_RATIO;

    // Exponential moving average — smooths ADC noise without delay
    batteryVoltage = (batteryVoltage * 0.8f) + (measured * 0.2f);

    bool chargePinActive = (digitalRead(CHARGING_PIN) == LOW);

    // ── Charging detect edge ─────────────────────────────────────────────────
    if (chargePinActive && !isCharging) {
        isCharging = true;
        triggerChargingPluggedSound();
    } else if (!chargePinActive && isCharging) {
        isCharging  = false;
        isFullCharge = false;
    }

    // ── Charge level thresholds ──────────────────────────────────────────────
    if (isCharging) {
        if (batteryVoltage >= 4.12f) {
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
        if (batteryVoltage <= 3.45f) {
            if (!isBatteryLow) {
                isBatteryLow = true;
                triggerLowBatterySound();
            }
        } else {
            isBatteryLow = false;
        }
    }
}
