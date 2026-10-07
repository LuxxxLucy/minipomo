#include "core/save.h"
#include "core/text.h"

#include <limits.h>

#define KEY_MAX 16
#define NUMBER_MAX 32
#define MS_PER_SEC 1000

static const int64_t DURATIONS[] = { MINIPOMO_FOCUS_MS, MINIPOMO_SHORT_BREAK_MS,
                                     MINIPOMO_LONG_BREAK_MS };

struct writer {
    char *out;
    int capacity;
    int length;
};

static void put(struct writer *w, char c)
{
    if (w->length < w->capacity) {
        w->out[w->length] = c;
    }
    w->length++;
}

static void text(struct writer *w, const char *s)
{
    for (; *s; s++) {
        put(w, *s);
    }
}

static void key(struct writer *w, const char *name)
{
    put(w, '\t');
    text(w, name);
    put(w, '=');
}

static void integer(struct writer *w, const char *name, int64_t n)
{
    char value[NUMBER_MAX];
    minipomo_format_int(value, n);
    key(w, name);
    text(w, value);
}

static void seconds(struct writer *w, const char *name, int64_t n)
{
    integer(w, name, n / MS_PER_SEC);
    put(w, '.');
    for (int place = MS_PER_SEC / 10; place; place /= 10) {
        put(w, '0' + (char)(n / place % 10));
    }
}

static void escaped(struct writer *w, const char *name, const char *s)
{
    key(w, name);
    for (; *s; s++) {
        if (*s == '\\' || *s == '\t' || *s == '\n') {
            put(w, '\\');
            put(w, *s == '\t' ? 't' : *s == '\n' ? 'n' : '\\');
        } else {
            put(w, *s);
        }
    }
}

static bool terminated(const char *s, int capacity)
{
    for (int i = 0; i < capacity; i++) {
        if (!s[i]) {
            return true;
        }
    }
    return false;
}

static bool valid(const struct minipomo *p)
{
    if (p->task_count < 0 || p->task_count > MINIPOMO_TASKS_MAX ||
        p->current < MINIPOMO_NONE || p->current >= p->task_count ||
        p->updated_at_ms < 0 || p->updated_at_ms > MINIPOMO_TIME_MAX) {
        return false;
    }
    for (int i = 0; i < p->task_count; i++) {
        const struct minipomo_task *t = &p->tasks[i];
        const struct minipomo_timer *timer = &t->timer;
        if (t->estimate < 1 || t->estimate > MINIPOMO_ESTIMATE_MAX ||
            t->pomodoros < 0 || t->focus_ms < 0 ||
            t->focus_ms > MINIPOMO_TIME_MAX || timer->phase < 0 ||
            timer->phase >= MINIPOMO_PHASE_COUNT || timer->remaining_ms < 0 ||
            timer->remaining_ms > DURATIONS[timer->phase] ||
            (timer->running && (i != p->current ||
                                (t->done && timer->phase == MINIPOMO_FOCUS))) ||
            !terminated(t->title, sizeof t->title) ||
            !terminated(t->note, sizeof t->note)) {
            return false;
        }
    }
    return true;
}

int minipomo_save(const struct minipomo *p, char *out, int capacity)
{
    if (!p || !out || capacity < 0 || !valid(p)) {
        return -1;
    }
    struct writer w = { out, capacity, 0 };
    text(&w, "pomo");
    integer(&w, "current", p->current);
    put(&w, '\n');
    for (int i = 0; i < p->task_count; i++) {
        const struct minipomo_task *t = &p->tasks[i];
        text(&w, "task");
        integer(&w, "done", t->done);
        integer(&w, "estimate", t->estimate);
        integer(&w, "pomodoros", t->pomodoros);
        seconds(&w, "focus_sec", t->focus_ms);
        integer(&w, "type", t->timer.phase);
        integer(&w, "running", t->timer.running);
        seconds(
            &w, "deadline",
            t->timer.running ? p->updated_at_ms + t->timer.remaining_ms : 0);
        seconds(&w, "remaining", t->timer.remaining_ms);
        escaped(&w, "title", t->title);
        escaped(&w, "note", t->note);
        put(&w, '\n');
    }
    return w.length <= capacity ? w.length : -1;
}

