/*
 * format.c - A small snprintf: walk the format, copy plain characters,
 * and turn each %-directive into text.
 *
 * Uses: <stdarg.h> (va_list), which GCC provides even with -ffreestanding.
 */

#include "format.h"
#include <stdint.h>

// Where the text goes; stops quietly when the buffer is full.
typedef struct {
    char *out;
    int size, length;
} sink_t;

static void emit(sink_t *s, char c) {
    if (s->length < s->size - 1)
        s->out[s->length++] = c;
}

// Write 'text' (n characters) padded to 'width'.
static void emit_padded(sink_t *s, const char *text, int n, int width, int left, char pad) {
    if (!left)
        for (int i = n; i < width; i++)
            emit(s, pad);
    for (int i = 0; i < n; i++)
        emit(s, text[i]);
    if (left)
        for (int i = n; i < width; i++)
            emit(s, ' ');
}

// 'value' as digits in 'base'; 'negative' adds a '-'.
static void emit_number(sink_t *s, uint32_t value, int base, int negative,
                        int width, int left, char pad) {
    static const char digits[] = "0123456789abcdef";
    char text[12];
    int n = 0;

    do {
        text[n++] = digits[value % base];
        value /= base;
    } while (value);

    if (negative) {
        if (pad == '0') {               // "-0042": the sign goes before the zeros
            emit(s, '-');
            width--;
        } else {
            text[n++] = '-';
        }
    }

    char ordered[12];                   // digits came out backwards
    for (int i = 0; i < n; i++)
        ordered[i] = text[n - 1 - i];
    emit_padded(s, ordered, n, width, left, pad);
}

int vformat(char *out, int size, const char *fmt, va_list args) {
    sink_t s = { out, size, 0 };
    if (size <= 0)
        return 0;

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            emit(&s, *fmt);
            continue;
        }

        fmt++;
        int left = 0, width = 0;
        char pad = ' ';
        if (*fmt == '-') { left = 1; fmt++; }
        if (*fmt == '0') { pad = '0'; fmt++; }
        while (*fmt >= '0' && *fmt <= '9')
            width = width * 10 + (*fmt++ - '0');

        switch (*fmt) {
        case 's': {
            const char *text = va_arg(args, const char *);
            if (!text) text = "(null)";
            int n = 0;
            while (text[n]) n++;
            emit_padded(&s, text, n, width, left, ' ');
            break;
        }
        case 'c': {
            char c = (char)va_arg(args, int);
            emit_padded(&s, &c, 1, width, left, ' ');
            break;
        }
        case 'd': {
            int v = va_arg(args, int);
            // Unsigned negate: -INT_MIN does not fit in an int.
            emit_number(&s, v < 0 ? -(uint32_t)v : (uint32_t)v, 10, v < 0, width, left, pad);
            break;
        }
        case 'u':
            emit_number(&s, va_arg(args, uint32_t), 10, 0, width, left, pad);
            break;
        case 'x':
            emit_number(&s, va_arg(args, uint32_t), 16, 0, width, left, pad);
            break;
        case 'o':                       // octal: file modes ("0644")
            emit_number(&s, va_arg(args, uint32_t), 8, 0, width, left, pad);
            break;
        case '%':
            emit(&s, '%');
            break;
        case '\0':                      // a lone '%' at the very end
            fmt--;
            break;
        default:                        // unknown: print it as it was
            emit(&s, '%');
            emit(&s, *fmt);
        }
    }

    out[s.length] = '\0';
    return s.length;
}

int format(char *out, int size, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vformat(out, size, fmt, args);
    va_end(args);
    return n;
}
