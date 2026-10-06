#include "core/minipomo.h"

const char *const MINIPOMO_TYPE_NAME[MINIPOMO_TYPE_COUNT] = {
    "Pomodoro",
    "Short Break",
    "Long Break",
};

static const int DURATION_SEC[MINIPOMO_TYPE_COUNT] = {
    MINIPOMO_FOCUS_SEC,
    MINIPOMO_SHORT_BREAK_SEC,
    MINIPOMO_LONG_BREAK_SEC,
};

static double seconds_left(const struct minipomo_task *t, double now)
{
    double left = t->running ? t->deadline - now : t->remaining;
    double full = DURATION_SEC[t->type];
    return left < 0 ? 0 : left > full ? full : left;
}

static void start(struct minipomo_task *t, double now)
{
    if (!t->running) {
        t->deadline = now + t->remaining;
        t->running = true;
    }
}

static void pause(struct minipomo_task *t, double now)
{
    if (!t->running) {
        return;
    }
    t->focus_sec = minipomo_focus_sec(t, now);
    t->remaining = seconds_left(t, now);
    t->running = false;
}

static void set_type(struct minipomo_task *t, enum minipomo_type type,
                     double now)
{
    pause(t, now);
    t->type = type;
    t->remaining = DURATION_SEC[type];
}

static void finish(struct minipomo_task *t, bool counted, double now)
{
    enum minipomo_type next = MINIPOMO_FOCUS;
    if (t->type == MINIPOMO_FOCUS) {
        t->pomodoros += counted;
        t->done |= counted && t->pomodoros >= t->estimate;
        bool long_break =
            t->pomodoros && t->pomodoros % MINIPOMO_LONG_BREAK_EVERY == 0;
        next = long_break ? MINIPOMO_LONG_BREAK : MINIPOMO_SHORT_BREAK;
    }
    set_type(t, next, now);
}

static struct minipomo_task *current(struct minipomo *p)
{
    return p->current == MINIPOMO_NONE ? 0 : &p->tasks[p->current];
}

static int first_unfinished(const struct minipomo *p)
{
    for (int i = 0; i < p->task_count; i++) {
        if (!p->tasks[i].done) {
            return i;
        }
    }
    return MINIPOMO_NONE;
}

static void set_details(struct minipomo_task *t, const char *title,
                        const char *note, int estimate)
{
    minipomo_text_copy(t->title, MINIPOMO_TITLE_MAX, title,
                       minipomo_text_len(title));
    minipomo_text_copy(t->note, MINIPOMO_NOTE_MAX, note,
                       minipomo_text_len(note));
    t->estimate = estimate < 1                       ? 1
                  : estimate > MINIPOMO_ESTIMATE_MAX ? MINIPOMO_ESTIMATE_MAX
                                                     : estimate;
}

void minipomo_init(struct minipomo *p)
{
    p->task_count = 0;
    p->current = MINIPOMO_NONE;
}

int minipomo_add(struct minipomo *p, const char *title, const char *note,
                 int estimate)
{
    if (p->task_count == MINIPOMO_TASKS_MAX) {
        return MINIPOMO_NONE;
    }
    struct minipomo_task *t = &p->tasks[p->task_count];
    *t = (struct minipomo_task){ .remaining = MINIPOMO_FOCUS_SEC };
    set_details(t, title, note, estimate);
    return p->task_count++;
}

void minipomo_edit(struct minipomo *p, int i, const char *title,
                   const char *note, int estimate)
{
    set_details(&p->tasks[i], title, note, estimate);
}

void minipomo_remove(struct minipomo *p, int i)
{
    for (int k = i; k < p->task_count - 1; k++) {
        p->tasks[k] = p->tasks[k + 1];
    }
    p->task_count--;
    if (p->current == i) {
        p->current = MINIPOMO_NONE;
    } else if (p->current > i) {
        p->current--;
    }
}

void minipomo_move(struct minipomo *p, int from, int to)
{
    struct minipomo_task t = p->tasks[from];
    int step = to > from ? 1 : -1;
    for (int k = from; k != to; k += step) {
        p->tasks[k] = p->tasks[k + step];
    }
    p->tasks[to] = t;
    if (p->current == from) {
        p->current = to;
    } else if (from < p->current && p->current <= to) {
        p->current--;
    } else if (to <= p->current && p->current < from) {
        p->current++;
    }
}

