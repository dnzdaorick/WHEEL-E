#include "sound.h"

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
        tone(BUZZER_PIN, currentMelody[0].frequency);
    } else {
        noTone(BUZZER_PIN);
    }
}

// ── Sound Library ─────────────────────────────────────────────────────────────
void triggerBootSound() {
    currentMelody[0] = {523,  100};  // C5
    currentMelody[1] = {659,  100};  // E5
    currentMelody[2] = {784,  100};  // G5
    currentMelody[3] = {1047, 180};  // C6
    playMelody(4);
}

void triggerConnectedSound() {
    currentMelody[0] = {880,  80};   // A5
    currentMelody[1] = {1047, 80};   // C6
    currentMelody[2] = {1319, 80};   // E6
    currentMelody[3] = {1568, 150};  // G6
    playMelody(4);
}

void triggerDisconnectedSound() {
    currentMelody[0] = {784, 150};
    currentMelody[1] = {622, 150};
    currentMelody[2] = {494, 250};
    playMelody(3);
}

void triggerBlinkSound() {
    currentMelody[0] = {2000, 15};
    playMelody(1);
}

void triggerYawnSound() {
    currentMelody[0] = {700, 70};
    currentMelody[1] = {600, 70};
    currentMelody[2] = {500, 70};
    currentMelody[3] = {400, 70};
    currentMelody[4] = {300, 150};
    currentMelody[5] = {350, 150};
    playMelody(6);
}

void triggerSingingSound() {
    currentMelody[0] = {523,  80};
    currentMelody[1] = {659,  80};
    currentMelody[2] = {784,  80};
    currentMelody[3] = {1047, 80};
    currentMelody[4] = {1319, 80};
    currentMelody[5] = {1568, 150};
    playMelody(6);
}

void triggerGiggleSound() {
    currentMelody[0] = {1300, 60};
    currentMelody[1] = {1600, 60};
    currentMelody[2] = {1400, 60};
    currentMelody[3] = {1800, 60};
    currentMelody[4] = {1500, 60};
    currentMelody[5] = {1900, 120};
    playMelody(6);
}

void triggerLookingSound() {
    currentMelody[0] = {2100, 15};
    currentMelody[1] = {0,    100};   // rest
    currentMelody[2] = {2300, 15};
    playMelody(3);
}

void triggerDizzySound() {
    currentMelody[0] = {900,  80};
    currentMelody[1] = {700,  80};
    currentMelody[2] = {500,  80};
    currentMelody[3] = {600,  80};
    currentMelody[4] = {800,  80};
    currentMelody[5] = {1000, 150};
    playMelody(6);
}

void triggerPoutingSound() {
    currentMelody[0] = {220, 150};
    currentMelody[1] = {180, 150};
    currentMelody[2] = {200, 250};
    playMelody(3);
}

void triggerSighSound() {
    currentMelody[0] = {440, 100};
    currentMelody[1] = {380, 100};
    currentMelody[2] = {311, 100};
    currentMelody[3] = {220, 120};
    currentMelody[4] = {147, 250};
    playMelody(5);
}

void triggerSneezeSound() {
    currentMelody[0] = {400,  150};
    currentMelody[1] = {500,  150};
    currentMelody[2] = {600,  150};
    currentMelody[3] = {700,  200};
    currentMelody[4] = {0,    150};   // rest
    currentMelody[5] = {1800, 80};
    currentMelody[6] = {300,  200};
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
    currentMelody[0] = {523,  100};
    currentMelody[1] = {784,  100};
    currentMelody[2] = {1047, 150};
    playMelody(3);
}

void triggerLowBatterySound() {
    currentMelody[0] = {220, 200};
    currentMelody[1] = {0,   150};    // rest
    currentMelody[2] = {220, 200};
    playMelody(3);
}

void triggerFullChargeSound() {
    currentMelody[0] = {523,  80};
    currentMelody[1] = {659,  80};
    currentMelody[2] = {784,  80};
    currentMelody[3] = {988,  80};
    currentMelody[4] = {1047, 250};
    playMelody(5);
}

// ── Background Sound Engine ───────────────────────────────────────────────────
void updateSoundEngine() {
    unsigned long now = millis();

    // 1. CHIPTUNE CHORD MULTIPLEXER — rapid square-wave arpeggiation for chords
    if (isMusicMode && activeFreqCount > 1) {
        if (now - lastArpeggioFrameTime >= 18) {
            lastArpeggioFrameTime = now;
            arpeggioIndexPointer = (arpeggioIndexPointer + 1) % activeFreqCount;
            tone(BUZZER_PIN, activeFrequencies[arpeggioIndexPointer]);
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
                    tone(BUZZER_PIN, currentMelody[currentNoteIndex].frequency);
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
                    tone(BUZZER_PIN, 349);
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
