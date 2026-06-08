#include "greatest.h"
#include "util/log.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

// The log macros emit via syslog(). LOG_PERROR makes syslog also echo each
// message to stderr, which lets us capture and assert on it here. The stderr
// echo is "<ident>[pid]: <message>" — note it carries neither the syslog
// priority level (INFO/WARN/...) nor the syslog timestamp, only the message
// the macro built (which embeds [__FILE__:__LINE__]).
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

TEST log_info_message(void) {
    char buf[512];
    CAPTURE_STDERR(buf, INFO("hello %d", 42));
    ASSERT(strstr(buf, "hello 42") != NULL);
    PASS();
}

TEST log_includes_location(void) {
    char buf[512];
    CAPTURE_STDERR(buf, INFO("x"));
    ASSERT(strstr(buf, "test_log.c:") != NULL); // [__FILE__:__LINE__]
    ASSERT(buf[strlen(buf) - 1] == '\n');       // newline-terminated
    PASS();
}

TEST log_error_appends_errno(void) {
    char buf[512];
    errno = EINVAL;
    CAPTURE_STDERR(buf, ERROR("boom"));
    ASSERT(strstr(buf, "boom") != NULL);
    ASSERT(strstr(buf, "errno=22") != NULL);
    ASSERT(strstr(buf, strerror(EINVAL)) != NULL);
    PASS();
}

TEST log_warn_and_debug_messages(void) {
    char buf[512];
    CAPTURE_STDERR(buf, WARN("careful"));
    ASSERT(strstr(buf, "careful") != NULL);

    CAPTURE_STDERR(buf, DEBUG("trace %s", "here"));
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
    // Route syslog to stderr (and tag it) so CAPTURE_STDERR can observe output.
    openlog("baguette_test", LOG_PERROR, LOG_USER);
    // DEBUG is at LOG_DEBUG priority; without this it may be filtered out.
    setlogmask(LOG_UPTO(LOG_DEBUG));
    RUN_TEST(log_info_message);
    RUN_TEST(log_includes_location);
    RUN_TEST(log_error_appends_errno);
    RUN_TEST(log_warn_and_debug_messages);
    RUN_TEST(log_no_varargs);
    closelog();
}
