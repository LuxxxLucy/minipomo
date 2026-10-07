#include "core/minipomo.h"
#include "core/text.h"

#include <limits.h>

static const int64_t PHASE_DURATION_MS[MINIPOMO_PHASE_COUNT] = {
    MINIPOMO_FOCUS_MS,
    MINIPOMO_SHORT_BREAK_MS,
    MINIPOMO_LONG_BREAK_MS,
};

static bool has_task(const struct minipomo *state, int index)
{
    return index >= 0 && index < state->task_count;
}

static int start_task_index(const struct minipomo *state)
{
    if (has_task(state, state->current)) {
        return state->current;
    }
    for (int index = 0; index < state->task_count; index++) {
        if (!state->tasks[index].done) {
            return index;
        }
    }
    return MINIPOMO_NONE;
}

static void set_phase(struct minipomo_task *task, enum minipomo_phase phase)
{
    task->timer = (struct minipomo_timer){
        .phase = phase,
        .remaining_ms = PHASE_DURATION_MS[phase],
    };
}

static void finish_phase(struct minipomo_task *task, bool completed)
{
    enum minipomo_phase next = MINIPOMO_FOCUS;
    if (task->timer.phase == MINIPOMO_FOCUS) {
        if (completed) {
            task->pomodoros += task->pomodoros < INT_MAX;
            task->done |= task->pomodoros >= task->estimate;
        }
        next =
            task->pomodoros && task->pomodoros % MINIPOMO_LONG_BREAK_EVERY == 0
                ? MINIPOMO_LONG_BREAK
                : MINIPOMO_SHORT_BREAK;
    }
    set_phase(task, next);
}

static bool valid_details(struct minipomo_details details)
{
    return details.title && details.title[0] && details.note &&
           details.estimate >= 1 && details.estimate <= MINIPOMO_ESTIMATE_MAX;
}

static void set_details(struct minipomo_task *task,
                        struct minipomo_details details)
{
    minipomo_text_copy(task->title, sizeof task->title, details.title,
                       minipomo_text_len(details.title));
    minipomo_text_copy(task->note, sizeof task->note, details.note,
                       minipomo_text_len(details.note));
    task->estimate = details.estimate;
}

static void start_task(struct minipomo *state, int index)
{
    if (!minipomo_get_can_start_task(state, index)) {
        return;
    }
    if (has_task(state, state->current)) {
        state->tasks[state->current].timer.running = false;
    }
    state->current = index;
    state->tasks[index].timer.running = true;
}

static void move_task(struct minipomo *state, int from, int to)
{
    struct minipomo_task task = state->tasks[from];
    int step = to > from ? 1 : -1;
    for (int index = from; index != to; index += step) {
        state->tasks[index] = state->tasks[index + step];
    }
    state->tasks[to] = task;
    if (state->current == from) {
        state->current = to;
    } else if (from < state->current && state->current <= to) {
        state->current--;
    } else if (to <= state->current && state->current < from) {
        state->current++;
    }
}

void minipomo_init(struct minipomo *state)
{
    *state = (struct minipomo){ .current = MINIPOMO_NONE };
}

struct minipomo_result minipomo_update(struct minipomo *state, int64_t now_ms)
{
    struct minipomo_result result = {
        .added_task = MINIPOMO_NONE,
        .completed.phase = MINIPOMO_NO_COMPLETION,
    };
    if (now_ms < 0 || now_ms > MINIPOMO_TIME_MAX) {
        result.status = -1;
        return result;
    }
    int64_t elapsed_ms =
        now_ms > state->updated_at_ms ? now_ms - state->updated_at_ms : 0;
    state->updated_at_ms += elapsed_ms;
    if (!has_task(state, state->current)) {
        return result;
    }
    struct minipomo_task *task = &state->tasks[state->current];
    if (!task->timer.running) {
        return result;
    }
    if (elapsed_ms > task->timer.remaining_ms) {
        elapsed_ms = task->timer.remaining_ms;
    }
    task->timer.remaining_ms -= elapsed_ms;
    if (task->timer.phase == MINIPOMO_FOCUS) {
        int64_t capacity_ms = MINIPOMO_TIME_MAX - task->focus_ms;
        task->focus_ms += elapsed_ms < capacity_ms ? elapsed_ms : capacity_ms;
    }
    if (task->timer.remaining_ms == 0) {
        result.completed.phase = task->timer.phase;
        minipomo_text_copy(result.completed.title,
                           sizeof result.completed.title, task->title,
                           minipomo_text_len(task->title));
        finish_phase(task, true);
    }
    return result;
}

