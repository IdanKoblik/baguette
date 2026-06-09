#include "format.h"
#include "../util/log.h"
#include <string.h>

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool parse_hex_color(const char *p, uint32_t *rgb) {
    if (p[0] != '#') return false;
    uint32_t v = 0;
    for (int i = 1; i <= 6; i++) {
        int h = hexval(p[i]);
        if (h < 0) return false;
        v = (v << 4) | (uint32_t)h;
    }
    *rgb = v;
    return true;
}

static void put_text(struct fmt_section *sec, size_t *len, char c) {
    if (*len < FMT_MAX_TEXT - 1)
        sec->text[(*len)++] = c;
}

static void parse_section(struct fmt_section *sec, const char *s, size_t len) {
    size_t      t   = 0;
    const char *p   = s;
    const char *end = s + len;

    while (p < end) {
        if (*p == '%' && p + 1 < end) {
            if (p[1] == '%') {              // %% -> literal %
                put_text(sec, &t, '%');
                p += 2;
                continue;
            }
            if (p[1] == '{') {              // %{...} tag
                const char *q = p + 2;
                while (q < end && *q != '}') q++;
                if (q < end) {              // found a closing brace
                    size_t inner = (size_t)(q - (p + 2));
                    uint32_t rgb;
                    if (inner == 1 && p[2] == '-') {
                        sec->has_color = false; // reset; later tags may set it again
                        sec->color     = 0;
                        p = q + 1;
                        continue;
                    }
                    if (inner == 7 && parse_hex_color(p + 2, &rgb)) {
                        sec->color     = rgb;   // last colour tag wins
                        sec->has_color = true;
                        p = q + 1;
                        continue;
                    }
                    // Unrecognised body: fall through and keep it literal.
                }
            }
        }
        put_text(sec, &t, *p);
        p++;
    }
    sec->text[t] = '\0';
}

int fmt_decode(const char *input, struct fmt_frame *out) {
    if (!input || !out) {
        ERROR("format: decode given null argument.");
        return -1;
    }

    memset(out, 0, sizeof(*out));

    // Split on the first two tabs; the last section absorbs any remaining tabs.
    struct fmt_section *sections[3] = { &out->left, &out->center, &out->right };
    const char *seg = input;
    for (int r = 0; r < 3; r++) {
        const char *delim = (r < 2) ? strchr(seg, FMT_REGION_DELIM) : NULL;
        size_t len = delim ? (size_t)(delim - seg) : strlen(seg);
        parse_section(sections[r], seg, len);
        if (!delim) break;
        seg = delim + 1;
    }

    return 0;
}

static void put_char(char *buf, size_t cap, size_t *need, char c) {
    if (*need + 1 < cap) buf[*need] = c;
    (*need)++;
}

static void put_str(char *buf, size_t cap, size_t *need, const char *s) {
    for (; *s; s++) put_char(buf, cap, need, *s);
}

int fmt_encode(const struct fmt_frame *frame, char *buf, size_t cap) {
    if (!frame || !buf || cap == 0) {
        if (buf && cap) buf[0] = '\0';
        ERROR("format: encode given null argument.");
        return -1;
    }

    const struct fmt_section *sections[3] = { &frame->left, &frame->center, &frame->right };
    size_t need = 0;

    for (int r = 0; r < 3; r++) {
        if (r > 0)
            put_char(buf, cap, &need, FMT_REGION_DELIM);

        const struct fmt_section *sec = sections[r];
        if (sec->has_color) {
            char tag[12]; // "%{#rrggbb}" + NUL
            snprintf(tag, sizeof tag, "%%{#%06x}", sec->color & 0xFFFFFFu);
            put_str(buf, cap, &need, tag);
        }

        for (const char *t = sec->text; *t; t++) {
            if (*t == '%') put_char(buf, cap, &need, '%'); // escape: % -> %%
            put_char(buf, cap, &need, *t);
        }
    }

    buf[need < cap ? need : cap - 1] = '\0';
    return (int)need;
}
