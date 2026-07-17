#include "face.h"
#include "sound.h"

// ── Blink Animation State ─────────────────────────────────────────────────────
bool          isBlinking      = false;
unsigned long lastBlinkTime   = 0;
unsigned long blinkDuration   = 180;
unsigned long blinkInterval   = 3500;
unsigned long connectedAnimStart = 0;
const unsigned long connectedAnimDuration = 2000;

// ─────────────────────────────────────────────────────────────────────────────
void displayStatusLine(const char* text) {
    display.clearDisplay();
    display.setCursor(0, 16);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.println("=== WHEEL-E ===");
    display.setCursor(0, 36);
    display.println(text);
    display.display();
}

// ── Face Rendering ────────────────────────────────────────────────────────────
void updateFaceAnimation() {
    display.clearDisplay();
    unsigned long now = millis();

    // Auto-blink when robot is looking forward or idle-resting
    if (currentFace == FACE_FORWARD || currentFace == FACE_LEFT ||
        currentFace == FACE_RIGHT   ||
        (currentFace == FACE_IDLE && currentIdleAct == ACT_RESTING)) {
        if (!isBlinking && (now - lastBlinkTime >= blinkInterval)) {
            isBlinking    = true;
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
            int pupilOffset = (int)(sin(now * 0.004f) * 6);
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
            int bounce = (int)(sin(now * 0.02f) * 2);
            display.fillTriangle(52, 38, 76, 38, 64, 48 + bounce, SSD1306_WHITE);
            break;
        }

        case FACE_DISCONNECTED: {
            display.drawRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
            display.drawRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);
            display.drawLine(18,  8, 46, 36, SSD1306_WHITE);
            display.drawLine(46,  8, 18, 36, SSD1306_WHITE);
            display.drawLine(82,  8, 110, 36, SSD1306_WHITE);
            display.drawLine(110, 8, 82,  36, SSD1306_WHITE);
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
                display.fillCircle(88,  26, 2, SSD1306_BLACK);
                display.drawLine(18, 2, 34,  1, SSD1306_WHITE);
                display.drawLine(34, 1, 46,  3, SSD1306_WHITE);
                display.drawLine(82, 3, 94,  1, SSD1306_WHITE);
                display.drawLine(94, 1, 110, 2, SSD1306_WHITE);
            }
            break;
        }

        case FACE_BACKWARD: {
            display.fillCircle(10, 42, 4, SSD1306_WHITE);
            display.fillCircle(118, 42, 4, SSD1306_WHITE);
            display.drawRoundRect(16, 6, 32, 32, 8, SSD1306_WHITE);
            display.drawRoundRect(80, 6, 32, 32, 8, SSD1306_WHITE);
            int shake = ((now / 40) % 2 == 0) ? 1 : -1;
            display.fillRoundRect(24 + shake, 18, 16, 16, 4, SSD1306_WHITE);
            display.fillRoundRect(88 + shake, 18, 16, 16, 4, SSD1306_WHITE);
            display.drawLine(48,  4, 48,  8, SSD1306_WHITE);
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
            int breath = (int)(sin(now * 0.003f) * 2) + 4;
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
            display.fillCircle(88,  26, 2, SSD1306_BLACK);
            display.drawRoundRect(48, 51, 32, 11, 2, SSD1306_WHITE);
            display.fillRect(80, 54, 2, 5, SSD1306_WHITE);
            display.fillRect(50, 53, 28, 7, SSD1306_WHITE);
            break;
        }

        case FACE_LOW_BATTERY: {
            int jitter = ((now / 30) % 2 == 0) ? 1 : -1;
            display.drawRoundRect(16 + jitter,  6, 32, 32, 8, SSD1306_WHITE);
            display.drawRoundRect(80 + jitter,  6, 32, 32, 8, SSD1306_WHITE);
            display.drawLine(24 + jitter, 14, 40 + jitter,  30, SSD1306_WHITE);
            display.drawLine(40 + jitter, 14, 24 + jitter,  30, SSD1306_WHITE);
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
            int singBounce = (int)(sin(now * 0.012f) * 2);
            display.drawCircle(32, 28 + singBounce, 12, SSD1306_WHITE);
            display.fillRect(16, 28 + singBounce, 32, 14, SSD1306_BLACK);
            display.drawCircle(96, 28 + singBounce, 12, SSD1306_WHITE);
            display.fillRect(80, 28 + singBounce, 32, 14, SSD1306_BLACK);
            int dynamicMouth = (activeFreqCount > 0) ? 14 : (int)(sin(now * 0.015f) * 3 + 8);
            display.fillCircle(64, 44, dynamicMouth, SSD1306_WHITE);
            for (int i = 0; i < 2; i++) {
                int noteShift = (int)(sin((now + (i * 2000)) * 0.008f) * 5);
                int noteX = 14 + (i * 96) + noteShift / 2;
                int noteY = 18 + noteShift;
                display.fillCircle(noteX, noteY, 2, SSD1306_WHITE);
                display.drawLine(noteX + 2, noteY,     noteX + 2, noteY - 6, SSD1306_WHITE);
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
                        display.fillCircle(88,  26, 2, SSD1306_BLACK);
                    }
                    break;
                }

                case ACT_SINGING: {
                    display.drawCircle(32, 28, 12, SSD1306_WHITE);
                    display.fillRect(16, 28, 32, 14, SSD1306_BLACK);
                    display.drawCircle(96, 28, 12, SSD1306_WHITE);
                    display.fillRect(80, 28, 32, 14, SSD1306_BLACK);
                    int singingMouth = (int)(sin(now * 0.015f) * 3 + 7);
                    display.fillCircle(64, 44, singingMouth, SSD1306_WHITE);
                    int noteOsc = (int)(sin(now * 0.01f) * 4);
                    display.fillCircle(54, 16 + noteOsc, 3, SSD1306_WHITE);
                    display.drawLine(56, 16 + noteOsc, 56, 8 + noteOsc, SSD1306_WHITE);
                    display.drawLine(56,  8 + noteOsc, 60, 10 + noteOsc, SSD1306_WHITE);
                    break;
                }

                case ACT_GIGGLING: {
                    int giggleShake = ((now / 75) % 2 == 0) ? 2 : -2;
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
                    int lookDir = (idleActElapsedTime < 2000) ? -1 : 1;
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
                    float rotAngle = now * 0.01f;
                    int dx1 = (int)(cos(rotAngle) * 8);
                    int dy1 = (int)(sin(rotAngle) * 8);
                    int dx2 = (int)(cos(rotAngle + 1.5708f) * 8);
                    int dy2 = (int)(sin(rotAngle + 1.5708f) * 8);
                    display.drawLine(32 - dx1, 22 - dy1, 32 + dx1, 22 + dy1, SSD1306_WHITE);
                    display.drawLine(32 - dx2, 22 - dy2, 32 + dx2, 22 + dy2, SSD1306_WHITE);
                    display.drawLine(96 - dx1, 22 - dy1, 96 + dx1, 22 + dy1, SSD1306_WHITE);
                    display.drawLine(96 - dx2, 22 - dy2, 96 + dx2, 22 + dy2, SSD1306_WHITE);
                    int starX = (int)(64 + cos(now * 0.005f) * 20);
                    int starY = (int)(12 + sin(now * 0.005f) * 4);
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
                    int crossX = 112; int crossY = 10;
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
                        int squeeze = (int)(elapsed / 250);
                        display.fillRoundRect(16, 6 + squeeze / 2, 32, 32 - squeeze, 8, SSD1306_WHITE);
                        display.fillRoundRect(80, 6 + squeeze / 2, 32, 32 - squeeze, 8, SSD1306_WHITE);
                        int sNose = ((now / 30) % 2 == 0) ? 1 : -1;
                        display.fillCircle(32 + sNose, 22, 3, SSD1306_BLACK);
                        display.fillCircle(96 + sNose, 22, 3, SSD1306_BLACK);
                        display.drawCircle(64, 44, 3 + squeeze / 3, SSD1306_WHITE);
                    } else {
                        display.drawLine(14, 22, 30, 14, SSD1306_WHITE);
                        display.drawLine(30, 14, 46, 22, SSD1306_WHITE);
                        display.drawLine(14, 23, 30, 15, SSD1306_WHITE);
                        display.drawLine(30, 15, 46, 23, SSD1306_WHITE);
                        display.drawLine(82, 22,  98, 14, SSD1306_WHITE);
                        display.drawLine(98, 14, 114, 22, SSD1306_WHITE);
                        display.drawLine(82, 23,  98, 15, SSD1306_WHITE);
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
                    int yawnOsc = (int)(sin(now * 0.008f) * 4) + 8;
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
