#include <Arduino.h>
#include <LiquidCrystal.h>
#include <avr/wdt.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int lastButtonState = 5;  // Default: no button pressed

// ── Analog Button Reader ──────────────────────────────────────────────────────
int readLcdButtons() {
    int val = analogRead(A0);
    if (val > 1000) return 5;  // No button
    if (val < 50)   return 0;  // Right
    if (val < 195)  return 1;  // Up
    if (val < 380)  return 2;  // Down
    if (val < 550)  return 3;  // Left
    if (val < 790)  return 4;  // Select
    return 5;
}

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    // 2-second hardware watchdog — automatically resets if the sketch hangs
    wdt_enable(WDTO_2S);

    Serial.begin(115200);  // Communicate with ESP32 via logic-level shifter
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WHEEL-E REMOTE");
    lcd.setCursor(0, 1);
    lcd.print("Status: Ready");

    wdt_reset();
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    wdt_reset();  // Keep watchdog fed — must run within 2 s

    int currentButtonState = readLcdButtons();

    // Transmit only on edge transitions — keeps the serial bus clean
    if (currentButtonState != lastButtonState) {
        delay(30);  // Software debounce window
        currentButtonState = readLcdButtons();
        if (currentButtonState != lastButtonState) {
            lastButtonState = currentButtonState;
            lcd.setCursor(0, 1);
            switch (currentButtonState) {
                case 0: lcd.print("CMD: RIGHT      "); Serial.print('R'); break;
                case 1: lcd.print("CMD: FORWARD    "); Serial.print('F'); break;
                case 2: lcd.print("CMD: BACKWARD   "); Serial.print('B'); break;
                case 3: lcd.print("CMD: LEFT       "); Serial.print('L'); break;
                case 4:
                case 5: lcd.print("CMD: STOP       "); Serial.print('S'); break;
            }
        }
    }
}