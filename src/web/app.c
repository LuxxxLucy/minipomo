#define CLAY_IMPLEMENTATION
#include "web/ui.h"

#define EXPORT(name) __attribute__((export_name(name)))
#define IMPORT(name) __attribute__((import_module("env"), import_name(name)))

IMPORT("measure")
float js_measure(const char *s, int len, int font, float size);
IMPORT("notify")
void js_notify(const char *message);
IMPORT("log")
void js_log(const char *s, int len);

enum draw_kind { DRAW_RECT = 1, DRAW_TEXT, DRAW_BORDER };
enum field_slot { FIELD_TITLE, FIELD_NOTE, FIELD_COUNT };
enum cursor { CURSOR_ARROW, CURSOR_HAND, CURSOR_DRAG };

// main.js reads these two structs; keep the field order
struct draw {
    Clay_BoundingBox box;
    Clay_Color color;
    Clay_CornerRadius radius;
    float border_left, border_right, border_top, border_bottom;
    float size;
    int kind;
    int font;
    int flags;
    const char *text;
    int len;
};

struct field {
    Clay_BoundingBox box;
    int version;
    const char *text;
};

struct hit {
    enum cmd cmd;
    int arg;
    int row;
};

static unsigned char arena[ARENA_BYTES];
static struct draw draws[MAX_DRAWS];
static char io[MINIPOMO_SAVE_MAX];
static char page_title[TITLE_LEN];

static struct minipomo pomo;
static struct form form;
static struct field fields[FIELD_COUNT] = {
    [FIELD_TITLE].text = form.title,
    [FIELD_NOTE].text = form.note,
};
static bool unsaved;
static enum cursor cursor;
static int held_row = NO_ROW;
static bool dragging;
static float press_y;
static double last_now;

static Clay_Dimensions measure(Clay_StringSlice s, Clay_TextElementConfig *cfg,
                               void *user)
{
    (void)user;
    return (Clay_Dimensions){
        js_measure(s.chars, s.length, cfg->fontId, cfg->fontSize),
        cfg->lineHeight ? cfg->lineHeight : cfg->fontSize,
    };
}

static void on_error(Clay_ErrorData e)
{
    js_log(e.errorText.chars, e.errorText.length);
}

static void open_form(int i)
{
    const struct minipomo_task *t = i == MINIPOMO_NONE ? NULL : &pomo.tasks[i];
    form.open = true;
    form.index = i;
    form.estimate = t ? t->estimate : 1;
    minipomo_text_put(form.title, t ? t->title : "");
    minipomo_text_put(form.note, t ? t->note : "");
    form.note_open = form.note[0] != '\0';
    for (int k = 0; k < FIELD_COUNT; k++) {
        fields[k].version++;
    }
}

static void save_form(void)
{
    if (!form.title[0]) {
        return;
    }
    if (form.index == MINIPOMO_NONE) {
        minipomo_add(&pomo, form.title, form.note, form.estimate);
    } else {
        minipomo_edit(&pomo, form.index, form.title, form.note, form.estimate);
    }
    form.open = false;
}

static void apply(enum cmd c, int arg, double now)
{
    switch (c) {
        case CMD_NONE:
            return;
        case CMD_START:
            minipomo_start(&pomo, now);
            break;
        case CMD_PAUSE:
            minipomo_pause(&pomo, now);
            break;
        case CMD_SKIP:
            minipomo_skip(&pomo, now);
            break;
        case CMD_SET_TYPE:
            minipomo_set_type(&pomo, arg, now);
            break;
        case CMD_PLAY:
            if (pomo.tasks[arg].running) {
                minipomo_pause(&pomo, now);
            } else {
                minipomo_play(&pomo, arg, now);
            }
            break;
        case CMD_MARK_DONE:
            minipomo_mark_done(&pomo, arg, !pomo.tasks[arg].done, now);
            break;
        case CMD_CLEAR:
            minipomo_init(&pomo);
            form.open = false;
            break;
        case CMD_EDIT:
            open_form(arg);
            break;
        case CMD_ADD:
            open_form(MINIPOMO_NONE);
            break;
        case CMD_ESTIMATE_UP:
            form.estimate += form.estimate < MINIPOMO_ESTIMATE_MAX;
            break;
        case CMD_ESTIMATE_DOWN:
            form.estimate -= form.estimate > 1;
            break;
        case CMD_NOTE:
            form.note_open = true;
            break;
        case CMD_SAVE:
            save_form();
            break;
        case CMD_CANCEL:
            form.open = false;
            break;
        case CMD_DELETE:
            minipomo_remove(&pomo, form.index);
            form.open = false;
            break;
    }
    unsaved = true;
}

