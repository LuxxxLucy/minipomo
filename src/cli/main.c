#define _DEFAULT_SOURCE
#include "core/save.h"
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "cli/config.h"
#include "app/display.h"
#include "core/save.h"
#include "core/text.h"

#define ESC "\x1b["
#define RESET ESC "0m"
#define BOLD ESC "1m"
#define DIM ESC "2m"
#define STRIKE ESC "9m"
#define KEY_ESC 0x1b
#define NO_KEY (-1)

extern char **environ;

struct rgb {
    int r, g, b;
};

static const struct rgb TYPE_RGB[MINIPOMO_PHASE_COUNT] = {
    RGB_FOCUS,
    RGB_SHORT_BREAK,
    RGB_LONG_BREAK,
};
static const char *const DIGIT[COLON_GLYPH + 1][CLOCK_ROWS] = DIGIT_GLYPHS;

static struct minipomo pomo;
static int selected;
static bool unsaved;
static char save_path[PATH_MAX];
static char save_text[MINIPOMO_SAVE_MAX];
static struct termios cooked;
static volatile sig_atomic_t quit;

static int64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void flush(void)
{
    if (fflush(stdout) != 0) {
        quit = 1;
    }
}

static void fg(struct rgb c)
{
    printf(ESC "38;2;%d;%d;%dm", c.r, c.g, c.b);
}

static void move_to(int row, int col)
{
    printf(ESC "%d;%dH", row, col);
}

static int cell_width(unsigned c)
{
    return (c >= 0x1100 && c <= 0x115F) || (c >= 0x2E80 && c <= 0xA4CF) ||
                   (c >= 0xAC00 && c <= 0xD7A3) ||
                   (c >= 0xF900 && c <= 0xFAFF) ||
                   (c >= 0xFE30 && c <= 0xFE4F) ||
                   (c >= 0xFF00 && c <= 0xFF60) ||
                   (c >= 0xFFE0 && c <= 0xFFE6) || c >= 0x20000
               ? 2
               : 1;
}

static const char *fit_line(const char *s, int width, int *used)
{
    *used = 0;
    while (*s && *s != '\n') {
        const unsigned char *u = (const unsigned char *)s;
        int n = u[0] < 0x80 ? 1 : u[0] < 0xE0 ? 2 : u[0] < 0xF0 ? 3 : 4;
        unsigned c = n == 1 ? u[0] : u[0] & (0x7F >> n);
        int i = 1;
        for (; i < n && u[i]; i++) {
            c = (c << 6) | (u[i] & 0x3F);
        }
        int w = cell_width(c);
        if (*used + w > width) {
            break;
        }
        *used += w;
        s += i;
    }
    return s;
}

static void print_padded(const char *s, int width)
{
    int used;
    const char *end = fit_line(s, width, &used);
    printf("%.*s%*s", (int)(end - s), s, width - used, "");
}

static int load(void)
{
    minipomo_init(&pomo);
    const char *env = getenv(STATE_ENV);
    const char *home = getenv("HOME");
    int size = env ? snprintf(save_path, sizeof save_path, "%s", env)
                   : snprintf(save_path, sizeof save_path, "%s/" STATE_FILE,
                              home ? home : ".");
    if (size < 0 || size >= (int)sizeof save_path) {
        return -1;
    }
    FILE *f = fopen(save_path, "r");
    if (!f) {
        return errno == ENOENT ? 0 : -1;
    }
    int len = (int)fread(save_text, 1, sizeof save_text, f);
    bool failed = fgetc(f) != EOF || ferror(f);
    if (fclose(f) != 0 || failed) {
        return -1;
    }
    return minipomo_load(&pomo, save_text, len);
}

static int save(void)
{
    if (!unsaved) {
        return 0;
    }
    int len = minipomo_save(&pomo, save_text, sizeof save_text);
    char temporary[PATH_MAX];
    int size = snprintf(temporary, sizeof temporary, "%s.XXXXXX", save_path);
    if (len < 0 || size < 0 || size >= (int)sizeof temporary) {
        return -1;
    }
    int fd = mkstemp(temporary);
    if (fd < 0) {
        return -1;
    }
    FILE *f = fdopen(fd, "w");
    if (!f) {
        close(fd);
        unlink(temporary);
        return -1;
    }
    bool failed = fwrite(save_text, 1, len, f) != (size_t)len;
    if (fclose(f) != 0) {
        failed = true;
    }
    if (failed || rename(temporary, save_path) != 0) {
        unlink(temporary);
        return -1;
    }
    unsaved = false;
    return 0;
}

