#include "flags.h"
#include <string.h>

int parse_flags(int argc, char **argv) {
    int flags = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
            flags |= FLAG_HELP;

        if (strcmp(argv[i], "--full") == 0 || strcmp(argv[i], "-f") == 0)
            flags |= FLAG_FULL;

        if (strcmp(argv[i], "--separated") == 0 || strcmp(argv[i], "-s") == 0)
            flags |= FLAG_SEPARATED;
    }

    return flags;
}
