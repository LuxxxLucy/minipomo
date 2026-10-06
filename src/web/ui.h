#ifndef UI_H
#define UI_H

#include "clay.h"
#include "core/minipomo.h"
#include "web/config.h"

#define NO_ROW (-1)

enum cmd {
    CMD_NONE,
    CMD_START,
    CMD_PAUSE,
    CMD_SKIP,
    CMD_SET_TYPE,
    CMD_PLAY,
    CMD_MARK_DONE,
    CMD_EDIT,
    CMD_ADD,
    CMD_ESTIMATE_UP,
    CMD_ESTIMATE_DOWN,
    CMD_NOTE,
    CMD_SAVE,
    CMD_CANCEL,
    CMD_DELETE,
    CMD_CLEAR,
};

enum draw_flag { FLAG_STRIKE = 1, FLAG_DASHED = 2 };

struct form {
    bool open;
    int index;
    int estimate;
    bool note_open;
    char title[MINIPOMO_TITLE_MAX];
    char note[MINIPOMO_NOTE_MAX];
};

struct view {
    const struct minipomo *pomo;
    const struct form *form;
    double now;
    float window_height;
    int minute_of_day;
    bool pointer_down;
    int dragged_row;
};

Clay_ElementId ui_click(enum cmd c, int arg);
void ui_link(const char *s, enum cmd c, int arg, Clay_Color normal,
             Clay_Color hover);
Clay_String ui_str(const char *s);
Clay_TextElementConfig ui_text(int font, int size, Clay_Color color);
void ui_gap(float w, float h);
void ui_grow(void);
Clay_Color ui_type_color(const struct minipomo *p);

void page(const struct view *v);
void timer_view(const struct view *v);
void task_view(const struct view *v);

#endif
