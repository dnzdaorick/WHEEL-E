#include "face.h"
#include "sound.h"
#include "battery.h"

// ── Blink Animation State ─────────────────────────────────────────────────────
bool          isBlinking      = false;
unsigned long lastBlinkTime   = 0;
unsigned long blinkDuration   = 180;
unsigned long blinkInterval   = 3500;
unsigned long connectedAnimStart = 0;
const unsigned long connectedAnimDuration = 2000;

// ── Particle System for Expressive Animations ─────────────────────────────────
struct Particle {
    float x, y;
    float vx, vy;
    int type; // 0 = dot/sweat, 1 = music note, 2 = Sleeping Z, 3 = tear, 4 = steam, 5 = sneeze splash, 6 = spark/plus
    float size;
    int lifetime;
    int maxLifetime;
};

const int MAX_PARTICLES = 16;
static Particle particles[MAX_PARTICLES];
static bool particlesInitialized = false;
static float smoothPupilOffset = 0.0f;
static unsigned long lastVisualNoteOnTime = 0;

static char lastProcessedCmd[16] = "STOP";
static float swipeDx = 0.0f;
static float swipeDy = 0.0f;
static unsigned long swipeTime = 0;
static unsigned long lastTapTime = 0;

static void getPadCoords(const char* padName, int& px, int& py) {
    if (strcmp(padName, "NW") == 0)            { px = 0; py = 0; }
    else if (strcmp(padName, "FORWARD") == 0)  { px = 1; py = 0; }
    else if (strcmp(padName, "NE") == 0)       { px = 2; py = 0; }
    else if (strcmp(padName, "LEFT") == 0)     { px = 0; py = 1; }
    else if (strcmp(padName, "CENTER") == 0)   { px = 1; py = 1; }
    else if (strcmp(padName, "RIGHT") == 0)    { px = 2; py = 1; }
    else if (strcmp(padName, "SW") == 0)       { px = 0; py = 2; }
    else if (strcmp(padName, "BACKWARD") == 0) { px = 1; py = 2; }
    else if (strcmp(padName, "SE") == 0)       { px = 2; py = 2; }
    else { px = -1; py = -1; }
}



static void initParticles() {
    if (particlesInitialized) return;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].lifetime = 0;
    }
    particlesInitialized = true;
}

static void spawnParticle(float x, float y, float vx, float vy, int type, float size, int lifetime) {
    initParticles();
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].lifetime <= 0) {
            particles[i].x = x;
            particles[i].y = y;
            particles[i].vx = vx;
            particles[i].vy = vy;
            particles[i].type = type;
            particles[i].size = size;
            particles[i].lifetime = lifetime;
            particles[i].maxLifetime = lifetime;
            break;
        }
    }
}

static void updateAndDrawParticles(int yOffset) {
    initParticles();
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].lifetime > 0) {
            // Apply physics & custom behaviors
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;

            if (particles[i].type == 3) { // Tear drops
                particles[i].vy += 0.08f; // gravity
                if (particles[i].y >= 60) {
                    particles[i].y = 60;
                    particles[i].lifetime = 0; // kill teardrop
                    // spawn splash dots
                    for (int j = 0; j < 3; j++) {
                        spawnParticle(particles[i].x, 60, ((float)random(-15, 16)) / 10.0f, -((float)random(10, 20)) / 10.0f, 5, 1, 8);
                    }
                }
            } else if (particles[i].type == 1) { // Music note wobble
                particles[i].vx = sin(millis() * 0.006f + i) * 0.4f;
            } else if (particles[i].type == 2) { // Z wave drift
                particles[i].vx = sin(millis() * 0.004f + i) * 0.3f;
            } else if (particles[i].type == 4) { // Steam cloud drift
                particles[i].vx += ((float)random(-5, 6)) / 100.0f;
            }

            int px = (int)particles[i].x;
            int py = (int)particles[i].y;

            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                switch (particles[i].type) {
                    case 0:
                    case 5:
                        display.drawPixel(px, py, SSD1306_WHITE);
                        break;
                    case 1: { // Note
                        int r = (int)(particles[i].size);
                        if (r < 1) r = 1;
                        int stemHeight = r * 3;
                        display.fillCircle(px, py, r, SSD1306_WHITE);
                        display.drawLine(px + r, py, px + r, py - stemHeight, SSD1306_WHITE);
                        display.drawLine(px + r, py - stemHeight, px + r + r * 2, py - stemHeight + r, SSD1306_WHITE);
                        break;
                    }
                    case 2: { // Z
                        int sz = (int)particles[i].size;
                        if (sz < 2) sz = 2;
                        display.drawLine(px, py, px + sz, py, SSD1306_WHITE);
                        display.drawLine(px + sz, py, px, py + sz, SSD1306_WHITE);
                        display.drawLine(px, py + sz, px + sz, py + sz, SSD1306_WHITE);
                        break;
                    }
                    case 3: { // Tear
                        display.fillTriangle(px, py - 2, px - 1, py + 1, px + 1, py + 1, SSD1306_WHITE);
                        display.fillCircle(px, py + 1, 1, SSD1306_WHITE);
                        break;
                    }
                    case 4: { // Steam
                        display.drawCircle(px, py, 2, SSD1306_WHITE);
                        break;
                    }
                    case 6: { // Spark/Plus
                        display.drawLine(px - 2, py, px + 2, py, SSD1306_WHITE);
                        display.drawLine(px, py - 2, px, py + 2, SSD1306_WHITE);
                        break;
                    }
                }
            }
            particles[i].lifetime--;
        }
    }
}

