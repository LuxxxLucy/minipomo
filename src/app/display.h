#ifndef MINIPOMO_DISPLAY_H
#define MINIPOMO_DISPLAY_H

#include "core/minipomo.h"

extern const char *const APP_PHASE_NAMES[MINIPOMO_PHASE_COUNT];
int app_seconds_left(const struct minipomo *state);
double app_fraction_left(const struct minipomo *state);
const char *app_message(const struct minipomo *state);
const char *app_completion_message(const struct minipomo_completion *completed);

#endif
