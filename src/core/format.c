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

// Apply a colour change at byte offset `t`. If the current span is still empty
// (back-to-back tags, or a tag at the very start) we just retune it; otherwise
// we close it and open a fresh span. Once FMT_MAX_SPANS is reached the change
// is dropped and the remaining text keeps the current colour.
static void set_color(struct fmt_section *sec, size_t t, uint32_t color, bool has_color) {
    struct fmt_span *cur = &sec->spans[sec->nspans - 1];
    if (t == cur->start) {
        cur->color     = color;
        cur->has_color = has_color;
        return;
    }
    if (sec->nspans >= FMT_MAX_SPANS)
        return;
    cur->len = t - cur->start;
    sec->spans[sec->nspans++] = (struct fmt_span){
        .start = t, .len = 0, .color = color, .has_color = has_color,
    };
}

static void parse_section(struct fmt_section *sec, const char *s, size_t len) {
    size_t      t   = 0;
    const char *p   = s;
    const char *end = s + len;

    // Every section starts with one default-coloured span at offset 0.
    sec->nspans   = 1;
    sec->spans[0] = (struct fmt_span){ .start = 0, .len = 0, .color = 0, .has_color = false };

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
                        set_color(sec, t, 0, false); // reset to default
                        p = q + 1;
                        continue;
                    }
                    if (inner == 7 && parse_hex_color(p + 2, &rgb)) {
                        set_color(sec, t, rgb, true);
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
    sec->spans[sec->nspans - 1].len = t - sec->spans[sec->nspans - 1].start;
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
        bool prev_color = false;
        for (size_t i = 0; i < sec->nspans; i++) {
            const struct fmt_span *sp = &sec->spans[i];
            if (sp->has_color) {
                char tag[12]; // "%{#rrggbb}" + NUL
                snprintf(tag, sizeof tag, "%%{#%06x}", sp->color & 0xFFFFFFu);
                put_str(buf, cap, &need, tag);
                prev_color = true;
            } else if (prev_color) {
                put_str(buf, cap, &need, "%{-}"); // back to default
                prev_color = false;
            }
            for (size_t k = 0; k < sp->len; k++) {
                char c = sec->text[sp->start + k];
                if (c == '%') put_char(buf, cap, &need, '%'); // escape: % -> %%
                put_char(buf, cap, &need, c);
            }
        }
    }

    buf[need < cap ? need : cap - 1] = '\0';
    return (int)need;
}
