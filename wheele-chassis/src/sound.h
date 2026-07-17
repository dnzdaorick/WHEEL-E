#pragma once

#include "config.h"

// ── Note / Melody Types ───────────────────────────────────────────────────────
struct Note {
    int frequency;
    unsigned long duration;
};

#define MAX_NOTES 16

// ── Melodic Sequencer State (defined in sound.cpp) ────────────────────────────
extern Note          currentMelody[MAX_NOTES];
extern int           melodyLength;
extern int           currentNoteIndex;
extern unsigned long noteStartTime;
extern bool          isPlayingSound;

// ── Polyphonic / Arpeggio State (defined in sound.cpp) ───────────────────────
extern int           activeFrequencies[9];
extern int           activeFreqCount;
extern unsigned long lastArpeggioFrameTime;
extern int           arpeggioIndexPointer;
extern unsigned long musicNoteOnTime;
extern bool          musicNotePendingStop;

// ── Blink Beep State (defined in sound.cpp) ───────────────────────────────────
extern unsigned long lastBackupBeep;
extern bool          backupBeepState;

// ── API ───────────────────────────────────────────────────────────────────────
void playMelody(int length);
void updateSoundEngine();

// Melody triggers
void triggerBootSound();
void triggerConnectedSound();
void triggerDisconnectedSound();
void triggerBlinkSound();
void triggerYawnSound();
void triggerSingingSound();
void triggerGiggleSound();
void triggerLookingSound();
void triggerDizzySound();
void triggerPoutingSound();
void triggerSighSound();
void triggerSneezeSound();
void triggerMovementNote(int frequency);
void triggerMusicModeUnlockSound();
void triggerMusicModeExitSound();
void triggerChargingPluggedSound();
void triggerLowBatterySound();
void triggerFullChargeSound();
