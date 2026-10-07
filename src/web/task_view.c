#include "web/ui.h"

static char count_texts[MINIPOMO_TASKS_MAX][COUNT_LEN];
static char focus_texts[MINIPOMO_TASKS_MAX][COUNT_LEN];
static char estimate_text[COUNT_LEN];
static char pomodoros_text[STAT_LEN];
static char focus_text[STAT_LEN];
static char finish_text[STAT_LEN];

static void list_head(const struct view *v)
{
    CLAY_AUTO_ID({
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(LIST_HEAD_H) },
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
        },
        .border = { COL_DIM, { .bottom = LINE_H } },
    })
    {
        CLAY_TEXT(CLAY_STRING(TEXT_TASKS),
                  ui_text(FONT_ROUNDED, SIZE_LIST, (Clay_Color)COL_WHITE));
        ui_grow();
        if (v->pomo->task_count) {
            ui_link(TEXT_CLEAR, CMD_CLEAR, 0, (Clay_Color)COL_DIM,
                    (Clay_Color)COL_WHITE);
        }
    }
}

static void icon(const char *glyph, enum cmd c, int arg, bool framed)
{
    CLAY(ui_click(c, arg), {
        .layout = {
            .sizing = { CLAY_SIZING_FIXED(ICON_SIZE), CLAY_SIZING_FIXED(ICON_SIZE) },
            .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
        },
        .backgroundColor = Clay_Hovered() ? (Clay_Color)COL_EDGE
                                          : (Clay_Color)COL_WHITE,
        .cornerRadius = CLAY_CORNER_RADIUS(ROW_RADIUS),
        .border = { COL_EDGE, CLAY_BORDER_OUTSIDE(framed) },
    })
    {
        CLAY_TEXT(ui_str(glyph),
                  ui_text(FONT_PLAIN, SIZE_TASK, (Clay_Color)COL_INK));
    }
}

static void task_row(const struct view *v, int i)
{
    const struct minipomo_task *t = &v->pomo->tasks[i];
    minipomo_format_ratio(count_texts[i], t->pomodoros, t->estimate);
    minipomo_format_duration(focus_texts[i], t->focus_ms / 1000);

    CLAY(CLAY_IDI(ID_ROW, i),
         {
             .backgroundColor = COL_WHITE,
             .cornerRadius = CLAY_CORNER_RADIUS(ROW_RADIUS),
             .layout.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
         })
    {
        bool hovered = Clay_Hovered() || v->dragged_row == i;
        Clay_Color edge = i == v->pomo->current ? (Clay_Color)COL_DARK
                          : hovered             ? (Clay_Color)COL_EDGE
                                                : (Clay_Color)COL_WHITE;
        CLAY_AUTO_ID({
            .layout.sizing = { CLAY_SIZING_FIXED(ROW_EDGE),
                               CLAY_SIZING_GROW(0) },
            .backgroundColor = edge,
            .cornerRadius = { ROW_RADIUS, 0, ROW_RADIUS, 0 },
        })
        {
        }
        CLAY_AUTO_ID({
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding = { ROW_PAD, ROW_PAD, ROW_PAD_Y, ROW_PAD_Y },
                .childGap = NOTE_PAD,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        })
        {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                    .childGap = ROW_PAD,
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                },
            })
            {
                CLAY(ui_click(t->done ? CMD_MARK_UNDONE : CMD_MARK_DONE, i), {
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(CHECK_SIZE),
                                    CLAY_SIZING_FIXED(CHECK_SIZE) },
                        .childAlignment = { CLAY_ALIGN_X_CENTER,
                                            CLAY_ALIGN_Y_CENTER },
                    },
                    .backgroundColor = Clay_Hovered()
                                           ? (Clay_Color)COL_MUTED
                                       : t->done ? ui_type_color(v->pomo)
                                                 : (Clay_Color)COL_EDGE,
                    .cornerRadius = CLAY_CORNER_RADIUS(CHECK_SIZE / 2),
                })
                {
                    CLAY_TEXT(CLAY_STRING(ICON_CHECK),
                              ui_text(FONT_ROUNDED, SIZE_TASK,
                                      (Clay_Color)COL_WHITE));
                }
                CLAY_AUTO_ID({ .layout.sizing.width = CLAY_SIZING_GROW(0) })
                {
                    Clay_TextElementConfig c = ui_text(
                        FONT_ROUNDED, SIZE_TASK,
                        t->done ? (Clay_Color)COL_MUTED : (Clay_Color)COL_INK);
                    c.userData = (void *)(t->done ? FLAG_STRIKE : 0);
                    c.lineHeight = SIZE_TASK + LINE_GAP;
                    CLAY_TEXT(ui_str(t->title), c);
                }
                CLAY_TEXT(
                    ui_str(focus_texts[i]),
                    ui_text(FONT_PLAIN, SIZE_NOTE, (Clay_Color)COL_MUTED));
                CLAY_TEXT(
                    ui_str(count_texts[i]),
                    ui_text(FONT_ROUNDED, SIZE_COUNT, (Clay_Color)COL_MUTED));
                if (minipomo_get_can_start_task(v->pomo, i)) {
                    icon(t->timer.running ? ICON_PAUSE : ICON_PLAY,
                         t->timer.running ? CMD_PAUSE_TASK : CMD_START_TASK, i,
                         false);
                } else {
                    ui_gap(ICON_SIZE, ICON_SIZE);
                }
                icon(ICON_MORE, CMD_EDIT, i, true);
            }
            if (t->note[0]) {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                        .padding = CLAY_PADDING_ALL(NOTE_PAD),
                    },
                    .backgroundColor = COL_NOTE,
                    .cornerRadius = CLAY_CORNER_RADIUS(NOTE_RADIUS),
                })
                {
                    Clay_TextElementConfig c = ui_text(
                        FONT_PLAIN, SIZE_NOTE, (Clay_Color)COL_NOTE_INK);
                    c.lineHeight = SIZE_NOTE + LINE_GAP;
                    CLAY_TEXT(ui_str(t->note), c);
                }
            }
        }
    }
}