void minipomo_play(struct minipomo *p, int i, double now)
{
    if (!minipomo_can_play(&p->tasks[i])) {
        return;
    }
    if (current(p)) {
        pause(current(p), now);
    }
    p->current = i;
    start(&p->tasks[i], now);
}

void minipomo_start(struct minipomo *p, double now)
{
    int i = current(p) ? p->current : first_unfinished(p);
    if (i != MINIPOMO_NONE) {
        minipomo_play(p, i, now);
    }
}

void minipomo_pause(struct minipomo *p, double now)
{
    if (current(p)) {
        pause(current(p), now);
    }
}

void minipomo_skip(struct minipomo *p, double now)
{
    if (current(p)) {
        finish(current(p), false, now);
    }
}

void minipomo_set_type(struct minipomo *p, enum minipomo_type type, double now)
{
    if (current(p)) {
        set_type(current(p), type, now);
    }
}

void minipomo_mark_done(struct minipomo *p, int i, bool done, double now)
{
    p->tasks[i].done = done;
    if (!minipomo_can_play(&p->tasks[i])) {
        pause(&p->tasks[i], now);
    }
}

bool minipomo_update(struct minipomo *p, double now)
{
    struct minipomo_task *t = current(p);
    if (!t || !t->running || now < t->deadline) {
        return false;
    }
    finish(t, true, now);
    return true;
}

bool minipomo_running(const struct minipomo *p)
{
    return p->current != MINIPOMO_NONE && p->tasks[p->current].running;
}

bool minipomo_can_play(const struct minipomo_task *t)
{
    return !t->done || t->type != MINIPOMO_FOCUS;
}

bool minipomo_can_start(const struct minipomo *p)
{
    if (p->current == MINIPOMO_NONE) {
        return first_unfinished(p) != MINIPOMO_NONE;
    }
    return minipomo_can_play(&p->tasks[p->current]);
}

enum minipomo_type minipomo_current_type(const struct minipomo *p)
{
    return p->current == MINIPOMO_NONE ? MINIPOMO_FOCUS
                                       : p->tasks[p->current].type;
}

int minipomo_seconds_left(const struct minipomo *p, double now)
{
    if (p->current == MINIPOMO_NONE) {
        return MINIPOMO_FOCUS_SEC;
    }
    double left = seconds_left(&p->tasks[p->current], now);
    int whole = (int)left;
    return whole < left ? whole + 1 : whole;
}

double minipomo_fraction_left(const struct minipomo *p, double now)
{
    if (p->current == MINIPOMO_NONE) {
        return 1;
    }
    const struct minipomo_task *t = &p->tasks[p->current];
    return seconds_left(t, now) / DURATION_SEC[t->type];
}

const char *minipomo_message(const struct minipomo *p)
{
    if (p->task_count == 0) {
        return MINIPOMO_TEXT_EMPTY;
    }
    if (p->current == MINIPOMO_NONE) {
        return "";
    }
    const struct minipomo_task *t = &p->tasks[p->current];
    if (t->type != MINIPOMO_FOCUS) {
        return MINIPOMO_TEXT_BREAK;
    }
    return t->title[0] ? t->title : MINIPOMO_TEXT_FOCUS;
}

double minipomo_focus_sec(const struct minipomo_task *t, double now)
{
    if (!t->running || t->type != MINIPOMO_FOCUS) {
        return t->focus_sec;
    }
    return t->focus_sec + t->remaining - seconds_left(t, now);
}

struct minipomo_stat minipomo_stat(const struct minipomo *p, double now)
{
    struct minipomo_stat s = { 0 };
    for (int i = 0; i < p->task_count; i++) {
        const struct minipomo_task *t = &p->tasks[i];
        s.pomodoros += t->pomodoros;
        s.estimate += t->estimate;
        s.focus_sec += minipomo_focus_sec(t, now);
        if (!t->done && t->estimate > t->pomodoros) {
            s.planned_sec += (t->estimate - t->pomodoros) *
                             (MINIPOMO_FOCUS_SEC + MINIPOMO_SHORT_BREAK_SEC);
        }
    }
    return s;
}