static struct hit hit_test(void)
{
    unsigned click_base =
        Clay_GetElementIdWithIndex(CLAY_STRING(ID_CLICK), 0).baseId;
    unsigned row_base =
        Clay_GetElementIdWithIndex(CLAY_STRING(ID_ROW), 0).baseId;
    struct hit h = { CMD_NONE, 0, NO_ROW };
    Clay_ElementIdArray ids = Clay_GetPointerOverIds();
    for (int k = 0; k < ids.length; k++) {
        Clay_ElementId id = ids.internalArray[k];
        if (id.baseId == click_base) {
            h.cmd = (enum cmd)(id.offset / MINIPOMO_TASKS_MAX);
            h.arg = (int)(id.offset % MINIPOMO_TASKS_MAX);
        } else if (id.baseId == row_base) {
            h.row = (int)id.offset;
        }
    }
    return h;
}

static float row_middle(int i)
{
    Clay_ElementId id = Clay_GetElementIdWithIndex(CLAY_STRING(ID_ROW), i);
    Clay_BoundingBox b = Clay_GetElementData(id).boundingBox;
    return b.y + b.height / 2;
}

static void drag_row(float y, bool down)
{
    if (held_row == NO_ROW) {
        return;
    }
    if (!down) {
        held_row = NO_ROW;
        dragging = false;
        return;
    }
    if (y - press_y > DRAG_START || press_y - y > DRAG_START) {
        dragging = true;
    }
    if (!dragging) {
        return;
    }
    int to = held_row;
    if (held_row > 0 && y < row_middle(held_row - 1)) {
        to = held_row - 1;
    } else if (held_row < pomo.task_count - 1 && y > row_middle(held_row + 1)) {
        to = held_row + 1;
    }
    if (to != held_row) {
        minipomo_move(&pomo, held_row, to);
        held_row = to;
        unsaved = true;
    }
}

static void place_field(enum field_slot k, Clay_ElementId id)
{
    fields[k].box = Clay_GetElementData(id).boundingBox;
}

static int emit(Clay_RenderCommandArray cmds)
{
    int n = 0;
    for (int i = 0; i < cmds.length && n < MAX_DRAWS; i++) {
        Clay_RenderCommand *c = &cmds.internalArray[i];
        struct draw *d = &draws[n];
        d->box = c->boundingBox;
        d->flags = (int)(long)c->userData;
        if (c->commandType == CLAY_RENDER_COMMAND_TYPE_RECTANGLE) {
            d->kind = DRAW_RECT;
            d->color = c->renderData.rectangle.backgroundColor;
            d->radius = c->renderData.rectangle.cornerRadius;
        } else if (c->commandType == CLAY_RENDER_COMMAND_TYPE_BORDER) {
            Clay_BorderRenderData *b = &c->renderData.border;
            d->kind = DRAW_BORDER;
            d->color = b->color;
            d->radius = b->cornerRadius;
            d->border_left = b->width.left;
            d->border_right = b->width.right;
            d->border_top = b->width.top;
            d->border_bottom = b->width.bottom;
        } else if (c->commandType == CLAY_RENDER_COMMAND_TYPE_TEXT) {
            Clay_TextRenderData *t = &c->renderData.text;
            d->kind = DRAW_TEXT;
            d->color = t->textColor;
            d->size = t->fontSize;
            d->font = t->fontId;
            d->text = t->stringContents.chars;
            d->len = t->stringContents.length;
        } else {
            continue;
        }
        n++;
    }
    return n;
}

static void set_page_title(double now)
{
    const char *message = minipomo_message(&pomo);
    char *p =
        minipomo_format_mmss(page_title, minipomo_seconds_left(&pomo, now));
    if (message[0]) {
        p = minipomo_text_put(p, " - ");
        minipomo_text_copy(p, TITLE_LEN - (int)(p - page_title), message,
                           minipomo_text_len(message));
    }
}

