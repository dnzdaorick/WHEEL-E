#include "sound.h"

uint8_t buzzerVolume = 100;


// ── Melodic Sequencer State ───────────────────────────────────────────────────
Note          currentMelody[MAX_NOTES];
int           melodyLength     = 0;
int           currentNoteIndex = -1;
unsigned long noteStartTime    = 0;
bool          isPlayingSound   = false;

// ── Polyphonic / Arpeggio State ───────────────────────────────────────────────
int           activeFrequencies[9] = {};
int           activeFreqCount      = 0;
unsigned long lastArpeggioFrameTime = 0;
int           arpeggioIndexPointer  = 0;
unsigned long musicNoteOnTime       = 0;
bool          musicNotePendingStop  = false;

// ── Backup Beep State ─────────────────────────────────────────────────────────
unsigned long lastBackupBeep = 0;
bool          backupBeepState = false;

// ─────────────────────────────────────────────────────────────────────────────
void playMelody(int length) {
    melodyLength    = length;
    currentNoteIndex = 0;
    noteStartTime   = millis();
    isPlayingSound  = true;
    if (currentMelody[0].frequency > 0) {
        customTone(BUZZER_PIN, currentMelody[0].frequency);
    } else {
        noTone(BUZZER_PIN);
    }
}

// ── Sound Library ─────────────────────────────────────────────────────────────
void triggerBootSound() {
    currentMelody[0] = {523,  60};   // C5
    currentMelody[1] = {659,  60};   // E5
    currentMelody[2] = {784,  60};   // G5
    currentMelody[3] = {1047, 60};   // C6
    currentMelody[4] = {1319, 120};  // E6
    playMelody(5);
}

void triggerConnectedSound() {
    currentMelody[0] = {1319, 50};   // E6
    currentMelody[1] = {1568, 50};   // G6
    currentMelody[2] = {2093, 100};  // C7
    playMelody(3);
}

void triggerDisconnectedSound() {
    currentMelody[0] = {784,  100};  // G5 (sad drop)
    currentMelody[1] = {587,  100};  // D5
    currentMelody[2] = {494,  200};  // B4
    playMelody(3);
}

void triggerBlinkSound() {
    currentMelody[0] = {2400, 10};   // ultra high chirp
    playMelody(1);
}

void triggerYawnSound() {
    currentMelody[0] = {880,  90};   // A5 (drooping slide)
    currentMelody[1] = {784,  100};  // G5
    currentMelody[2] = {698,  110};  // F5
    currentMelody[3] = {659,  130};  // E5
    currentMelody[4] = {587,  150};  // D5
    currentMelody[5] = {523,  250};  // C5
    playMelody(6);
}

void triggerSingingSound() {
    currentMelody[0] = {523,  120};  // C5 (cheerful motif)
    currentMelody[1] = {784,  120};  // G5
    currentMelody[2] = {880,  120};  // A5
    currentMelody[3] = {784,  120};  // G5
    currentMelody[4] = {1047, 240};  // C6
    playMelody(5);
}

void triggerGiggleSound() {
    currentMelody[0] = {1319, 45};   // Fast high giggles
    currentMelody[1] = {1568, 45};
    currentMelody[2] = {1319, 45};
    currentMelody[3] = {1568, 45};
    currentMelody[4] = {1319, 45};
    currentMelody[5] = {1568, 100};
    playMelody(6);
}

void triggerLookingSound() {
    currentMelody[0] = {1047, 30};   // C6
    currentMelody[1] = {0,    40};    // rest
    currentMelody[2] = {1319, 40};   // E6 (curious double chirp)
    playMelody(3);
}

void triggerDizzySound() {
    currentMelody[0] = {880,  70};   // Wobbly out-of-tune spiral
    currentMelody[1] = {659,  70};
    currentMelody[2] = {784,  70};
    currentMelody[3] = {587,  70};
    currentMelody[4] = {740,  70};   // F#5
    currentMelody[5] = {554,  150};  // C#5
    playMelody(6);
}

void triggerPoutingSound() {
    currentMelody[0] = {110, 150};   // A2 (low buzz growl)
    currentMelody[1] = {98,  150};   // G2
    currentMelody[2] = {104, 250};   // G#2
    playMelody(3);
}

void triggerSighSound() {
    currentMelody[0] = {330,  120};  // E4 (long exhalation)
    currentMelody[1] = {294,  120};  // D4
    currentMelody[2] = {262,  140};  // C4
    currentMelody[3] = {247,  160};  // B3
    currentMelody[4] = {220,  300};  // A3
    playMelody(5);
}

