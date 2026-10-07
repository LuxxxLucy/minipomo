#include "core/text.h"

int minipomo_text_len(const char *s)
{
    int n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

char *minipomo_text_put(char *dst, const char *src)
{
    while (*src) {
        *dst++ = *src++;
    }
    *dst = '\0';
    return dst;
}

void minipomo_text_copy(char *dst, int cap, const char *src, int len)
{
    if (len > cap - 1) {
        len = cap - 1;
        while (len > 0 && (src[len] & 0xC0) == 0x80) {
            len--;
        }
    }
    for (int i = 0; i < len; i++) {
        dst[i] = src[i];
    }
    dst[len] = '\0';
}

char *minipomo_format_int(char *dst, long long n)
{
    char digits[20];
    int k = 0;
    unsigned long long magnitude =
        n < 0 ? 0ULL - (unsigned long long)n : (unsigned long long)n;
    if (n < 0) {
        *dst++ = '-';
    }
    do {
        digits[k++] = '0' + magnitude % 10;
        magnitude /= 10;
    } while (magnitude);
    while (k) {
        *dst++ = digits[--k];
    }
    *dst = '\0';
    return dst;
}

char *minipomo_format_ratio(char *dst, long long done, long long total)
{
    dst = minipomo_text_put(minipomo_format_int(dst, done), "/");
    return minipomo_format_int(dst, total);
}

static char *two_digits(char *dst, int n)
{
    *dst++ = '0' + n / 10 % 10;
    *dst++ = '0' + n % 10;
    *dst = '\0';
    return dst;
}

char *minipomo_format_mmss(char *dst, int sec)
{
    dst = two_digits(dst, sec / 60);
    *dst++ = ':';
    return two_digits(dst, sec % 60);
}

char *minipomo_format_duration(char *dst, long long sec)
{
    long long min = sec / 60;
    if (min >= 60) {
        dst = minipomo_text_put(minipomo_format_int(dst, min / 60), "h ");
        return minipomo_text_put(two_digits(dst, min % 60), "m");
    }
    return minipomo_text_put(minipomo_format_int(dst, min), "m");
}