struct minipomo_result minipomo_modify(struct minipomo *state,
                                       struct minipomo_change change,
                                       int64_t now_ms)
{
    struct minipomo_result result = minipomo_update(state, now_ms);
    if (result.status < 0) {
        return result;
    }
    switch (change.type) {
        case MINIPOMO_ADD: {
            if (state->task_count == MINIPOMO_TASKS_MAX ||
                !valid_details(change.add)) {
                break;
            }
            int index = state->task_count++;
            state->tasks[index] = (struct minipomo_task){ 0 };
            set_details(&state->tasks[index], change.add);
            set_phase(&state->tasks[index], MINIPOMO_FOCUS);
            result.added_task = index;
            return result;
        }
        case MINIPOMO_EDIT:
            if (!has_task(state, change.edit.task) ||
                !valid_details(change.edit.details)) {
                break;
            }
            set_details(&state->tasks[change.edit.task], change.edit.details);
            return result;
        case MINIPOMO_REMOVE:
            if (!has_task(state, change.remove)) {
                break;
            }
            for (int index = change.remove; index < state->task_count - 1;
                 index++) {
                state->tasks[index] = state->tasks[index + 1];
            }
            state->tasks[--state->task_count] = (struct minipomo_task){ 0 };
            if (state->current == change.remove) {
                state->current = MINIPOMO_NONE;
            } else if (state->current > change.remove) {
                state->current--;
            }
            return result;
        case MINIPOMO_MOVE:
            if (!has_task(state, change.move.from) ||
                !has_task(state, change.move.to)) {
                break;
            }
            move_task(state, change.move.from, change.move.to);
            return result;
        case MINIPOMO_START_CURRENT:
            start_task(state, start_task_index(state));
            return result;
        case MINIPOMO_START_TASK:
            if (!has_task(state, change.start_task)) {
                break;
            }
            start_task(state, change.start_task);
            return result;
        case MINIPOMO_PAUSE:
            if (has_task(state, state->current)) {
                state->tasks[state->current].timer.running = false;
            }
            return result;
        case MINIPOMO_SKIP:
            if (has_task(state, state->current) &&
                state->tasks[state->current].timer.running) {
                finish_phase(&state->tasks[state->current], false);
            }
            return result;
        case MINIPOMO_SET_PHASE: {
            if (change.phase < MINIPOMO_FOCUS ||
                change.phase >= MINIPOMO_PHASE_COUNT) {
                break;
            }
            int index = start_task_index(state);
            if (has_task(state, index)) {
                state->current = index;
                set_phase(&state->tasks[index], change.phase);
            }
            return result;
        }
        case MINIPOMO_SET_DONE:
            if (!has_task(state, change.set_done.task)) {
                break;
            }
            state->tasks[change.set_done.task].done = change.set_done.done;
            if (!minipomo_get_can_start_task(state, change.set_done.task)) {
                state->tasks[change.set_done.task].timer.running = false;
            }
            return result;
        case MINIPOMO_CLEAR: {
            int64_t updated_at_ms = state->updated_at_ms;
            minipomo_init(state);
            state->updated_at_ms = updated_at_ms;
            return result;
        }
    }
    result.status = -1;
    return result;
}

bool minipomo_get_can_start_task(const struct minipomo *state, int index)
{
    return has_task(state, index) &&
           (!state->tasks[index].done ||
            state->tasks[index].timer.phase != MINIPOMO_FOCUS);
}

struct minipomo_timer_view minipomo_get_timer(const struct minipomo *state)
{
    int index = start_task_index(state);
    struct minipomo_timer_view view = {
        .phase = MINIPOMO_FOCUS,
        .remaining_ms = MINIPOMO_FOCUS_MS,
        .duration_ms = MINIPOMO_FOCUS_MS,
    };
    if (has_task(state, index)) {
        const struct minipomo_timer *timer = &state->tasks[index].timer;
        view.phase = timer->phase;
        view.remaining_ms = timer->remaining_ms;
        view.duration_ms = PHASE_DURATION_MS[timer->phase];
        view.running = timer->running;
        view.can_start = minipomo_get_can_start_task(state, index);
    }
    return view;
}

struct minipomo_stats minipomo_get_stats(const struct minipomo *state)
{
    struct minipomo_stats stats = { 0 };
    for (int index = 0; index < state->task_count; index++) {
        const struct minipomo_task *task = &state->tasks[index];
        stats.pomodoros += task->pomodoros;
        stats.estimate += task->estimate;
        stats.focus_ms += task->focus_ms;
        if (!task->done && task->estimate > task->pomodoros) {
            stats.planned_ms += (int64_t)(task->estimate - task->pomodoros) *
                                (MINIPOMO_FOCUS_MS + MINIPOMO_SHORT_BREAK_MS);
        }
    }
    return stats;
}
