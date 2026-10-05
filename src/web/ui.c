#include "web/ui.h"

static const Clay_Color TYPE_COLOR[MINIPOMO_TYPE_COUNT] = {
    COL_FOCUS,
    COL_SHORT_BREAK,
    COL_LONG_BREAK,
};

Clay_ElementId ui_click(enum cmd c, int arg)
{
    return CLAY_IDI(ID_CLICK, c * MINIPOMO_TASKS_MAX + arg);
}

void ui_link(const char *s, enum cmd c, int arg, Clay_Color normal,
             Clay_Color hover)
{
    CLAY(ui_click(c, arg), { 0 })
    {
        CLAY_TEXT(ui_str(s), ui_text(FONT_PLAIN, SIZE_TAB,
                                     Clay_Hovered() ? hover : normal));
    }
}

Clay_String ui_str(const char *s)
{
    return (Clay_String){ .length = minipomo_text_len(s), .chars = s };
}

Clay_TextElementConfig ui_text(int font, int size, Clay_Color color)
{
    return (Clay_TextElementConfig){ .fontId = font,
                                     .fontSize = size,
                                     .textColor = color };
}

void ui_gap(float w, float h)
{
    CLAY_AUTO_ID(
        { .layout.sizing = { CLAY_SIZING_FIXED(w), CLAY_SIZING_FIXED(h) } })
    {
    }
}

void ui_grow(void)
{
    CLAY_AUTO_ID(
        { .layout.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) } })
    {
    }
}

Clay_Color ui_type_color(const struct minipomo *p)
{
    return TYPE_COLOR[minipomo_current_type(p)];
}

void page(const struct view *v)
{
    Clay_Color white = COL_WHITE;
    double fraction = minipomo_fraction_left(v->pomo, v->now);
    CLAY(CLAY_ID(ID_PAGE), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0),
                        CLAY_SIZING_FIT(.min = v->window_height) },
            .padding = { PAGE_PAD, PAGE_PAD, 0, 0 },
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = ui_type_color(v->pomo),
        .transition = {
            .handler = Clay_EaseOut,
            .duration = FADE_SEC,
            .properties = CLAY_TRANSITION_PROPERTY_BACKGROUND_COLOR,
        },
    })
    {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0, PAGE_W),
                            CLAY_SIZING_FIXED(HEADER_H) },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        })
        {
            CLAY_TEXT(CLAY_STRING(TEXT_APP),
                      ui_text(FONT_ROUNDED, SIZE_TITLE, white));
        }
        CLAY_AUTO_ID({
            .layout.sizing = { CLAY_SIZING_GROW(0, PAGE_W),
                               CLAY_SIZING_FIXED(TRACK_H) },
            .backgroundColor = COL_SHADE,
        })
        {
            CLAY_AUTO_ID({
                .layout.sizing = { CLAY_SIZING_PERCENT(1 - fraction),
                                   CLAY_SIZING_GROW(0) },
                .backgroundColor = white,
            })
            {
            }
        }
        ui_grow();
        ui_gap(0, CARD_GAP);
        timer_view(v);
        ui_gap(0, LIST_GAP);
        task_view(v);
        ui_gap(0, CARD_GAP);
        ui_grow();
        CLAY_TEXT(CLAY_STRING(TEXT_CREDIT),
                  ui_text(FONT_PLAIN, SIZE_CREDIT, (Clay_Color)COL_DIM));
        ui_gap(0, FOOTER_PAD);
    }
}