static void notify(const char *message)
{
#ifdef __APPLE__
    char *argv[] = { "osascript",
                     "-e",
                     "on run argv",
                     "-e",
                     "display notification (item 1 of argv) with title "
                     "\"" TEXT_APP "\" sound name \"" NOTIFY_SOUND "\"",
                     "-e",
                     "end run",
                     (char *)message,
                     NULL };
#else
    char *argv[] = { "notify-send", TEXT_APP, (char *)message, NULL };
#endif
    pid_t pid;
    posix_spawnp(&pid, argv[0], NULL, NULL, argv, environ);
}

static void completed(struct minipomo_result result)
{
    if (result.completed.phase != MINIPOMO_NO_COMPLETION) {
        unsaved = true;
        notify(app_completion_message(&result.completed));
    }
}

static struct minipomo_result modify(struct minipomo_change change, int64_t now)
{
    struct minipomo_result result = minipomo_modify(&pomo, change, now);
    unsaved |= result.status == 0;
    completed(result);
    return result;
}

static void raw_mode(void)
{
    struct termios t = cooked;
    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
    printf(ESC "?25l");
}

static void cooked_mode(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &cooked);
    printf(ESC "?25h");
}

static void on_signal(int sig)
{
    (void)sig;
    quit = 1;
}

static void draw_clock(int row, int col, const char *mmss)
{
    for (int y = 0; y < CLOCK_ROWS; y++) {
        move_to(row + y, col);
        for (const char *c = mmss; *c; c++) {
            const char *glyph = DIGIT[*c == ':' ? COLON_GLYPH : *c - '0'][y];
            for (const char *g = glyph; *g; g++) {
                printf(*g == '#' ? ICON_BLOCK : "  ");
            }
            printf("  ");
        }
    }
}

static void draw_task(int row, int col, int width, int i, struct rgb color)
{
    const struct minipomo_task *t = &pomo.tasks[i];
    char focus[COUNT_LEN], count[COUNT_LEN];
    minipomo_format_duration(focus, t->focus_ms / 1000);
    minipomo_format_ratio(count, t->pomodoros, t->estimate);

    move_to(row, col - MARK_W);
    printf(i == selected ? ICON_SELECT : "  ");
    fg(color);
    printf(i == pomo.current ? ICON_EDGE : "  ");
    printf(t->done ? ICON_CHECK RESET DIM STRIKE : RESET ICON_OPEN BOLD);
    print_padded(t->title, width - ROW_FIXED_W);
    printf(RESET DIM "%7s %5s " RESET, focus, count);
    printf(!minipomo_get_can_start_task(&pomo, i) ? "  "
           : t->timer.running                     ? ICON_PAUSE
                                                  : ICON_PLAY);
}