// ── Animation Helpers ─────────────────────────────────────────────────────────
static float getBlinkProgress(unsigned long now) {
    if (!isBlinking) return 0.0f;
    unsigned long elapsed = now - lastBlinkTime;
    if (elapsed >= blinkDuration) return 0.0f;
    float phase = (float)elapsed / blinkDuration;
    return sin(phase * 3.14159265f);
}

static void drawEye(int eyeX, int eyeY, int eyeHeight) {
    int r = eyeHeight / 4;
    if (r < 2) r = 2;
    if (r > 8) r = 8;
    display.fillRoundRect(eyeX, eyeY, 32, eyeHeight, r, SSD1306_WHITE);
    if (eyeHeight > 8) {
        int pupil1Y = eyeY + (int)(8.0f * ((float)eyeHeight / 32.0f));
        int pupil2Y = eyeY + (int)(20.0f * ((float)eyeHeight / 32.0f));
        int r1 = (int)(4.0f * ((float)eyeHeight / 32.0f));
        int r2 = (int)(2.0f * ((float)eyeHeight / 32.0f));
        if (r1 < 1) r1 = 1;
        
        display.fillCircle(eyeX + 22, pupil1Y, r1, SSD1306_BLACK);
        if (r2 >= 1) {
            display.fillCircle(eyeX + 8, pupil2Y, r2, SSD1306_BLACK);
        }
    }
}

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

    // Breathing offset calculation
    int yOffset = 0;
    if (currentFace == FACE_IDLE || currentFace == FACE_FORWARD || 
        currentFace == FACE_LEFT || currentFace == FACE_RIGHT ||
        currentFace == FACE_MUSIC_MODE || currentFace == FACE_CONNECTED) {
        yOffset = (int)(sin(now * 0.002f) * 1.5f);
    }

    switch (currentFace) {

        case FACE_BOOTING: {
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            
            // Title
            display.setCursor(16, 2);
            display.println("WHEEL-E SYSTEM BOOT");
            display.drawLine(16, 12, 112, 12, SSD1306_WHITE);
            
            // Spinner
            const char spinner[] = {'|', '/', '-', '\\'};
            char spinChar = spinner[(now / 100) % 4];
            
            // Checklist
            display.setCursor(16, 18);
            if (now < 800) {
                display.printf("BOOTING %c", spinChar);
            } else {
                display.println("SYSTEM OK  [PASS]");
                display.setCursor(16, 27);
                if (now < 1500) {
                    display.printf("WiFi AP  %c", spinChar);
                } else {
                    display.println("WiFi AP  [192.168.67.1]");
                    display.setCursor(16, 36);
                    if (now < 2200) {
                        display.printf("I2C BUS  %c", spinChar);
                    } else {
                        display.println("I2C BUS  [400 kHz]");
                        display.setCursor(16, 45);
                        display.println("BATTERY  [READY]");
                    }
                }
            }
            
            // Progress Bar
            int progressWidth = min(120, (int)(now * 120 / 3000));
            display.drawRect(4, 54, 120, 6, SSD1306_WHITE);
            if (progressWidth > 4) {
                display.fillRect(6, 56, progressWidth - 4, 2, SSD1306_WHITE);
            }
            break;
        }

        case FACE_CONNECTING: {
            display.drawRoundRect(16, 6 + yOffset, 32, 32, 8, SSD1306_WHITE);
            display.drawRoundRect(80, 6 + yOffset, 32, 32, 8, SSD1306_WHITE);
            int pupilOffset = (int)(sin(now * 0.004f) * 6);
            display.fillRoundRect(24 + pupilOffset, 12 + yOffset, 16, 20, 4, SSD1306_WHITE);
            display.fillRoundRect(88 + pupilOffset, 12 + yOffset, 16, 20, 4, SSD1306_WHITE);
            display.drawCircle(64, 44 + yOffset, 4, SSD1306_WHITE);
            
            // WiFi Antenna animation
            int antennaTimer = (now / 250) % 4;
            display.fillCircle(64, 26 + yOffset, 1, SSD1306_WHITE);
            if (antennaTimer >= 1) {
                display.drawLine(61, 22 + yOffset, 67, 22 + yOffset, SSD1306_WHITE);
            }
            if (antennaTimer >= 2) {
                display.drawLine(58, 18 + yOffset, 70, 18 + yOffset, SSD1306_WHITE);
            }
            if (antennaTimer >= 3) {
                display.drawLine(55, 14 + yOffset, 73, 14 + yOffset, SSD1306_WHITE);
            }
            break;
        }

        case FACE_CONNECTED: {
            display.fillRoundRect(16, 12 + yOffset, 32, 20, 6, SSD1306_WHITE);
            display.fillRoundRect(80, 12 + yOffset, 32, 20, 6, SSD1306_WHITE);
            display.drawLine(14, 22 + yOffset, 50, 22 + yOffset, SSD1306_BLACK);
            display.drawLine(78, 22 + yOffset, 114, 22 + yOffset, SSD1306_BLACK);
            
            // Pulsing cheeks
            int blushRadius = 4 + (int)(sin(now * 0.01f) * 1.5f);
            display.fillCircle(10, 42 + yOffset, blushRadius, SSD1306_WHITE);
            display.fillCircle(118, 42 + yOffset, blushRadius, SSD1306_WHITE);
            
            int bounce = (int)(sin(now * 0.02f) * 2);
            display.fillTriangle(52, 38 + yOffset, 76, 38 + yOffset, 64, 48 + bounce + yOffset, SSD1306_WHITE);
            
            // Spawn float sparkles
            if (random(0, 15) == 0) {
                spawnParticle(64 + random(-20, 21), 40 + yOffset, ((float)random(-5, 6)) / 10.0f, -0.6f - ((float)random(0, 10)) / 20.0f, 6, 2, 25);
            }
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
            
            // Falling teardrops
            if (random(0, 15) == 0) {
                if (random(0, 2) == 0) {
                    spawnParticle(16 + random(4, 28), 24, 0, 0.4f, 3, 2, 70);
                } else {
                    spawnParticle(80 + random(4, 28), 24, 0, 0.4f, 3, 2, 70);
                }
            }
            break;
        }

        case FACE_FORWARD: {
            display.fillCircle(10, 42 + yOffset, 4, SSD1306_WHITE);
            display.fillCircle(118, 42 + yOffset, 4, SSD1306_WHITE);
            display.fillRoundRect(50, 36 + yOffset, 28, 16, 6, SSD1306_WHITE);
            
            float blinkProgress = getBlinkProgress(now);
            int eyeHeight = (int)(32.0f - (30.0f * blinkProgress));
            int eyeY = 6 + yOffset + (32 - eyeHeight) / 2;
            drawEye(16, eyeY, eyeHeight);
            drawEye(80, eyeY, eyeHeight);
            
            display.drawLine(18, 2 + yOffset, 34, 1 + yOffset, SSD1306_WHITE);
            display.drawLine(34, 1 + yOffset, 46, 3 + yOffset, SSD1306_WHITE);
            display.drawLine(82, 3 + yOffset, 94, 1 + yOffset, SSD1306_WHITE);
            display.drawLine(94, 1 + yOffset, 110, 2 + yOffset, SSD1306_WHITE);
            break;
        }

        case FACE_BACKWARD: {
            display.fillCircle(10, 42 + yOffset, 4, SSD1306_WHITE);
            display.fillCircle(118, 42 + yOffset, 4, SSD1306_WHITE);
            display.drawRoundRect(16, 6 + yOffset, 32, 32, 8, SSD1306_WHITE);
            display.drawRoundRect(80, 6 + yOffset, 32, 32, 8, SSD1306_WHITE);
            int shake = ((now / 40) % 2 == 0) ? 1 : -1;
            display.fillRoundRect(24 + shake, 18 + yOffset, 16, 16, 4, SSD1306_WHITE);
            display.fillRoundRect(88 + shake, 18 + yOffset, 16, 16, 4, SSD1306_WHITE);
            display.drawLine(48,  4 + yOffset, 48,  8 + yOffset, SSD1306_WHITE);
            display.drawLine(112, 4 + yOffset, 112, 8 + yOffset, SSD1306_WHITE);
            display.drawRoundRect(52, 40 + yOffset, 24, 14, 6, SSD1306_WHITE);
            break;
        }

        case FACE_LEFT: {
            display.fillCircle(10, 42 + yOffset, 4, SSD1306_WHITE);
            display.fillCircle(118, 42 + yOffset, 4, SSD1306_WHITE);
            display.drawLine(52, 43 + yOffset, 56, 46 + yOffset, SSD1306_WHITE);
            display.drawLine(56, 46 + yOffset, 60, 43 + yOffset, SSD1306_WHITE);
            display.drawLine(60, 43 + yOffset, 64, 46 + yOffset, SSD1306_WHITE);
            display.drawLine(64, 46 + yOffset, 68, 43 + yOffset, SSD1306_WHITE);
            
            float blinkProgress = getBlinkProgress(now);
            int eyeHeight = (int)(32.0f - (30.0f * blinkProgress));
            int eyeY = 6 + yOffset + (32 - eyeHeight) / 2;
            
            if (eyeHeight <= 8) {
                display.fillRoundRect(16, 18 + yOffset, 32, eyeHeight, 4, SSD1306_WHITE);
                display.fillRoundRect(80, 18 + yOffset, 32, eyeHeight, 4, SSD1306_WHITE);
            } else {
                display.drawRoundRect(16, eyeY, 32, eyeHeight, 8, SSD1306_WHITE);
                display.drawRoundRect(80, eyeY, 32, eyeHeight, 8, SSD1306_WHITE);
                
                int pupilH = (int)(24.0f * ((float)eyeHeight / 32.0f));
                int pupilY = eyeY + (eyeHeight - pupilH) / 2;
                display.fillRoundRect(19, pupilY, 18, pupilH, 6, SSD1306_WHITE);
                display.fillRoundRect(83, pupilY, 18, pupilH, 6, SSD1306_WHITE);
            }
            break;
        }

        case FACE_RIGHT: {
            display.fillCircle(10, 42 + yOffset, 4, SSD1306_WHITE);
            display.fillCircle(118, 42 + yOffset, 4, SSD1306_WHITE);
            display.drawLine(60, 43 + yOffset, 64, 46 + yOffset, SSD1306_WHITE);
            display.drawLine(64, 46 + yOffset, 68, 43 + yOffset, SSD1306_WHITE);
            display.drawLine(68, 43 + yOffset, 72, 46 + yOffset, SSD1306_WHITE);
            display.drawLine(72, 46 + yOffset, 76, 43 + yOffset, SSD1306_WHITE);
            
            float blinkProgress = getBlinkProgress(now);
            int eyeHeight = (int)(32.0f - (30.0f * blinkProgress));
            int eyeY = 6 + yOffset + (32 - eyeHeight) / 2;
            
            if (eyeHeight <= 8) {
                display.fillRoundRect(16, 18 + yOffset, 32, eyeHeight, 4, SSD1306_WHITE);
                display.fillRoundRect(80, 18 + yOffset, 32, eyeHeight, 4, SSD1306_WHITE);
            } else {
                display.drawRoundRect(16, eyeY, 32, eyeHeight, 8, SSD1306_WHITE);
                display.drawRoundRect(80, eyeY, 32, eyeHeight, 8, SSD1306_WHITE);
                
                int pupilH = (int)(24.0f * ((float)eyeHeight / 32.0f));
                int pupilY = eyeY + (eyeHeight - pupilH) / 2;
                display.fillRoundRect(27, pupilY, 18, pupilH, 6, SSD1306_WHITE);
                display.fillRoundRect(91, pupilY, 18, pupilH, 6, SSD1306_WHITE);
            }
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
            
            // Flashing Warning Exclamation Triangle
            if ((now / 400) % 2 == 0) {
                display.drawTriangle(64, 12, 54, 30, 74, 30, SSD1306_WHITE);
                display.drawLine(64, 17, 64, 23, SSD1306_WHITE);
                display.drawPixel(64, 26, SSD1306_WHITE);
            }
            break;
        }

        case FACE_MUSIC_MODE: {
            int cheekRadius = 4; // Constant size dimples, no pulsing effect
            int eyeRadius = 12;

            if (activeFreqCount > 0) {
                eyeRadius = 15;
            } else {
                eyeRadius = 12;
            }

            // --- Detect Swipe vs Tap vs Hold ---
            bool commandChanged = (strcmp(currentCmd, lastProcessedCmd) != 0);
            
            if (commandChanged) {
                if (strcmp(currentCmd, "STOP") != 0) {
                    if (strcmp(lastProcessedCmd, "STOP") == 0) {
                        // TAP!
                        lastTapTime = now;
                        // Spawn one clear big note floating up from mouth
                        spawnParticle(64, 44 + yOffset, 
                                      ((float)random(-5, 6)) / 10.0f, 
                                      -1.6f, 
                                      1,    // music note type
                                      3.5f,  // larger size
                                      50);  // lifetime
                        // Helper notes
                        for (int i = 0; i < 2; i++) {
                            spawnParticle(64, 44 + yOffset, 
                                          ((float)random(-15, 16)) / 10.0f, 
                                          -1.2f - ((float)random(0, 10)) / 10.0f, 
                                          1, 
                                          2.0f, 
                                          40 + random(0, 15));
                        }
                    } else {
                        // SWIPE!
                        int x1, y1, x2, y2;
                        getPadCoords(lastProcessedCmd, x1, y1);
                        getPadCoords(currentCmd, x2, y2);
                        if (x1 != -1 && x2 != -1) {
                            swipeDx = (float)(x2 - x1);
                            swipeDy = (float)(y2 - y1);
                            swipeTime = now;
                            
                            // Spawn note burst in the swipe direction
                            float vx = swipeDx * 2.5f;
                            float vy = swipeDy * 2.0f - 1.2f;
                            for (int i = 0; i < 4; i++) {
                                spawnParticle(64, 44 + yOffset, 
                                              vx + ((float)random(-15, 16)) / 10.0f, 
                                              vy + ((float)random(-10, 11)) / 10.0f, 
                                              1, 
                                              2.0f, 
                                              35 + random(0, 15));
                            }
                        }
                    }
                }
                strncpy(lastProcessedCmd, currentCmd, sizeof(lastProcessedCmd) - 1);
                lastProcessedCmd[sizeof(lastProcessedCmd) - 1] = '\0';
            }

            // Sync with musicNoteOnTime if external note-start events happen
            if (activeFreqCount > 0 && musicNoteOnTime != lastVisualNoteOnTime) {
                lastVisualNoteOnTime = musicNoteOnTime;
                // If we didn't capture it via commandChanged (e.g. if parsed notes started), trigger tap/swipe logic manually
                if (now - lastTapTime > 50 && now - swipeTime > 50) {
                    // Treat as tap
                    lastTapTime = now;
                    spawnParticle(64, 44 + yOffset, 
                                  ((float)random(-5, 6)) / 10.0f, 
                                  -1.6f, 
                                  1, 3.5f, 50);
                }
            }

            // --- Hold Detection & Continuous Particle Spawning ---
            bool isHolding = (activeFreqCount > 0 && (now - musicNoteOnTime > 300));
            
            if (isHolding) {
                // Continuous small notes streaming from the mouth while holding
                if (random(0, 8) == 0) {
                    spawnParticle(64, 44 + yOffset, 
                                  ((float)random(-12, 13)) / 10.0f, 
                                  -1.0f - ((float)random(5, 15)) / 10.0f, 
                                  1, 
                                  1.8f, 
                                  45);
                }
            } else {
                // Gentle idle floating notes spawn when not playing/holding
                if (activeFreqCount == 0 && random(0, 15) == 0) {
                    spawnParticle(64 + random(-10, 11), 40 + yOffset, ((float)random(-10, 11)) / 10.0f, -0.8f, 1, 2.0f, 45);
                }
            }

            // --- Jitter & Elastic Slide (Swipe Offset) Offset Computations ---
            int holdJitterX = isHolding ? random(-1, 2) : 0;
            int holdJitterY = isHolding ? random(-1, 2) : 0;

            float swipeProgress = 0.0f;
            if (now - swipeTime < 300) {
                swipeProgress = 1.0f - ((float)(now - swipeTime) / 300.0f);
            }
            int swipeXOffset = (int)(swipeDx * 8.0f * swipeProgress);
            int swipeYOffset = (int)(swipeDy * 4.0f * swipeProgress);

            // Shifting elements with combined offsets
            // Cheeks slide with a smaller factor for a pseudo-3D head turn effect
            display.fillCircle(10 + (int)(swipeXOffset * 0.3f), 42 + yOffset + (int)(swipeYOffset * 0.3f), cheekRadius, SSD1306_WHITE);
            display.fillCircle(118 + (int)(swipeXOffset * 0.3f), 42 + yOffset + (int)(swipeYOffset * 0.3f), cheekRadius, SSD1306_WHITE);
            
            int singBounce = (int)(sin(now * 0.02f) * 3);
            
            int leX = 32 + swipeXOffset + holdJitterX;
            int reX = 96 + swipeXOffset + holdJitterX;
            int eyeY = 28 + singBounce + yOffset + swipeYOffset + holdJitterY;

            // Left eye (arched happy closed eye shape)
            display.drawCircle(leX, eyeY, eyeRadius, SSD1306_WHITE);
            display.fillRect(leX - eyeRadius, eyeY, eyeRadius * 2, eyeRadius + 2, SSD1306_BLACK);

            // Right eye (arched happy closed eye shape)
            display.drawCircle(reX, eyeY, eyeRadius, SSD1306_WHITE);
            display.fillRect(reX - eyeRadius, eyeY, eyeRadius * 2, eyeRadius + 2, SSD1306_BLACK);
            
            // Vibrating mouth size when holding a note
            int dynamicMouth = (activeFreqCount > 0) ? (14 + (isHolding ? (int)(sin(now * 0.08f) * 2.0f) : 0)) : (int)(sin(now * 0.015f) * 3 + 8);
            display.fillCircle(64 + swipeXOffset + holdJitterX, 44 + yOffset + swipeYOffset + holdJitterY, dynamicMouth, SSD1306_WHITE);
            
            break;
        }

        case FACE_IDLE: {
            display.fillCircle(10, 42 + yOffset, 4, SSD1306_WHITE);
            display.fillCircle(118, 42 + yOffset, 4, SSD1306_WHITE);

            switch (currentIdleAct) {

                case ACT_RESTING: {
                    display.drawPixel(64, 45 + yOffset, SSD1306_WHITE);
                    display.drawLine(56, 42 + yOffset, 60, 45 + yOffset, SSD1306_WHITE);
                    display.drawLine(60, 45 + yOffset, 64, 42 + yOffset, SSD1306_WHITE);
                    display.drawLine(64, 42 + yOffset, 68, 45 + yOffset, SSD1306_WHITE);
                    display.drawLine(68, 45 + yOffset, 72, 42 + yOffset, SSD1306_WHITE);
                    
                    float blinkProgress = getBlinkProgress(now);
                    int eyeHeight = (int)(32.0f - (30.0f * blinkProgress));
                    int eyeY = 6 + yOffset + (32 - eyeHeight) / 2;
                    drawEye(16, eyeY, eyeHeight);
                    drawEye(80, eyeY, eyeHeight);
                    break;
                }

                case ACT_SINGING: {
                    display.drawCircle(32, 28 + yOffset, 12, SSD1306_WHITE);
                    display.fillRect(16, 28 + yOffset, 32, 14, SSD1306_BLACK);
                    display.drawCircle(96, 28 + yOffset, 12, SSD1306_WHITE);
                    display.fillRect(80, 28 + yOffset, 32, 14, SSD1306_BLACK);
                    
                    int singingMouth = (int)(sin(now * 0.015f) * 3 + 7);
                    display.fillCircle(64, 44 + yOffset, singingMouth, SSD1306_WHITE);
                    
                    // Spawn musical notes
                    if (random(0, 10) == 0) {
                        spawnParticle(64 + random(-6, 7), 40 + yOffset, ((float)random(-10, 11)) / 10.0f, -0.7f, 1, 2, 45);
                    }
                    break;
                }

                case ACT_GIGGLING: {
                    int giggleShake = ((now / 75) % 2 == 0) ? 2 : -2;
                    // Happy curved eyes ^ ^
                    display.drawLine(16, 24 + giggleShake + yOffset, 32, 16 + giggleShake + yOffset, SSD1306_WHITE);
                    display.drawLine(32, 16 + giggleShake + yOffset, 48, 24 + giggleShake + yOffset, SSD1306_WHITE);
                    display.drawLine(80, 24 + giggleShake + yOffset, 96, 16 + giggleShake + yOffset, SSD1306_WHITE);
                    display.drawLine(96, 16 + giggleShake + yOffset, 112, 24 + giggleShake + yOffset, SSD1306_WHITE);
                    
                    if ((now / 150) % 2 == 0) {
                        display.fillCircle(64, 44 + giggleShake + yOffset, 6, SSD1306_WHITE);
                    } else {
                        display.drawLine(58, 44 + giggleShake + yOffset, 70, 44 + giggleShake + yOffset, SSD1306_WHITE);
                    }
                    
                    // Sweat laugh marks
                    if (random(0, 10) == 0) {
                        spawnParticle(48 + random(0, 32), 12 + yOffset, ((float)random(-8, 9)) / 10.0f, -0.5f, 0, 1, 15);
                    }
                    break;
                }

                case ACT_LOOKING: {
                    display.drawRoundRect(16, 6 + yOffset, 32, 32, 8, SSD1306_WHITE);
                    display.drawRoundRect(80, 6 + yOffset, 32, 32, 8, SSD1306_WHITE);
                    
                    int lookDir = (idleActElapsedTime < 2000) ? -1 : 1;
                    float targetOffset = lookDir * 6.0f;
                    smoothPupilOffset = (smoothPupilOffset * 0.85f) + (targetOffset * 0.15f);
                    
                    int pupilX1 = 32 + (int)smoothPupilOffset;
                    int pupilX2 = 96 + (int)smoothPupilOffset;
                    display.fillRoundRect(pupilX1 - 8, 12 + yOffset, 16, 20, 4, SSD1306_WHITE);
                    display.fillRoundRect(pupilX2 - 8, 12 + yOffset, 16, 20, 4, SSD1306_WHITE);
                    display.fillCircle(pupilX1 + 4, 16 + yOffset, 2, SSD1306_BLACK);
                    display.fillCircle(pupilX2 + 4, 16 + yOffset, 2, SSD1306_BLACK);
                    
                    int shift = (int)(smoothPupilOffset / 3.0f);
                    display.drawPixel(64 + shift, 45 + yOffset, SSD1306_WHITE);
                    display.drawLine(56 + shift, 42 + yOffset, 60 + shift, 45 + yOffset, SSD1306_WHITE);
                    display.drawLine(60 + shift, 45 + yOffset, 64 + shift, 42 + yOffset, SSD1306_WHITE);
                    display.drawLine(64 + shift, 42 + yOffset, 68 + shift, 45 + yOffset, SSD1306_WHITE);
                    display.drawLine(68 + shift, 45 + yOffset, 72 + shift, 42 + yOffset, SSD1306_WHITE);
                    break;
                }

                case ACT_DIZZY: {
                    display.drawRoundRect(16, 6 + yOffset, 32, 32, 8, SSD1306_WHITE);
                    display.drawRoundRect(80, 6 + yOffset, 32, 32, 8, SSD1306_WHITE);
                    float rotAngle = now * 0.01f;
                    int dx1 = (int)(cos(rotAngle) * 8);
                    int dy1 = (int)(sin(rotAngle) * 8);
                    int dx2 = (int)(cos(rotAngle + 1.5708f) * 8);
                    int dy2 = (int)(sin(rotAngle + 1.5708f) * 8);
                    display.drawLine(32 - dx1, 22 - dy1 + yOffset, 32 + dx1, 22 + dy1 + yOffset, SSD1306_WHITE);
                    display.drawLine(32 - dx2, 22 - dy2 + yOffset, 32 + dx2, 22 + dy2 + yOffset, SSD1306_WHITE);
                    display.drawLine(96 - dx1, 22 - dy1 + yOffset, 96 + dx1, 22 + dy1 + yOffset, SSD1306_WHITE);
                    display.drawLine(96 - dx2, 22 - dy2 + yOffset, 96 + dx2, 22 + dy2 + yOffset, SSD1306_WHITE);
                    
                    display.drawLine(54, 43 + yOffset, 58, 46 + yOffset, SSD1306_WHITE);
                    display.drawLine(58, 46 + yOffset, 62, 43 + yOffset, SSD1306_WHITE);
                    display.drawLine(62, 43 + yOffset, 66, 46 + yOffset, SSD1306_WHITE);
                    display.drawLine(66, 46 + yOffset, 70, 43 + yOffset, SSD1306_WHITE);
                    
                    // Orbiting dizzy stars (3 stars)
                    for (int s = 0; s < 3; s++) {
                        float angle = now * 0.005f + s * 2.094f;
                        int starX = (int)(64 + cos(angle) * 22);
                        int starY = (int)(10 + sin(angle) * 5 + yOffset);
                        display.drawLine(starX - 2, starY, starX + 2, starY, SSD1306_WHITE);
                        display.drawLine(starX, starY - 2, starX, starY + 2, SSD1306_WHITE);
                    }
                    break;
                }

                case ACT_POUTING: {
                    display.fillRoundRect(16, 10 + yOffset, 32, 28, 8, SSD1306_WHITE);
                    display.fillRoundRect(80, 10 + yOffset, 32, 28, 8, SSD1306_WHITE);
                    display.fillCircle(32, 24 + yOffset, 4, SSD1306_BLACK);
                    display.fillCircle(96, 24 + yOffset, 4, SSD1306_BLACK);
                    display.drawLine(14, 4 + yOffset, 46, 12 + yOffset, SSD1306_WHITE);
                    display.drawLine(14, 5 + yOffset, 46, 13 + yOffset, SSD1306_WHITE);
                    display.drawLine(114, 4 + yOffset, 82, 12 + yOffset, SSD1306_WHITE);
                    display.drawLine(114, 5 + yOffset, 82, 13 + yOffset, SSD1306_WHITE);
                    display.drawLine(56, 46 + yOffset, 72, 46 + yOffset, SSD1306_WHITE);
                    display.drawLine(56, 46 + yOffset, 54, 49 + yOffset, SSD1306_WHITE);
                    display.drawLine(72, 46 + yOffset, 74, 49 + yOffset, SSD1306_WHITE);
                    
                    // Pulsing anger cross
                    float pulseFactor = 1.0f + sin(now * 0.015f) * 0.25f;
                    int markSz = (int)(4.0f * pulseFactor);
                    int crossX = 112; int crossY = 12 + yOffset;
                    display.drawLine(crossX - markSz, crossY, crossX + markSz, crossY, SSD1306_WHITE);
                    display.drawLine(crossX, crossY - markSz, crossX, crossY + markSz, SSD1306_WHITE);
                    
                    // Steam particles
                    if (random(0, 15) == 0) {
                        spawnParticle(crossX, crossY + 4, ((float)random(2, 6)) / 10.0f, -0.4f, 4, 2, 20);
                    }
                    break;
                }

                case ACT_SIGHING: {
                    display.fillRoundRect(16, 6 + yOffset, 32, 32, 8, SSD1306_WHITE);
                    display.fillRoundRect(80, 6 + yOffset, 32, 32, 8, SSD1306_WHITE);
                    display.fillRect(16, 6 + yOffset, 32, 12, SSD1306_BLACK);
                    display.fillRect(80, 6 + yOffset, 32, 12, SSD1306_BLACK);
                    display.drawRoundRect(16, 6 + yOffset, 32, 32, 8, SSD1306_WHITE);
                    display.drawRoundRect(80, 6 + yOffset, 32, 32, 8, SSD1306_WHITE);
                    display.drawLine(58, 45 + yOffset, 70, 45 + yOffset, SSD1306_WHITE);
                    display.drawLine(58, 45 + yOffset, 56, 47 + yOffset, SSD1306_WHITE);
                    display.drawLine(70, 45 + yOffset, 72, 47 + yOffset, SSD1306_WHITE);
                    
                    // Spawn puff particles
                    if (random(0, 10) == 0) {
                        spawnParticle(64, 45 + yOffset, ((float)random(4, 10)) / 10.0f, ((float)random(-3, 4)) / 10.0f, 4, 2, 30);
                    }
                    break;
                }

                case ACT_SNEEZING: {
                    unsigned long elapsed = idleActElapsedTime;
                    if (elapsed < 2500) {
                        int squeeze = (int)(elapsed / 250);
                        int sneezeShakeX = random(-2, 3);
                        int sneezeShakeY = random(-2, 3);
                        
                        int eyeHeight = 32 - squeeze * 3;
                        if (eyeHeight < 2) eyeHeight = 2;
                        int eyeY = 6 + yOffset + sneezeShakeY + (32 - eyeHeight) / 2;
                        drawEye(16 + sneezeShakeX, eyeY, eyeHeight);
                        drawEye(80 + sneezeShakeX, eyeY, eyeHeight);
                        
                        display.drawCircle(64 + sneezeShakeX, 44 + sneezeShakeY + yOffset, 3 + squeeze / 3, SSD1306_WHITE);
                    } else {
                        display.drawLine(14, 22 + yOffset, 30, 14 + yOffset, SSD1306_WHITE);
                        display.drawLine(30, 14 + yOffset, 46, 22 + yOffset, SSD1306_WHITE);
                        display.drawLine(14, 23 + yOffset, 30, 15 + yOffset, SSD1306_WHITE);
                        display.drawLine(30, 15 + yOffset, 46, 23 + yOffset, SSD1306_WHITE);
                        
                        display.drawLine(82, 22 + yOffset,  98, 14 + yOffset, SSD1306_WHITE);
                        display.drawLine(98, 14 + yOffset, 114, 22 + yOffset, SSD1306_WHITE);
                        display.drawLine(82, 23 + yOffset,  98, 15 + yOffset, SSD1306_WHITE);
                        display.drawLine(98, 15 + yOffset, 114, 23 + yOffset, SSD1306_WHITE);
                        
                        display.fillRoundRect(52, 36 + yOffset, 24, 20, 6, SSD1306_WHITE);
                        display.fillCircle(40, 48 + yOffset, 4, SSD1306_WHITE);
                        display.fillCircle(88, 48 + yOffset, 4, SSD1306_WHITE);
                        display.fillCircle(64, 56 + yOffset, 3, SSD1306_WHITE);
                        
                        // Sneeze droplets
                        if (elapsed < 2700) {
                            for (int p = 0; p < 4; p++) {
                                spawnParticle(64, 46 + yOffset, ((float)random(-30, 31)) / 10.0f, ((float)random(10, 30)) / 10.0f, 5, 1, 15);
                            }
                        }
                    }
                    break;
                }

                case ACT_YAWNING: {
                    display.drawCircle(32, 26 + yOffset, 10, SSD1306_WHITE);
                    display.fillRect(16, 26 + yOffset, 32, 12, SSD1306_BLACK);
                    display.drawCircle(96, 26 + yOffset, 10, SSD1306_WHITE);
                    display.fillRect(80, 26 + yOffset, 32, 12, SSD1306_BLACK);
                    int yawnOsc = (int)(sin(now * 0.008f) * 4) + 8;
                    display.fillRoundRect(58, 43 - yawnOsc / 2 + yOffset, 12, yawnOsc, 6, SSD1306_WHITE);
                    
                    // Tears
                    if (random(0, 15) == 0) {
                        spawnParticle(32, 26 + yOffset, -0.3f, 0.4f, 3, 1, 35);
                        spawnParticle(96, 26 + yOffset, 0.3f, 0.4f, 3, 1, 35);
                    }
                    break;
                }

                case ACT_NAPPING: {
                    int eyeY = 22 + yOffset;
                    int r = 8;
                    // Left eye (bottom curved arc / u-shape)
                    display.drawCircle(32, eyeY, r, SSD1306_WHITE);
                    display.fillRect(32 - r - 2, eyeY - r - 2, (r + 2) * 2, r + 2, SSD1306_BLACK);

                    // Right eye (bottom curved arc / u-shape)
                    display.drawCircle(96, eyeY, r, SSD1306_WHITE);
                    display.fillRect(96 - r - 2, eyeY - r - 2, (r + 2) * 2, r + 2, SSD1306_BLACK);
                    
                    // Cute tiny sleeping circle mouth
                    display.drawCircle(64, 40 + yOffset, 2, SSD1306_WHITE);

                    // Sleepy Z's
                    if (random(0, 20) == 0) {
                        spawnParticle(104, 24 + yOffset, ((float)random(2, 6)) / 10.0f, -0.4f, 2, (float)random(2, 5), 60);
                    }

                    // Display details (Battery & Volume) at the bottom
                    int pct = 0;
                    if (batteryVoltage >= 4.20f) pct = 100;
                    else if (batteryVoltage <= 3.45f) pct = 0;
                    else pct = (int)((batteryVoltage - 3.45f) / (4.20f - 3.45f) * 100.0f);

                    display.setTextSize(1);
                    display.setTextColor(SSD1306_WHITE);
                    display.setCursor(4, 54 + yOffset);
                    display.printf("BAT:%d%%", pct);

                    display.setCursor(80, 54 + yOffset);
                    display.printf("VOL:%d%%", buzzerVolume);
                    
                    break;
                }
            }
            break;
        }
    }

    // Update and render particle system
    updateAndDrawParticles(yOffset);

    display.display();
}

