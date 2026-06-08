#include "buffer.h"
#include "../util/log.h"

int init_buffer(struct hud_state *state) {
    if (!state) {
        ERROR("cannot init buffer, hud state is null.");
        return -1;
    }

    // TODO

    return 0;
}