static void draw(int64_t now)
{
    struct winsize ws = { 0 };
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    int rows = ws.ws_row ? ws.ws_row : DEFAULT_ROWS;
    int cols = ws.ws_col ? ws.ws_col : DEFAULT_COLS;
    int width =
        cols - PAGE_MARGIN < LINE_MAX_W ? cols - PAGE_MARGIN : LINE_MAX_W;
    int left = (cols - width) / 2 + 1;
    enum minipomo_phase type = minipomo_get_timer(&pomo).phase;
    struct rgb color = TYPE_RGB[type];

    printf(RESET ESC "2J");
    move_to(1, left);
    printf(BOLD TEXT_APP RESET);

    move_to(2, left);
    int filled = (int)((1 - app_fraction_left(&pomo)) * width);
    fg(color);
    for (int x = 0; x < width; x++) {
        printf(x == filled ? RESET DIM ICON_TRACK : ICON_TRACK);
    }
    printf(RESET);

    char mmss[CLOCK_LEN];
    minipomo_format_mmss(mmss, app_seconds_left(&pomo));
    fg(color);
    draw_clock(CLOCK_TOP, (cols - CLOCK_COLS) / 2 + 1, mmss);
    printf(RESET);

    const char *button =
        minipomo_get_timer(&pomo).running ? TEXT_PAUSE : TEXT_START;
    move_to(BUTTON_ROW, (cols - (int)strlen(button)) / 2 + 1);
    if (minipomo_get_timer(&pomo).running ||
        minipomo_get_timer(&pomo).can_start) {
        fg(color);
        printf(BOLD "%s" RESET, button);
    } else {
        printf(DIM "%s" RESET, button);
    }

    int tabs_width = -TAB_GAP;
    for (int k = 0; k < MINIPOMO_PHASE_COUNT; k++) {
        tabs_width += (int)strlen(APP_PHASE_NAMES[k]) + TAB_KEY + TAB_GAP;
    }
    move_to(TABS_ROW, (cols - tabs_width) / 2 + 1);
    for (int k = 0; k < MINIPOMO_PHASE_COUNT; k++) {
        if (k == (int)type) {
            fg(color);
            printf(BOLD);
        } else {
            printf(DIM);
        }
        printf("%d %s" RESET "%*s", k + 1, APP_PHASE_NAMES[k], TAB_GAP, "");
    }

    const char *message = app_message(&pomo);
    int message_width;
    const char *end = fit_line(message, width, &message_width);
    move_to(MESSAGE_ROW, (cols - message_width) / 2 + 1);
    printf("%.*s", (int)(end - message), message);

    char stat[STAT_LEN], *s = stat;
    if (pomo.task_count) {
        struct minipomo_stats st = minipomo_get_stats(&pomo);
        time_t finish = (time_t)(now / 1000) + (st.planned_ms / 1000);
        struct tm tm;
        localtime_r(&finish, &tm);
        s = minipomo_format_ratio(minipomo_text_put(s, TEXT_POMOS),
                                  st.pomodoros, st.estimate);
        s = minipomo_format_duration(minipomo_text_put(s, TEXT_SPENT),
                                     st.focus_ms / 1000);
        s = minipomo_format_mmss(minipomo_text_put(s, TEXT_FINISH),
                                 tm.tm_hour * 60 + tm.tm_min);
    }
    move_to(LIST_ROW, left);
    printf(BOLD TEXT_TASKS RESET);
    move_to(LIST_ROW, left + width - (int)(s - stat));
    printf(DIM "%.*s", (int)(s - stat), stat);
    move_to(LIST_ROW + 1, left);
    for (int x = 0; x < width; x++) {
        printf(ICON_RULE);
    }
    printf(RESET);

    int visible = rows - (LIST_ROW + 2) - LIST_FOOT;
    visible = visible < 1 ? 1 : visible;
    int first = selected >= visible ? selected - visible + 1 : 0;
    int row = LIST_ROW + 2;
    for (int i = first; i < pomo.task_count && i < first + visible; i++) {
        draw_task(row++, left, width, i, color);
    }
    move_to(row + 1, left);
    printf(DIM TEXT_ADD);

    move_to(rows, left);
    print_padded(TEXT_HELP, width);
    printf(RESET);
    flush();
}

static void ask(const char *label, const char *fallback, char *dst, int cap)
{
    move_to(LAST_ROW, 1);
    printf(RESET ESC "2K%s", label);
    if (fallback[0]) {
        printf(" [%s]", fallback);
    }
    printf(": ");
    cooked_mode();
    flush();
    char line[MINIPOMO_NOTE_MAX];
    struct pollfd input = { .fd = STDIN_FILENO, .events = POLLIN };
    while (!quit) {
        int ready = poll(&input, 1, POLL_MS);
        completed(minipomo_update(&pomo, now_ms()));
        save();
        if (ready > 0 || (ready < 0 && errno != EINTR)) {
            break;
        }
    }
    if (quit || !fgets(line, sizeof line, stdin)) {
        line[0] = '\0';
    } else if (!strchr(line, '\n')) {
        for (int c = getchar(); c != '\n' && c != EOF; c = getchar()) {
        }
    }
    line[strcspn(line, "\n")] = '\0';
    const char *src = line[0] ? line : fallback;
    minipomo_text_copy(dst, cap, src, (int)strlen(src));
    raw_mode();
}

static bool confirm(const char *label)
{
    char answer[ANSWER_LEN];
    ask(label, "", answer, sizeof answer);
    return answer[0] == 'y' || answer[0] == 'Y';
}

static void edit_task(int i)
{
    const struct minipomo_task *t = i == MINIPOMO_NONE ? NULL : &pomo.tasks[i];
    char title[MINIPOMO_TITLE_MAX], note[MINIPOMO_NOTE_MAX];
    char estimate[ESTIMATE_LEN], old_estimate[ESTIMATE_LEN];
    minipomo_format_int(old_estimate, t ? t->estimate : 1);
    ask(TEXT_TASK, t ? t->title : "", title, sizeof title);
    if (!title[0]) {
        return;
    }
    ask(TEXT_ESTIMATE, old_estimate, estimate, sizeof estimate);
    ask(TEXT_NOTE, t ? t->note : "", note, sizeof note);
    char *end;
    errno = 0;
    long count = strtol(estimate, &end, 10);
    if (errno || end == estimate || *end || count < 1 ||
        count > MINIPOMO_ESTIMATE_MAX) {
        char answer[ANSWER_LEN];
        ask("Estimate must be between 1 and 99. Press Enter", "", answer,
            sizeof answer);
        return;
    }
    struct minipomo_details details = { title, note, (int)count };
    struct minipomo_change change = { .type =
                                          t ? MINIPOMO_EDIT : MINIPOMO_ADD };
    if (t) {
        change.edit.task = i;
        change.edit.details = details;
    } else {
        change.add = details;
    }
    struct minipomo_result result = modify(change, now_ms());
    if (result.added_task != MINIPOMO_NONE) {
        selected = result.added_task;
    }
}

