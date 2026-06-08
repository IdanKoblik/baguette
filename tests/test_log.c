#include "greatest.h"
#include "util/log.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CAPTURE_STDERR(buf, stmt)                       \
    do {                                                \
        char _path[] = "/tmp/baguette_log_XXXXXX";      \
        int _fd = mkstemp(_path);                       \
        int _saved = dup(STDERR_FILENO);                \
        fflush(stderr);                                 \
        dup2(_fd, STDERR_FILENO);                       \
        stmt;                                           \
        fflush(stderr);                                 \
        dup2(_saved, STDERR_FILENO);                    \
        close(_saved);                                  \
        lseek(_fd, 0, SEEK_SET);                        \
        ssize_t _n = read(_fd, (buf), sizeof(buf) - 1); \
        (buf)[_n < 0 ? 0 : _n] = '\0';                  \
        close(_fd);                                     \
        unlink(_path);                                  \
    } while (0)

TEST log_info_tag_and_message(void) {
    char buf[512];
    CAPTURE_STDERR(buf, INFO("hello %d", 42));
    ASSERT(strstr(buf, "[INFO]") != NULL);
    ASSERT(strstr(buf, "hello 42") != NULL);
    PASS();
}

TEST log_includes_location_and_timestamp(void) {
    char buf[512];
    CAPTURE_STDERR(buf, INFO("x"));
    ASSERT(strstr(buf, "test_log.c:") != NULL); // __FILE__:__LINE__
    ASSERT(strstr(buf, "20") != NULL);          // year in the timestamp
    ASSERT(buf[strlen(buf) - 1] == '\n');       // newline-terminated
    PASS();
}

TEST log_error_appends_errno(void) {
    char buf[512];
    errno = EINVAL;
    CAPTURE_STDERR(buf, ERROR("boom"));
    ASSERT(strstr(buf, "[ERROR]") != NULL);
    ASSERT(strstr(buf, "boom") != NULL);
    ASSERT(strstr(buf, "errno=22") != NULL);
    ASSERT(strstr(buf, strerror(EINVAL)) != NULL);
    PASS();
}

TEST log_warn_and_debug_tags(void) {
    char buf[512];
    CAPTURE_STDERR(buf, WARN("careful"));
    ASSERT(strstr(buf, "[WARN]") != NULL);
    ASSERT(strstr(buf, "careful") != NULL);

    CAPTURE_STDERR(buf, DEBUG("trace %s", "here"));
    ASSERT(strstr(buf, "[DEBUG]") != NULL);
    ASSERT(strstr(buf, "trace here") != NULL);
    PASS();
}

TEST log_no_varargs(void) {
    char buf[512];
    CAPTURE_STDERR(buf, INFO("plain message"));
    ASSERT(strstr(buf, "plain message") != NULL);
    PASS();
}

SUITE(log_suite) {
    RUN_TEST(log_info_tag_and_message);
    RUN_TEST(log_includes_location_and_timestamp);
    RUN_TEST(log_error_appends_errno);
    RUN_TEST(log_warn_and_debug_tags);
    RUN_TEST(log_no_varargs);
}
