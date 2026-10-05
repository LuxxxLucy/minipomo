#ifndef CONFIG_H
#define CONFIG_H

#define COL_FOCUS { 186, 73, 73, 255 }
#define COL_SHORT_BREAK { 56, 133, 138, 255 }
#define COL_LONG_BREAK { 57, 112, 151, 255 }
#define COL_WHITE { 255, 255, 255, 255 }
#define COL_DIM { 255, 255, 255, 153 }
#define COL_SOFT { 255, 255, 255, 204 }
#define COL_CARD { 255, 255, 255, 26 }
#define COL_SHADE { 0, 0, 0, 26 }
#define COL_BUTTON_SHADOW { 235, 235, 235, 255 }
#define COL_INK { 85, 85, 85, 255 }
#define COL_MUTED { 170, 170, 170, 255 }
#define COL_EDGE { 223, 223, 223, 255 }
#define COL_FIELD { 239, 239, 239, 255 }
#define COL_FOOT { 246, 246, 246, 255 }
#define COL_DARK { 34, 34, 34, 255 }
#define COL_NOTE { 252, 248, 222, 255 }
#define COL_NOTE_INK { 103, 90, 15, 255 }
#define COL_DASH { 255, 255, 255, 102 }

#define FONT_ROUNDED 0
#define FONT_PLAIN 1

#define PAGE_W 480
#define PAGE_PAD 12
#define HEADER_H 60
#define TRACK_H 4
#define CARD_GAP 40
#define CARD_PAD_TOP 20
#define CARD_PAD_BOTTOM 30
#define CARD_RADIUS 6
#define TAB_H 28
#define TAB_PAD_X 12
#define TAB_GAP 4
#define CLOCK_GAP 20
#define BUTTON_W 200
#define BUTTON_H 55
#define BUTTON_RADIUS 4
#define BUTTON_DROP 6
#define SKIP_W 60
#define MESSAGE_GAP 20
#define FOOTER_PAD 16

#define LIST_GAP 20
#define LIST_HEAD_H 44
#define LINE_H 2
#define STAT_LINE_H 1
#define ROW_GAP 8
#define ROW_RADIUS 4
#define ROW_EDGE 6
#define ROW_PAD 14
#define ROW_PAD_Y 18
#define CHECK_SIZE 24
#define ICON_SIZE 28
#define NOTE_PAD 10
#define NOTE_RADIUS 8
#define ADD_H 64
#define ADD_RADIUS 8
#define DASH_W 2
#define FORM_RADIUS 8
#define FORM_PAD 20
#define FORM_GAP 12
#define TITLE_FIELD_H 34
#define NOTE_FIELD_H 80
#define FIELD_RADIUS 6
#define ESTIMATE_W 75
#define ESTIMATE_H 40
#define SAVE_PAD_X 16
#define SAVE_PAD_Y 8
#define STAT_PAD 18
#define DRAG_START 6

#define SIZE_TITLE 20
#define SIZE_TAB 16
#define SIZE_CLOCK 120
#define SIZE_BUTTON 22
#define SIZE_MESSAGE 18
#define SIZE_CREDIT 14
#define SIZE_LIST 18
#define SIZE_TASK 16
#define SIZE_COUNT 18
#define SIZE_NOTE 15
#define LINE_GAP 6

#define FADE_SEC 0.5
#define DAY_MIN (24 * 60)
#define HOUR_SEC 3600

#define ARENA_BYTES (4 << 20)
#define MAX_ELEMENTS 1024
#define MAX_DRAWS 1024
// the page grows past the window; main.js sizes the canvas to it
#define LAYOUT_H 100000
#define TITLE_LEN (MINIPOMO_TITLE_MAX + 16)
#define CLOCK_LEN 8
#define COUNT_LEN 16
#define STAT_LEN 32

#define ID_CLICK "click"
#define ID_ROW "row"
#define ID_PAGE "page"
#define ID_FIELD_TITLE "field-title"
#define ID_FIELD_NOTE "field-note"

#define TEXT_APP "MiniPomo"
#define TEXT_START "START"
#define TEXT_PAUSE "PAUSE"
#define TEXT_SKIP "skip"
#define TEXT_CREDIT "visuals are inspired and copied from pomofocus.io"
#define TEXT_TASKS "Tasks"
#define TEXT_CLEAR "Clear all"
#define TEXT_ADD "+  Add Task"
#define TEXT_ESTIMATE "Est Pomodoros"
#define TEXT_ADD_NOTE "+ Add Note"
#define TEXT_DELETE "Delete"
#define TEXT_CANCEL "Cancel"
#define TEXT_SAVE "Save"
#define TEXT_POMOS "Pomos: "
#define TEXT_SPENT "Spent: "
#define TEXT_FINISH "Finish At: "
#define ICON_PLAY "▶"
#define ICON_PAUSE "❚❚"
#define ICON_CHECK "✓"
#define ICON_MORE "⋮"
#define ICON_UP "▲"
#define ICON_DOWN "▼"

#endif