static int read_key(void)
{
    unsigned char c, seq[2];
    struct pollfd in = { .fd = STDIN_FILENO, .events = POLLIN };
    if (read(STDIN_FILENO, &c, 1) != 1) {
        return NO_KEY;
    }
    if (c != KEY_ESC) {
        return c;
    }
    if (poll(&in, 1, ESC_WAIT_MS) <= 0 || read(STDIN_FILENO, seq, 2) != 2 ||
        seq[0] != '[') {
        return NO_KEY;
    }
    return seq[1] == 'A' ? 'k' : seq[1] == 'B' ? 'j' : NO_KEY;
}

static void on_key(int key, int64_t now)
{
    bool on_task = selected < pomo.task_count;
    switch (key) {
        case ' ':
            modify(
                (struct minipomo_change){ .type =
                                              minipomo_get_timer(&pomo).running
                                                  ? MINIPOMO_PAUSE
                                                  : MINIPOMO_START_CURRENT },
                now);
            break;
        case 's':
            modify((struct minipomo_change){ .type = MINIPOMO_SKIP }, now);
            break;
        case '1':
        case '2':
        case '3':
            modify((struct minipomo_change){ .type = MINIPOMO_SET_PHASE,
                                             .phase = key - '1' },
                   now);
            break;
        case 'k':
            selected -= selected > 0;
            break;
        case 'j':
            selected += selected < pomo.task_count - 1;
            break;
        case 'K':
            if (on_task && selected > 0) {
                modify((struct minipomo_change){ .type = MINIPOMO_MOVE,
                                                 .move = { selected,
                                                           selected - 1 } },
                       now);
                selected--;
            }
            break;
        case 'J':
            if (on_task && selected < pomo.task_count - 1) {
                modify((struct minipomo_change){ .type = MINIPOMO_MOVE,
                                                 .move = { selected,
                                                           selected + 1 } },
                       now);
                selected++;
            }
            break;
        case '\n':
        case 'p':
            if (on_task) {
                modify(
                    (struct minipomo_change){
                        .type = pomo.tasks[selected].timer.running
                                    ? MINIPOMO_PAUSE
                                    : MINIPOMO_START_TASK,
                        .start_task = selected },
                    now);
            }
            break;
        case 'x':
            if (on_task) {
                modify(
                    (struct minipomo_change){
                        .type = MINIPOMO_SET_DONE,
                        .set_done = { selected, !pomo.tasks[selected].done } },
                    now);
            }
            break;
        case 'a':
            edit_task(MINIPOMO_NONE);
            break;
        case 'e':
            if (on_task) {
                edit_task(selected);
            }
            break;
        case 'd':
            if (on_task && confirm(TEXT_DELETE)) {
                modify((struct minipomo_change){ .type = MINIPOMO_REMOVE,
                                                 .remove = selected },
                       now_ms());
                selected -= selected > 0 && selected == pomo.task_count;
            }
            break;
        case 'c':
            if (confirm(TEXT_CLEAR)) {
                modify((struct minipomo_change){ .type = MINIPOMO_CLEAR },
                       now_ms());
                selected = 0;
            }
            break;
        case 'q':
            quit = 1;
            break;
    }
}

int main(void)
{
    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, &cooked) != 0) {
        fprintf(stderr, TEXT_NO_TTY);
        return 1;
    }
    if (load() != 0) {
        fprintf(stderr, "Cannot load MiniPomo state: %s\n", save_path);
        return 1;
    }
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGCHLD, SIG_IGN);
    setvbuf(stdout, NULL, _IOFBF, OUT_BUF);
    printf(ESC "?1049h");
    raw_mode();

    struct pollfd in = { .fd = STDIN_FILENO, .events = POLLIN };
    while (!quit) {
        int key = poll(&in, 1, POLL_MS) > 0 ? read_key() : NO_KEY;
        if (key != NO_KEY) {
            on_key(key, now_ms());
        }
        int64_t now = now_ms();
        completed(minipomo_update(&pomo, now));
        save();
        draw(now);
    }
    int saved = save();
    cooked_mode();
    printf(RESET ESC "2J" ESC "?1049l");
    flush();
    if (saved != 0) {
        fprintf(stderr, "Cannot save MiniPomo state: %s\n", save_path);
    }
    return saved != 0;
}
