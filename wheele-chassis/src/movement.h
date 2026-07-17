#pragma once

#include "config.h"

// ── API ───────────────────────────────────────────────────────────────────────
// Parses the incoming WebSocket command string, drives motors, handles
// music-mode note assignment, and manages the unlock/exit combo gesture.
void processMovement(const char* command);
