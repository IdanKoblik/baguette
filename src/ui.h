#pragma once

#include "hud.h"

int init_surface(struct hud_state *state);

int init_buffer(struct hud_state *state);

void hud_draw(struct hud_state *state, const char *left, const char *center, const char *right);