void triggerSneezeSound() {
    currentMelody[0] = {880,  60};   // A5 (rapid inhale build)
    currentMelody[1] = {988,  60};   // B5
    currentMelody[2] = {1047, 60};   // C6
    currentMelody[3] = {1175, 120};  // D6
    currentMelody[4] = {0,    140};  // hold
    currentMelody[5] = {2794, 60};   // F7 (sneeze burst!)
    currentMelody[6] = {262,  200};  // C4 (sigh release)
    playMelody(7);
}

void triggerMovementNote(int frequency) {
    currentMelody[0] = {frequency, 140};
    playMelody(1);
}

void triggerMusicModeUnlockSound() {
    currentMelody[0] = {523,  60};
    currentMelody[1] = {659,  60};
    currentMelody[2] = {784,  60};
    currentMelody[3] = {1047, 60};
    currentMelody[4] = {1319, 60};
    currentMelody[5] = {1568, 200};
    playMelody(6);
}

void triggerMusicModeExitSound() {
    currentMelody[0] = {1047, 80};
    currentMelody[1] = {784,  80};
    currentMelody[2] = {659,  80};
    currentMelody[3] = {523,  200};
    playMelody(4);
}

void triggerChargingPluggedSound() {
    currentMelody[0] = {523,  80};   // C5
    currentMelody[1] = {659,  80};   // E5
    currentMelody[2] = {784,  80};   // G5
    currentMelody[3] = {1047, 150};  // C6
    playMelody(4);
}

void triggerLowBatterySound() {
    currentMelody[0] = {440, 150};   // A4 alert beep
    currentMelody[1] = {0,   100};
    currentMelody[2] = {440, 150};
    playMelody(3);
}

void triggerFullChargeSound() {
    currentMelody[0] = {523,  60};   // Triumphant fanfare
    currentMelody[1] = {659,  60};
    currentMelody[2] = {784,  60};
    currentMelody[3] = {1047, 60};
    currentMelody[4] = {1319, 60};
    currentMelody[5] = {1568, 200};
    playMelody(6);
}

// ── Background Sound Engine ───────────────────────────────────────────────────
void updateSoundEngine() {
    unsigned long now = millis();

    // 1. CHIPTUNE CHORD MULTIPLEXER — rapid square-wave arpeggiation for chords
    if (isMusicMode && activeFreqCount > 1) {
        if (now - lastArpeggioFrameTime >= 18) {
            lastArpeggioFrameTime = now;
            arpeggioIndexPointer = (arpeggioIndexPointer + 1) % activeFreqCount;
            customTone(BUZZER_PIN, activeFrequencies[arpeggioIndexPointer]);
        }
        return;  // arpeggio owns the buzzer while active
    }

    // 2. Envelope release for single-note taps in music mode
    if (isMusicMode && musicNotePendingStop) {
        if (now - musicNoteOnTime >= 150) {
            noTone(BUZZER_PIN);
            musicNotePendingStop = false;
        }
    }

    // 3. Automated melody sequencer
    if (isPlayingSound) {
        if (now - noteStartTime >= (unsigned long)currentMelody[currentNoteIndex].duration) {
            currentNoteIndex++;
            if (currentNoteIndex < melodyLength) {
                noteStartTime = now;
                if (currentMelody[currentNoteIndex].frequency > 0) {
                    customTone(BUZZER_PIN, currentMelody[currentNoteIndex].frequency);
                } else {
                    noTone(BUZZER_PIN);
                }
            } else {
                noTone(BUZZER_PIN);
                isPlayingSound   = false;
                currentNoteIndex = -1;
            }
        }
    }

    // 4. Continuous reverse warning beep
    if (strcmp(currentCmd, "BACKWARD") == 0 && !isMusicMode) {
        if (!isPlayingSound) {
            if (now - lastBackupBeep >= 250) {
                lastBackupBeep  = now;
                backupBeepState = !backupBeepState;
                if (backupBeepState) {
                    customTone(BUZZER_PIN, 349);
                } else {
                    noTone(BUZZER_PIN);
                }
            }
        }
    } else {
        if (backupBeepState) {
            noTone(BUZZER_PIN);
            backupBeepState = false;
        }
    }
}

void customTone(uint8_t pin, unsigned int frequency) {
    uint8_t channel = 0;
    if (frequency == 0 || buzzerVolume == 0) {
        ledcWrite(channel, 0);
        noTone(pin);
        return;
    }
    ledcSetup(channel, frequency, 8);
    ledcAttachPin(pin, channel);
    // Cubic scaling for human hearing and to counteract the piezo buzzer's sharp response threshold
    float volFrac = (float)buzzerVolume / 100.0f;
    float volCubic = volFrac * volFrac * volFrac;
    
    uint32_t minDuty = 3;
    uint32_t maxDuty = 127;
    uint32_t duty = minDuty + (uint32_t)((maxDuty - minDuty) * volCubic);
    
    ledcWrite(channel, duty);
}

