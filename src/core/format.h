#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Wire format for stdin frames.
//
// One line is one frame. A tab splits the line into up to three sections:
//
//     LEFT  \t  CENTER  \t  RIGHT
//
// The third section keeps any further tabs literally. Each section is plain
// text with optional inline colour:
//
//     %{#rrggbb}   colour the text that follows
//     %{-}         reset to the default colour
//     %%           a literal '%'
//
// A colour tag applies from where it appears until the next tag or the end of
// the section, so a single section can carry several differently-coloured runs
// (e.g. a red volume next to a blue uptime). Anything that is not a recognised
// tag (unknown body, missing '}') is kept literally, so no input is ever
// silently dropped.

#define FMT_REGION_DELIM '\t'
#define FMT_MAX_TEXT 256 // bytes per section, including the NUL
#define FMT_MAX_SPANS 32 // colour runs per section; extras keep prior colour

// A run of bytes in fmt_section::text drawn in one colour. has_color == false
// means "use the default colour" (no tag yet, or after a %{-} reset).
struct fmt_span {
    size_t start;   // byte offset into the section's text
    size_t len;     // byte length of the run
    uint32_t color; // 0xRRGGBB, meaningful only when has_color is true
    bool has_color;
};

// One section: its decoded text plus the colour runs covering it. There is
// always at least one span; spans tile text[] in order with no gaps.
struct fmt_section {
    char text[FMT_MAX_TEXT];
    struct fmt_span spans[FMT_MAX_SPANS];
    size_t nspans;
};

struct fmt_frame {
    struct fmt_section left;
    struct fmt_section center;
    struct fmt_section right;
};

int fmt_decode(const char *input, struct fmt_frame *out);

int fmt_encode(const struct fmt_frame *frame, char *buf, size_t cap);
