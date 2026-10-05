#include "core/minipomo.h"

// One record per line: a name, then tab-separated key=value fields. Values
// escape backslash, tab, and newline. Unknown keys are skipped.
//
//   pomo  current=0
//   task  done=0  estimate=3  pomodoros=1  focus_sec=2520  type=0  running=1
//         deadline=...  remaining=1500  title=Write report  note=...

static char *put_key(char *s, const char *key)
{
    *s++ = '\t';
    s = minipomo_text_put(s, key);
    *s++ = '=';
    return s;
}

static char *put_int(char *s, const char *key, long long n)
{
    return minipomo_format_int(put_key(s, key), n);
}

static char *put_text(char *s, const char *key, const char *text)
{
    s = put_key(s, key);
    for (; *text; text++) {
        if (*text == '\\' || *text == '\t' || *text == '\n') {
            *s++ = '\\';
            *s++ = *text == '\t' ? 't' : *text == '\n' ? 'n' : '\\';
        } else {
            *s++ = *text;
        }
    }
    return s;
}

int minipomo_save(const struct minipomo *p, char *out)
{
    char *s = put_int(minipomo_text_put(out, "pomo"), "current", p->current);
    *s++ = '\n';
    for (int i = 0; i < p->task_count; i++) {
        const struct minipomo_task *t = &p->tasks[i];
        s = minipomo_text_put(s, "task");
        s = put_int(s, "done", t->done);
        s = put_int(s, "estimate", t->estimate);
        s = put_int(s, "pomodoros", t->pomodoros);
        s = put_int(s, "focus_sec", (long long)t->focus_sec);
        s = put_int(s, "type", t->type);
        s = put_int(s, "running", t->running);
        s = put_int(s, "deadline", (long long)t->deadline);
        s = put_int(s, "remaining", (long long)t->remaining);
        s = put_text(s, "title", t->title);
        s = put_text(s, "note", t->note);
        *s++ = '\n';
    }
    return (int)(s - out);
}

static const char *get_text(const char *s, const char *end, char stop,
                            char *dst, int cap)
{
    int n = 0;
    while (s < end && *s != stop && *s != '\t' && *s != '\n') {
        char c = *s++;
        if (c == '\\' && s < end) {
            c = *s++;
            c = c == 't' ? '\t' : c == 'n' ? '\n' : c;
        }
        if (n < cap - 1) {
            dst[n++] = c;
        }
    }
    dst[n] = '\0';
    return s;
}

static long long get_int(const char *s)
{
    bool negative = *s == '-';
    unsigned long long n = 0;
    for (s += negative; *s >= '0' && *s <= '9'; s++) {
        n = n * 10 + (unsigned)(*s - '0');
    }
    return negative ? -(long long)n : (long long)n;
}

static bool same(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static void set_field(struct minipomo_task *t, const char *key,
                      const char *value)
{
    long long n = get_int(value);
    if (same(key, "done")) {
        t->done = n != 0;
    } else if (same(key, "estimate")) {
        t->estimate = (int)n;
    } else if (same(key, "pomodoros")) {
        t->pomodoros = (int)n;
    } else if (same(key, "focus_sec")) {
        t->focus_sec = (double)n;
    } else if (same(key, "type") && n >= 0 && n < MINIPOMO_TYPE_COUNT) {
        t->type = (enum minipomo_type)n;
    } else if (same(key, "running")) {
        t->running = n != 0;
    } else if (same(key, "deadline")) {
        t->deadline = (double)n;
    } else if (same(key, "remaining")) {
        t->remaining = (double)n;
    } else if (same(key, "title")) {
        minipomo_text_copy(t->title, MINIPOMO_TITLE_MAX, value,
                           minipomo_text_len(value));
    } else if (same(key, "note")) {
        minipomo_text_copy(t->note, MINIPOMO_NOTE_MAX, value,
                           minipomo_text_len(value));
    }
}

void minipomo_load(struct minipomo *p, const char *in, int len)
{
    const char *s = in, *end = in + len;
    minipomo_init(p);
    while (s < end) {
        char name[MINIPOMO_KEY_MAX], key[MINIPOMO_KEY_MAX],
            value[MINIPOMO_NOTE_MAX + 1];
        s = get_text(s, end, '\t', name, MINIPOMO_KEY_MAX);
        int i = same(name, "task") ? minipomo_add(p, "", "", 1) : MINIPOMO_NONE;
        struct minipomo_task *t = i == MINIPOMO_NONE ? 0 : &p->tasks[i];
        while (s < end && *s == '\t') {
            s = get_text(s + 1, end, '=', key, MINIPOMO_KEY_MAX);
            s += s < end && *s == '=';
            s = get_text(s, end, '\t', value, sizeof value);
            if (t) {
                set_field(t, key, value);
            } else if (same(name, "pomo") && same(key, "current")) {
                p->current = (int)get_int(value);
            }
        }
        if (t) {
            minipomo_edit(p, i, t->title, t->note, t->estimate);
        }
        s += s < end;
    }
    if (p->current < MINIPOMO_NONE || p->current >= p->task_count) {
        p->current = MINIPOMO_NONE;
    }
    for (int i = 0; i < p->task_count; i++) {
        p->tasks[i].running &= i == p->current;
    }
}