static void form_view(const struct view *v)
{
    const struct form *f = v->form;
    Clay_Color ink = COL_INK;
    minipomo_format_int(estimate_text, f->estimate);

    CLAY_AUTO_ID({
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = COL_WHITE,
        .cornerRadius = CLAY_CORNER_RADIUS(FORM_RADIUS),
    })
    {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(FORM_PAD),
                .childGap = FORM_GAP,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        })
        {
            CLAY(CLAY_ID(ID_FIELD_TITLE),
                 {
                     .layout.sizing = { CLAY_SIZING_GROW(0),
                                        CLAY_SIZING_FIXED(TITLE_FIELD_H) },
                 })
            {
            }
            CLAY_TEXT(CLAY_STRING(TEXT_ESTIMATE),
                      ui_text(FONT_ROUNDED, SIZE_TASK, ink));
            CLAY_AUTO_ID({
                .layout = { .childGap = FORM_GAP / 2,
                            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
            })
            {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(ESTIMATE_W),
                                    CLAY_SIZING_FIXED(ESTIMATE_H) },
                        .padding = { NOTE_PAD, NOTE_PAD, 0, 0 },
                        .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                    },
                    .backgroundColor = COL_FIELD,
                    .cornerRadius = CLAY_CORNER_RADIUS(FIELD_RADIUS),
                })
                {
                    CLAY_TEXT(ui_str(estimate_text),
                              ui_text(FONT_ROUNDED, SIZE_TASK, ink));
                }
                icon(ICON_UP, CMD_ESTIMATE_UP, 0, true);
                icon(ICON_DOWN, CMD_ESTIMATE_DOWN, 0, true);
            }
            if (f->note_open) {
                CLAY(CLAY_ID(ID_FIELD_NOTE),
                     {
                         .layout.sizing = { CLAY_SIZING_GROW(0),
                                            CLAY_SIZING_FIXED(NOTE_FIELD_H) },
                         .backgroundColor = COL_FIELD,
                         .cornerRadius = CLAY_CORNER_RADIUS(FIELD_RADIUS),
                     })
                {
                }
            } else {
                ui_link(TEXT_ADD_NOTE, CMD_NOTE, 0, (Clay_Color)COL_MUTED, ink);
            }
        }
        CLAY_AUTO_ID({
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding = { FORM_PAD, FORM_PAD, FORM_GAP, FORM_GAP },
                .childGap = FORM_PAD,
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = COL_FOOT,
            .cornerRadius = { 0, 0, FORM_RADIUS, FORM_RADIUS },
        })
        {
            if (f->index != MINIPOMO_NONE) {
                ui_link(TEXT_DELETE, CMD_DELETE, 0, (Clay_Color)COL_MUTED, ink);
            }
            ui_grow();
            ui_link(TEXT_CANCEL, CMD_CANCEL, 0, (Clay_Color)COL_MUTED, ink);
            CLAY(ui_click(CMD_SAVE, 0),
                 {
                     .layout.padding = { SAVE_PAD_X, SAVE_PAD_X, SAVE_PAD_Y,
                                         SAVE_PAD_Y },
                     .backgroundColor =
                         Clay_Hovered() ? ink : (Clay_Color)COL_DARK,
                     .cornerRadius = CLAY_CORNER_RADIUS(ROW_RADIUS),
                 })
            {
                CLAY_TEXT(
                    CLAY_STRING(TEXT_SAVE),
                    ui_text(FONT_ROUNDED, SIZE_TASK, (Clay_Color)COL_WHITE));
            }
        }
    }
}