static bool same(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static const char *read_text(const char *s, const char *end, char stop,
                             char *out, int capacity)
{
    int n = 0;
    while (s < end && *s != stop && *s != '\t' && *s != '\n') {
        char c = *s++;
        if (c == '\\') {
            if (s == end || (*s != 't' && *s != 'n' && *s != '\\')) {
                return 0;
            }
            c = *s++;
            c = c == 't' ? '\t' : c == 'n' ? '\n' : c;
        }
        if (!c || n == capacity - 1) {
            return 0;
        }
        out[n++] = c;
    }
    out[n] = '\0';
    return s;
}

static bool number(const char *s, bool seconds_value, int64_t *value)
{
    bool negative = *s == '-';
    s += negative;
    if (*s < '0' || *s > '9') {
        return false;
    }
    int64_t n = 0;
    while (*s >= '0' && *s <= '9') {
        int digit = *s++ - '0';
        if (n > (INT64_MAX - digit) / 10) {
            return false;
        }
        n = n * 10 + digit;
    }
    if (seconds_value) {
        int fraction = 0, place = MS_PER_SEC / 10;
        if (*s == '.') {
            s++;
            if (*s < '0' || *s > '9') {
                return false;
            }
            while (*s >= '0' && *s <= '9') {
                fraction += (*s++ - '0') * place;
                place /= 10;
            }
        }
        if (n > (INT64_MAX - fraction) / MS_PER_SEC) {
            return false;
        }
        n = n * MS_PER_SEC + fraction;
    }
    *value = negative ? -n : n;
    return !*s;
}

static bool field(struct minipomo_task *t, int64_t *deadline, const char *name,
                  const char *value)
{
    if (same(name, "title") || same(name, "note")) {
        bool title = same(name, "title");
        int capacity = title ? MINIPOMO_TITLE_MAX : MINIPOMO_NOTE_MAX;
        int len = minipomo_text_len(value);
        if (len >= capacity) {
            return false;
        }
        minipomo_text_copy(title ? t->title : t->note, capacity, value, len);
        return true;
    }
    bool time = same(name, "focus_sec") || same(name, "deadline") ||
                same(name, "remaining");
    bool count = same(name, "done") || same(name, "estimate") ||
                 same(name, "pomodoros") || same(name, "type") ||
                 same(name, "running");
    if (!time && !count) {
        return true;
    }
    int64_t n;
    if (!number(value, time, &n) || n < 0 || (count && n > INT_MAX)) {
        return false;
    }
    if (same(name, "done")) {
        t->done = n != 0;
        return n <= 1;
    } else if (same(name, "estimate")) {
        t->estimate = (int)n;
    } else if (same(name, "pomodoros")) {
        t->pomodoros = (int)n;
    } else if (same(name, "type")) {
        t->timer.phase = (enum minipomo_phase)n;
    } else if (same(name, "running")) {
        t->timer.running = n != 0;
        return n <= 1;
    } else if (same(name, "focus_sec")) {
        t->focus_ms = n;
    } else if (same(name, "deadline")) {
        *deadline = n;
    } else {
        t->timer.remaining_ms = n;
    }
    return true;
}

int minipomo_load(struct minipomo *p, const char *in, int len)
{
    if (!p || len < 0 || (!in && len)) {
        return -1;
    }
    struct minipomo candidate = { .current = MINIPOMO_NONE };
    if (!len) {
        *p = candidate;
        return 0;
    }
    const char *s = in, *end = in + len;
    bool header = false;
    while (s < end) {
        char name[KEY_MAX], name_key[KEY_MAX], value[MINIPOMO_NOTE_MAX];
        s = read_text(s, end, '\t', name, sizeof name);
        if (!s) {
            return -1;
        }
        struct minipomo_task *t = 0;
        int64_t deadline = -1;
        if (same(name, "pomo")) {
            if (header || candidate.task_count) {
                return -1;
            }
            header = true;
        } else if (same(name, "task")) {
            if (!header || candidate.task_count == MINIPOMO_TASKS_MAX) {
                return -1;
            }
            t = &candidate.tasks[candidate.task_count++];
            t->estimate = 1;
            t->timer.remaining_ms = MINIPOMO_FOCUS_MS;
        } else {
            return -1;
        }
        while (s < end && *s == '\t') {
            s = read_text(s + 1, end, '=', name_key, sizeof name_key);
            if (!s || s == end || *s != '=' || !name_key[0]) {
                return -1;
            }
            s = read_text(s + 1, end, '\t', value, sizeof value);
            if (!s || (t && !field(t, &deadline, name_key, value))) {
                return -1;
            }
            if (!t && same(name_key, "current")) {
                int64_t n;
                if (!number(value, false, &n) || n < MINIPOMO_NONE ||
                    n >= MINIPOMO_TASKS_MAX) {
                    return -1;
                }
                candidate.current = (int)n;
            }
        }
        if (t && t->timer.running) {
            if (deadline < t->timer.remaining_ms ||
                deadline > MINIPOMO_TIME_MAX + MINIPOMO_FOCUS_MS) {
                return -1;
            }
            candidate.updated_at_ms = deadline - t->timer.remaining_ms;
        }
        if (s < end && *s++ != '\n') {
            return -1;
        }
    }
    if (!valid(&candidate)) {
        return -1;
    }
    *p = candidate;
    return 0;
}
