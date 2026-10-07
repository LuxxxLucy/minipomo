#include "app/display.h"

static const char TEXT_FOCUS[] = "Time to focus!";
static const char TEXT_BREAK[] = "Time for a break!";
static const char TEXT_EMPTY[] = "Add a task to start";

const char *const APP_PHASE_NAMES[MINIPOMO_PHASE_COUNT] = {
    "Pomodoro",
    "Short Break",
    "Long Break",
};

int app_seconds_left(const struct minipomo *state)
{
    struct minipomo_timer_view timer = minipomo_get_timer(state);
    return (int)((timer.remaining_ms + 999) / 1000);
}

double app_fraction_left(const struct minipomo *state)
{
    struct minipomo_timer_view timer = minipomo_get_timer(state);
    return (double)timer.remaining_ms / timer.duration_ms;
}

const char *app_message(const struct minipomo *state)
{
    if (!state->task_count) {
        return TEXT_EMPTY;
    }
    if (state->current == MINIPOMO_NONE) {
        return "";
    }
    const struct minipomo_task *task = &state->tasks[state->current];
    return task->timer.phase == MINIPOMO_FOCUS ? task->title : TEXT_BREAK;
}

const char *app_completion_message(const struct minipomo_completion *completed)
{
    if (completed->phase == MINIPOMO_FOCUS) {
        return TEXT_BREAK;
    }
    return completed->title[0] ? completed->title : TEXT_FOCUS;
}
