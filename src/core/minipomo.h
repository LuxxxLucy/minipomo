#ifndef MINIPOMO_H
#define MINIPOMO_H

#include <stdbool.h>
#include <stdint.h>

#define MINIPOMO_FOCUS_MS (25 * 60 * 1000)
#define MINIPOMO_SHORT_BREAK_MS (5 * 60 * 1000)
#define MINIPOMO_LONG_BREAK_MS (15 * 60 * 1000)
#define MINIPOMO_LONG_BREAK_EVERY 4
#define MINIPOMO_TASKS_MAX 32
#define MINIPOMO_TITLE_MAX 128
#define MINIPOMO_NOTE_MAX 512
#define MINIPOMO_ESTIMATE_MAX 99
#define MINIPOMO_TIME_MAX (INT64_MAX / MINIPOMO_TASKS_MAX)
#define MINIPOMO_NONE (-1)

enum minipomo_phase {
    MINIPOMO_FOCUS,
    MINIPOMO_SHORT_BREAK,
    MINIPOMO_LONG_BREAK,
    MINIPOMO_PHASE_COUNT,
    MINIPOMO_NO_COMPLETION = MINIPOMO_PHASE_COUNT,
};

struct minipomo_timer {
    enum minipomo_phase phase;
    int64_t remaining_ms;
    bool running;
};

struct minipomo_task {
    char title[MINIPOMO_TITLE_MAX];
    char note[MINIPOMO_NOTE_MAX];
    int estimate;
    int pomodoros;
    bool done;
    int64_t focus_ms;
    struct minipomo_timer timer;
};

struct minipomo {
    struct minipomo_task tasks[MINIPOMO_TASKS_MAX];
    int task_count;
    int current;
    int64_t updated_at_ms;
};

enum minipomo_change_type {
    MINIPOMO_ADD,
    MINIPOMO_EDIT,
    MINIPOMO_REMOVE,
    MINIPOMO_MOVE,
    MINIPOMO_START_CURRENT,
    MINIPOMO_START_TASK,
    MINIPOMO_PAUSE,
    MINIPOMO_SKIP,
    MINIPOMO_SET_PHASE,
    MINIPOMO_SET_DONE,
    MINIPOMO_CLEAR,
};

struct minipomo_details {
    const char *title;
    const char *note;
    int estimate;
};

struct minipomo_change {
    enum minipomo_change_type type;
    union {
        struct minipomo_details add;
        struct {
            int task;
            struct minipomo_details details;
        } edit;
        int remove;
        struct {
            int from;
            int to;
        } move;
        int start_task;
        enum minipomo_phase phase;
        struct {
            int task;
            bool done;
        } set_done;
    };
};

struct minipomo_completion {
    enum minipomo_phase phase;
    char title[MINIPOMO_TITLE_MAX];
};

struct minipomo_result {
    int status;
    int added_task;
    struct minipomo_completion completed;
};

struct minipomo_timer_view {
    enum minipomo_phase phase;
    int64_t remaining_ms;
    int64_t duration_ms;
    bool running;
    bool can_start;
};

struct minipomo_stats {
    int64_t pomodoros;
    int estimate;
    int64_t focus_ms;
    int64_t planned_ms;
};

void minipomo_init(struct minipomo *state);
struct minipomo_result minipomo_update(struct minipomo *state, int64_t now_ms);
struct minipomo_result minipomo_modify(struct minipomo *state,
                                       struct minipomo_change change,
                                       int64_t now_ms);
struct minipomo_timer_view minipomo_get_timer(const struct minipomo *state);
bool minipomo_get_can_start_task(const struct minipomo *state, int task);
struct minipomo_stats minipomo_get_stats(const struct minipomo *state);

#endif