static void add_button(void)
{
    CLAY(ui_click(CMD_ADD, 0), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(ADD_H) },
            .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
        },
        .backgroundColor = COL_SHADE,
        .cornerRadius = CLAY_CORNER_RADIUS(ADD_RADIUS),
        .border = { COL_DASH, CLAY_BORDER_OUTSIDE(DASH_W) },
        .userData = (void *)FLAG_DASHED,
    })
    {
        CLAY_TEXT(CLAY_STRING(TEXT_ADD),
                  ui_text(FONT_ROUNDED, SIZE_TASK,
                          Clay_Hovered() ? (Clay_Color)COL_WHITE
                                         : (Clay_Color)COL_SOFT));
    }
}

static void stat_view(const struct view *v)
{
    struct minipomo_stats s = minipomo_get_stats(v->pomo);
    minipomo_format_ratio(pomodoros_text, s.pomodoros, s.estimate);
    minipomo_format_duration(focus_text, s.focus_ms / 1000);
    int tenths_of_hour = ((s.planned_ms / 1000) * 10 + HOUR_SEC / 2) / HOUR_SEC;
    char *p = minipomo_format_mmss(
        finish_text, (v->minute_of_day + (s.planned_ms / 1000) / 60) % DAY_MIN);
    p = minipomo_format_int(minipomo_text_put(p, " ("), tenths_of_hour / 10);
    p = minipomo_format_int(minipomo_text_put(p, "."), tenths_of_hour % 10);
    minipomo_text_put(p, "h)");

    Clay_Color dim = COL_DIM;
    Clay_Color white = COL_WHITE;
    CLAY_AUTO_ID({
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
            .padding = CLAY_PADDING_ALL(STAT_PAD),
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER },
        },
        .backgroundColor = COL_CARD,
        .border = { COL_DIM, { .top = STAT_LINE_H } },
    })
    {
        CLAY_TEXT(CLAY_STRING(TEXT_POMOS), ui_text(FONT_PLAIN, SIZE_TASK, dim));
        CLAY_TEXT(ui_str(pomodoros_text),
                  ui_text(FONT_ROUNDED, SIZE_TASK, white));
        ui_gap(STAT_PAD, 0);
        CLAY_TEXT(CLAY_STRING(TEXT_SPENT), ui_text(FONT_PLAIN, SIZE_TASK, dim));
        CLAY_TEXT(ui_str(focus_text), ui_text(FONT_ROUNDED, SIZE_TASK, white));
        ui_gap(STAT_PAD, 0);
        CLAY_TEXT(CLAY_STRING(TEXT_FINISH),
                  ui_text(FONT_PLAIN, SIZE_TASK, dim));
        CLAY_TEXT(ui_str(finish_text), ui_text(FONT_ROUNDED, SIZE_TASK, white));
    }
}

void task_view(const struct view *v)
{
    const struct minipomo *p = v->pomo;
    const struct form *f = v->form;
    CLAY_AUTO_ID({
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0, PAGE_W), CLAY_SIZING_FIT(0) },
            .childGap = ROW_GAP,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    })
    {
        list_head(v);
        for (int i = 0; i < p->task_count; i++) {
            if (f->open && f->index == i) {
                form_view(v);
            } else {
                task_row(v, i);
            }
        }
        if (f->open && f->index == MINIPOMO_NONE) {
            form_view(v);
        } else if (p->task_count < MINIPOMO_TASKS_MAX) {
            add_button();
        }
        if (p->task_count) {
            stat_view(v);
        }
    }
}
