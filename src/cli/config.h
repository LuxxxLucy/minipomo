#ifndef CONFIG_H
#define CONFIG_H

#define RGB_FOCUS { 186, 73, 73 }
#define RGB_SHORT_BREAK { 56, 133, 138 }
#define RGB_LONG_BREAK { 57, 112, 151 }

#define POLL_MS 250
#define ESC_WAIT_MS 50
#define OUT_BUF (1 << 16)

#define DEFAULT_ROWS 24
#define DEFAULT_COLS 80
#define PAGE_MARGIN 4
#define LINE_MAX_W 64
#define CLOCK_TOP 4
#define CLOCK_ROWS 5
#define CLOCK_COLS 34
#define BUTTON_ROW (CLOCK_TOP + CLOCK_ROWS + 1)
#define TABS_ROW (CLOCK_TOP + CLOCK_ROWS + 3)
#define MESSAGE_ROW (CLOCK_TOP + CLOCK_ROWS + 5)
#define LIST_ROW (CLOCK_TOP + CLOCK_ROWS + 7)
#define LIST_FOOT 3
#define TAB_KEY 2
#define TAB_GAP 2
#define MARK_W 2
#define ROW_FIXED_W 21
#define LAST_ROW 999

#define CLOCK_LEN 8
#define COUNT_LEN 32
#define STAT_LEN 64
#define ESTIMATE_LEN 8
#define ANSWER_LEN 4

#define STATE_ENV "MINIPOMO_FILE"
#define STATE_FILE ".minipomo"
#define NOTIFY_SOUND "Purr"

#define TEXT_APP "MiniPomo"
#define TEXT_START "START"
#define TEXT_PAUSE "PAUSE"
#define TEXT_TASKS "Tasks"
#define TEXT_ADD "+ a: Add Task"
#define TEXT_POMOS "Pomos "
#define TEXT_SPENT "  Spent "
#define TEXT_FINISH "  Finish "
#define TEXT_TASK "Task"
#define TEXT_ESTIMATE "Est pomodoros"
#define TEXT_NOTE "Note"
#define TEXT_DELETE "Delete this task? (y/N)"
#define TEXT_CLEAR "Clear all tasks? (y/N)"
#define TEXT_NO_TTY "minipomo: needs a terminal\n"
#define TEXT_HELP                                                            \
    "space start/pause  s skip  1-3 type  \xE2\x86\x91\xE2\x86\x93 select  " \
    "enter play  x done  a add  e edit  d delete  J/K move  c clear  q quit"

#define ICON_BLOCK "\xE2\x96\x88\xE2\x96\x88"
#define ICON_SELECT "\xE2\x80\xBA "
#define ICON_EDGE "\xE2\x96\x8C "
#define ICON_CHECK "\xE2\x9C\x93 "
#define ICON_OPEN "\xE2\x97\x8B "
#define ICON_PAUSE "\xE2\x80\x96 "
#define ICON_PLAY "\xE2\x96\xB6 "
#define ICON_TRACK "\xE2\x94\x81"
#define ICON_RULE "\xE2\x94\x80"

#define COLON_GLYPH 10
#define DIGIT_GLYPHS                           \
    {                                          \
        { "###", "# #", "# #", "# #", "###" }, \
        { "  #", "  #", "  #", "  #", "  #" }, \
        { "###", "  #", "###", "#  ", "###" }, \
        { "###", "  #", "###", "  #", "###" }, \
        { "# #", "# #", "###", "  #", "  #" }, \
        { "###", "#  ", "###", "  #", "###" }, \
        { "###", "#  ", "###", "# #", "###" }, \
        { "###", "  #", "  #", "  #", "  #" }, \
        { "###", "# #", "###", "# #", "###" }, \
        { "###", "# #", "###", "  #", "###" }, \
        { " ", "#", " ", "#", " " },           \
    }

#endif
