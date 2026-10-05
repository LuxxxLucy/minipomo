#ifndef MINIPOMO_H
#define MINIPOMO_H

#include <stdbool.h>

#define MINIPOMO_FOCUS_SEC (25 * 60)
#define MINIPOMO_SHORT_BREAK_SEC (5 * 60)
#define MINIPOMO_LONG_BREAK_SEC (15 * 60)
#define MINIPOMO_LONG_BREAK_EVERY 4

#define MINIPOMO_TASKS_MAX 32
#define MINIPOMO_TITLE_MAX 128
#define MINIPOMO_NOTE_MAX 512
#define MINIPOMO_ESTIMATE_MAX 99
#define MINIPOMO_KEY_MAX 16
#define MINIPOMO_NONE (-1)

// escaping at most doubles the text; 256 bytes covers keys and numbers
#define MINIPOMO_SAVE_MAX       \
    (256 + MINIPOMO_TASKS_MAX * \
               (2 * (MINIPOMO_TITLE_MAX + MINIPOMO_NOTE_MAX) + 256))

#define MINIPOMO_TEXT_FOCUS "Time to focus!"
#define MINIPOMO_TEXT_BREAK "Time for a break!"
#define MINIPOMO_TEXT_EMPTY "Add a task to start"

enum minipomo_type {
    MINIPOMO_FOCUS,
    MINIPOMO_SHORT_BREAK,
    MINIPOMO_LONG_BREAK,
    MINIPOMO_TYPE_COUNT,
};

extern const char *const MINIPOMO_TYPE_NAME[MINIPOMO_TYPE_COUNT];

struct minipomo_task {
    char title[MINIPOMO_TITLE_MAX];
    char note[MINIPOMO_NOTE_MAX];
    int estimate;
    int pomodoros;
    bool done;
    double focus_sec;
    enum minipomo_type type;
    bool running;
    double deadline;
    double remaining;
};

struct minipomo {
    struct minipomo_task tasks[MINIPOMO_TASKS_MAX];
    int task_count;
    int current;
};

struct minipomo_stat {
    int pomodoros;
    int estimate;
    double focus_sec;
    int planned_sec;
};

void minipomo_init(struct minipomo *p);
int minipomo_add(struct minipomo *p, const char *title, const char *note,
                 int estimate);
void minipomo_edit(struct minipomo *p, int i, const char *title,
                   const char *note, int estimate);
void minipomo_remove(struct minipomo *p, int i);
void minipomo_move(struct minipomo *p, int from, int to);
void minipomo_play(struct minipomo *p, int i, double now);
void minipomo_start(struct minipomo *p, double now);
void minipomo_pause(struct minipomo *p, double now);
void minipomo_skip(struct minipomo *p, double now);
void minipomo_set_type(struct minipomo *p, enum minipomo_type type, double now);
void minipomo_mark_done(struct minipomo *p, int i, bool done, double now);
bool minipomo_update(struct minipomo *p, double now);
void minipomo_reset_stat(struct minipomo *p);

bool minipomo_running(const struct minipomo *p);
enum minipomo_type minipomo_current_type(const struct minipomo *p);
int minipomo_seconds_left(const struct minipomo *p, double now);
double minipomo_fraction_left(const struct minipomo *p, double now);
const char *minipomo_message(const struct minipomo *p);
double minipomo_focus_sec(const struct minipomo_task *t, double now);
struct minipomo_stat minipomo_stat(const struct minipomo *p, double now);

int minipomo_save(const struct minipomo *p, char *out);
void minipomo_load(struct minipomo *p, const char *in, int len);

int minipomo_text_len(const char *s);
char *minipomo_text_put(char *dst, const char *src);
void minipomo_text_copy(char *dst, int cap, const char *src, int len);
char *minipomo_format_int(char *dst, long long n);
char *minipomo_format_ratio(char *dst, int done, int total);
char *minipomo_format_mmss(char *dst, int sec);
char *minipomo_format_duration(char *dst, int sec);

#endif
