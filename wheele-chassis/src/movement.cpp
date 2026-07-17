#include "movement.h"
#include "sound.h"

// Combo gesture state (private to this module)
static unsigned long lastComboTapTime = 0;
static int           comboStateStep   = 0;

// Note-to-frequency lookup table for music mode (one-pass parser)
static const struct { const char* name; int freq; } NOTE_MAP[] = {
    {"NW",       523},
    {"FORWARD",  587},
    {"NE",       659},
    {"LEFT",     698},
    {"CENTER",   784},
    {"RIGHT",    880},
    {"SW",       988},
    {"BACKWARD", 1047},
    {"SE",       1175},
};
static constexpr int NOTE_MAP_SIZE = sizeof(NOTE_MAP) / sizeof(NOTE_MAP[0]);

// ── Motor Output Helper ───────────────────────────────────────────────────────
static void setMotors(bool in1, bool in2, bool in3, bool in4) {
    digitalWrite(MOTOR_IN1, in1 ? HIGH : LOW);
    digitalWrite(MOTOR_IN2, in2 ? HIGH : LOW);
    digitalWrite(MOTOR_IN3, in3 ? HIGH : LOW);
    digitalWrite(MOTOR_IN4, in4 ? HIGH : LOW);
}

// ─────────────────────────────────────────────────────────────────────────────
void processMovement(const char* command) {
    unsigned long now = millis();

    // Work on a mutable local copy so we can truncate compound commands safely
    char cmdBuf[64];
    strncpy(cmdBuf, command, sizeof(cmdBuf) - 1);
    cmdBuf[sizeof(cmdBuf) - 1] = '\0';

    // Drive mode: strip anything after the first comma (only one direction allowed)
    if (!isMusicMode) {
        char* comma = strchr(cmdBuf, ',');
        if (comma) *comma = '\0';
    }

    const char* cmd = cmdBuf;

    // Wake from sleep/yawn on any movement
    if (strcmp(cmd, "STOP") != 0 &&
        (currentIdleAct == ACT_NAPPING || currentIdleAct == ACT_YAWNING)) {
        currentIdleAct      = ACT_RESTING;
        idleActElapsedTime  = 0;
        idleActDuration     = random(2000, 10000);
        triggerConnectedSound();
    }

    // ── Music mode unlock combo: LEFT → RIGHT → LEFT → RIGHT ─────────────────
    if (strcmp(cmd, "STOP") != 0 &&
        strcmp(cmd, currentCmd) != 0 &&
        !strchr(cmd, ',')) {

        if (now - lastComboTapTime > 2000) {
            comboStateStep = 0;
        }
        lastComboTapTime = now;

        bool comboFired = false;
        if      (comboStateStep == 0 && strcmp(cmd, "LEFT")  == 0) comboStateStep = 1;
        else if (comboStateStep == 1 && strcmp(cmd, "RIGHT") == 0) comboStateStep = 2;
        else if (comboStateStep == 2 && strcmp(cmd, "LEFT")  == 0) comboStateStep = 3;
        else if (comboStateStep == 3 && strcmp(cmd, "RIGHT") == 0) {
            comboStateStep = 0;
            comboFired     = true;
        } else {
            comboStateStep = (strcmp(cmd, "LEFT") == 0) ? 1 : 0;
        }

        if (comboFired) {
            if (!isMusicMode) {
                isMusicMode   = true;
                currentFace   = FACE_MUSIC_MODE;
                triggerMusicModeUnlockSound();
                webSocket.broadcastTXT("UI_MUSIC");
            } else {
                isMusicMode            = false;
                currentFace            = FACE_IDLE;
                currentIdleAct         = ACT_RESTING;
                idleActElapsedTime     = 0;
                idleActDuration        = random(2000, 10000);
                sleepElapsedTime       = 0;
                musicInactivityElapsedTime = 0;
                triggerMusicModeExitSound();
                webSocket.broadcastTXT("UI_DRIVE");
            }
            strncpy(currentCmd, "STOP", sizeof(currentCmd));
            activeFreqCount = 0;
            noTone(BUZZER_PIN);
            return;
        }
    }

    // ── Music Mode — polyphonic note assignment (single-pass tokeniser) ───────
    if (isMusicMode) {
        if (strcmp(cmd, "STOP") == 0) {
            if (now - musicNoteOnTime >= 150) {
                noTone(BUZZER_PIN);
                musicNotePendingStop = false;
            } else {
                musicNotePendingStop = true;
            }
            activeFreqCount = 0;
        } else {
            // One-pass tokenise: scan comma-separated tokens once
            activeFreqCount = 0;
            char tokenBuf[64];
            strncpy(tokenBuf, cmd, sizeof(tokenBuf) - 1);
            tokenBuf[sizeof(tokenBuf) - 1] = '\0';
            char* token = strtok(tokenBuf, ",");
            while (token && activeFreqCount < NOTE_MAP_SIZE) {
                for (int i = 0; i < NOTE_MAP_SIZE; i++) {
                    if (strcmp(token, NOTE_MAP[i].name) == 0) {
                        activeFrequencies[activeFreqCount++] = NOTE_MAP[i].freq;
                        break;
                    }
                }
                token = strtok(nullptr, ",");
            }

            if (activeFreqCount > 0) {
                isPlayingSound       = false;
                currentNoteIndex     = -1;
                musicNoteOnTime      = now;
                musicNotePendingStop = false;
                // Single note: play immediately; chord: arpeggio handles it
                if (activeFreqCount == 1) {
                    tone(BUZZER_PIN, activeFrequencies[0]);
                }
            }
        }
        strncpy(currentCmd, cmd, sizeof(currentCmd) - 1);
        currentCmd[sizeof(currentCmd) - 1] = '\0';
        // Motors stay off in music mode
        setMotors(false, false, false, false);
        return;
    }

    // ── Drive Mode — directional audio feedback on state change ──────────────
    if (strcmp(cmd, currentCmd) != 0 && strcmp(cmd, "STOP") != 0) {
        if      (strcmp(cmd, "FORWARD")  == 0) triggerMovementNote(523);
        else if (strcmp(cmd, "BACKWARD") == 0) triggerMovementNote(587);
        else if (strcmp(cmd, "LEFT")     == 0) triggerMovementNote(659);
        else if (strcmp(cmd, "RIGHT")    == 0) triggerMovementNote(698);
    }

    strncpy(currentCmd, cmd, sizeof(currentCmd) - 1);
    currentCmd[sizeof(currentCmd) - 1] = '\0';

    // Motor output table
    if      (strcmp(cmd, "FORWARD")  == 0) setMotors(true,  false, false, true);
    else if (strcmp(cmd, "BACKWARD") == 0) setMotors(false, true,  true,  false);
    else if (strcmp(cmd, "LEFT")     == 0) setMotors(true,  false, true,  false);
    else if (strcmp(cmd, "RIGHT")    == 0) setMotors(false, true,  false, true);
    else                                    setMotors(false, false, false, false);
}
