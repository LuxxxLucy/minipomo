#ifndef MINIPOMO_SAVE_H
#define MINIPOMO_SAVE_H

#include "core/minipomo.h"

#define MINIPOMO_SAVE_MAX       \
    (256 + MINIPOMO_TASKS_MAX * \
               (2 * (MINIPOMO_TITLE_MAX + MINIPOMO_NOTE_MAX) + 384))

int minipomo_save(const struct minipomo *state, char *out, int capacity);
int minipomo_load(struct minipomo *state, const char *text, int length);

#endif
