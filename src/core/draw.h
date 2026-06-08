#pragma once

#include "hud.h"

#define HUD_PADDING 4

void draw_hud(struct hud_state *state, const char *left, const char *center, const char *right);