EXPORT("app_init")
int app_init(void)
{
    Clay_SetMaxElementCount(MAX_ELEMENTS);
    if (Clay_MinMemorySize() > ARENA_BYTES) {
        return -1;
    }
    Clay_Initialize(Clay_CreateArenaWithCapacityAndMemory(ARENA_BYTES, arena),
                    (Clay_Dimensions){ 0, 0 },
                    (Clay_ErrorHandler){ .errorHandlerFunction = on_error });
    Clay_SetMeasureTextFunction(measure, 0);
    minipomo_init(&pomo);
    return 0;
}

EXPORT("app_frame")
int app_frame(float width, float height, double now, int utc_offset_min,
              float pointer_x, float pointer_y, bool pointer_down)
{
    if (minipomo_update(&pomo, now)) {
        js_notify(minipomo_message(&pomo));
        unsaved = true;
    }

    Clay_SetLayoutDimensions((Clay_Dimensions){ width, LAYOUT_H });
    Clay_SetPointerState((Clay_Vector2){ pointer_x, pointer_y }, pointer_down);
    Clay_PointerDataInteractionState press = Clay_GetPointerState().state;
    struct hit hit = hit_test();
    bool on_row = hit.row != NO_ROW && !form.open;
    if (press == CLAY_POINTER_DATA_RELEASED_THIS_FRAME && !dragging) {
        apply(hit.cmd, hit.arg, now);
    }
    if (press == CLAY_POINTER_DATA_PRESSED_THIS_FRAME && !hit.cmd && on_row) {
        held_row = hit.row;
        press_y = pointer_y;
    }
    drag_row(pointer_y, pointer_down);
    cursor = dragging            ? CURSOR_DRAG
             : hit.cmd || on_row ? CURSOR_HAND
                                 : CURSOR_ARROW;

    struct view v = {
        .pomo = &pomo,
        .form = &form,
        .now = now,
        .window_height = height,
        .minute_of_day =
            (int)(((long long)(now / 60) - utc_offset_min) % DAY_MIN),
        .pointer_down = pointer_down,
        .dragged_row = dragging ? held_row : NO_ROW,
    };
    Clay_BeginLayout();
    page(&v);
    float dt = last_now ? (float)(now - last_now) : 0;
    last_now = now;
    Clay_RenderCommandArray cmds = Clay_EndLayout(dt);

    place_field(FIELD_TITLE, CLAY_ID(ID_FIELD_TITLE));
    place_field(FIELD_NOTE, CLAY_ID(ID_FIELD_NOTE));
    set_page_title(now);
    return emit(cmds);
}

EXPORT("app_draws")
struct draw *app_draws(void)
{
    return draws;
}

EXPORT("app_fields")
struct field *app_fields(void)
{
    return fields;
}

EXPORT("app_title")
const char *app_title(void)
{
    return page_title;
}

EXPORT("app_io")
char *app_io(void)
{
    return io;
}

EXPORT("app_io_size")
int app_io_size(void)
{
    return sizeof io;
}

EXPORT("app_cursor")
int app_cursor(void)
{
    return cursor;
}

EXPORT("app_height")
float app_height(void)
{
    return Clay_GetElementData(CLAY_ID(ID_PAGE)).boundingBox.height;
}

EXPORT("app_input")
void app_input(int field, int len)
{
    char *dst = field == FIELD_TITLE ? form.title : form.note;
    int cap = field == FIELD_TITLE ? MINIPOMO_TITLE_MAX : MINIPOMO_NOTE_MAX;
    minipomo_text_copy(dst, cap, io, len);
}

EXPORT("app_key")
void app_key(bool enter)
{
    apply(enter ? CMD_SAVE : CMD_CANCEL, 0, last_now);
}

EXPORT("app_save")
int app_save(void)
{
    if (!unsaved) {
        return -1;
    }
    unsaved = false;
    return minipomo_save(&pomo, io);
}

EXPORT("app_load")
void app_load(int len)
{
    minipomo_load(&pomo, io, len);
}
