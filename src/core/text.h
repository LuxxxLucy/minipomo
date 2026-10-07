#ifndef MINIPOMO_TEXT_H
#define MINIPOMO_TEXT_H

int minipomo_text_len(const char *text);
char *minipomo_text_put(char *out, const char *text);
void minipomo_text_copy(char *out, int capacity, const char *text, int length);
char *minipomo_format_int(char *out, long long value);
char *minipomo_format_ratio(char *out, long long done, long long total);
char *minipomo_format_mmss(char *out, int seconds);
char *minipomo_format_duration(char *out, long long seconds);

#endif
