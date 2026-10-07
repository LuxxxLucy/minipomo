#include "web/ui.h"

static char mmss[CLOCK_LEN];

static void type_tab(const struct view *v, enum minipomo_phase type)
{
    Clay_Color white = COL_WHITE;
    Clay_Color dim = COL_DIM;
    CLAY(ui_click(CMD_SET_TYPE, type), {
        .layout = {
            .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(TAB_H) },
            .padding = { TAB_PAD_X, TAB_PAD_X, 0, 0 },
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
        },
    })
    {
        bool lit = Clay_Hovered() || type == minipomo_get_timer(v->pomo).phase;
        CLAY_TEXT(ui_str(APP_PHASE_NAMES[type]),
                  ui_text(FONT_PLAIN, SIZE_TAB, lit ? white : dim));
    }
}

static void start_button(const struct view *v)
{
    bool running = minipomo_get_timer(v->pomo).running;
    bool enabled = running || minipomo_get_timer(v->pomo).can_start;
    enum cmd c = !enabled ? CMD_NONE : running ? CMD_PAUSE : CMD_START;
    CLAY(ui_click(c, 0), {
        .layout = {
            .sizing = { CLAY_SIZING_FIXED(BUTTON_W),
                        CLAY_SIZING_FIXED(BUTTON_H + BUTTON_DROP) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    })
    {
        bool pressed =
            running || (enabled && Clay_Hovered() && v->pointer_down);
        if (pressed) {
            ui_gap(BUTTON_W, BUTTON_DROP);
        }
        CLAY_AUTO_ID({
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(BUTTON_H) },
                .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = COL_WHITE,
            .cornerRadius = CLAY_CORNER_RADIUS(BUTTON_RADIUS),
        })
        {
            Clay_Color label =
                enabled ? ui_type_color(v->pomo) : (Clay_Color)COL_MUTED;
            CLAY_TEXT(ui_str(running ? TEXT_PAUSE : TEXT_START),
                      ui_text(FONT_ROUNDED, SIZE_BUTTON, label));
        }
        if (!pressed) {
            CLAY_AUTO_ID({
                .layout.sizing = { CLAY_SIZING_GROW(0),
                                   CLAY_SIZING_FIXED(BUTTON_DROP) },
                .backgroundColor = COL_BUTTON_SHADOW,
                .cornerRadius = { 0, 0, BUTTON_RADIUS, BUTTON_RADIUS },
            })
            {
            }
        }
    }
}

static void skip_button(void)
{
    CLAY(ui_click(CMD_SKIP, 0), {
        .layout = {
            .sizing = { CLAY_SIZING_FIXED(SKIP_W), CLAY_SIZING_FIXED(BUTTON_H) },
            .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
        },
    })
    {
        CLAY_TEXT(CLAY_STRING(TEXT_SKIP),
                  ui_text(FONT_PLAIN, SIZE_TAB,
                          Clay_Hovered() ? (Clay_Color)COL_WHITE
                                         : (Clay_Color)COL_DIM));
    }
}

void timer_view(const struct view *v)
{
    Clay_Color white = COL_WHITE;
    minipomo_format_mmss(mmss, app_seconds_left(v->pomo));

    CLAY_AUTO_ID({
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0, PAGE_W), CLAY_SIZING_FIT(0) },
            .padding = { 0, 0, CARD_PAD_TOP, CARD_PAD_BOTTOM },
            .childGap = CLOCK_GAP,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = COL_CARD,
        .cornerRadius = CLAY_CORNER_RADIUS(CARD_RADIUS),
    })
    {
        CLAY_TEXT(ui_str(mmss), ui_text(FONT_ROUNDED, SIZE_CLOCK, white));
        CLAY_AUTO_ID({ .layout.childAlignment = { .y = CLAY_ALIGN_Y_CENTER } })
        {
            ui_gap(SKIP_W, 0);
            start_button(v);
            if (minipomo_get_timer(v->pomo).running) {
                skip_button();
            } else {
                ui_gap(SKIP_W, 0);
            }
        }
        CLAY_AUTO_ID({ .layout.childGap = TAB_GAP })
        {
            for (int type = 0; type < MINIPOMO_PHASE_COUNT; type++) {
                type_tab(v, type);
            }
        }
    }
    ui_gap(0, MESSAGE_GAP);
    CLAY_TEXT(ui_str(app_message(v->pomo)),
              ui_text(FONT_PLAIN, SIZE_MESSAGE, white));
}
