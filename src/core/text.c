#include "core/minipomo.h"

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
    if (n < 0) {
        *dst++ = '-';
        n = -n;
    }
    do {
        digits[k++] = '0' + n % 10;
        n /= 10;
    } while (n);
    while (k) {
        *dst++ = digits[--k];
    }
    *dst = '\0';
    return dst;
}

char *minipomo_format_ratio(char *dst, int done, int total)
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

char *minipomo_format_duration(char *dst, int sec)
{
    int min = sec / 60;
    if (min >= 60) {
        dst = minipomo_text_put(minipomo_format_int(dst, min / 60), "h ");
        return minipomo_text_put(two_digits(dst, min % 60), "m");
    }
    return minipomo_text_put(minipomo_format_int(dst, min), "m");
}
