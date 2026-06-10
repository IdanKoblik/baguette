#pragma once

enum {
    FLAG_HELP = 1 << 0,
    FLAG_FULL = 1 << 1,
    FLAG_SEPARATED = 1 << 2
};

int parse_flags(int argc, char **argv);
