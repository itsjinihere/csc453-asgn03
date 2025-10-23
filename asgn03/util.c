/*
 * util.c - tiny helpers: checked malloc, nanosleep helper, argv parsing
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "util.h"

void *xmalloc(size_t nbytes)
{
    void *p = malloc(nbytes);
    if (!p) {
        perror("malloc");
        exit(1);
    }
    return p;
}

void dawdle_ms(int max_ms)
{
    /* sleep for [0, max_ms] milliseconds (best effort) */
    struct timespec ts;
    long m = 0;

    if (max_ms <= 0) return;
    m = rand() % (max_ms + 1);

    ts.tv_sec = m / 1000;
    ts.tv_nsec = (m % 1000) * 1000000L;

    if (nanosleep(&ts, NULL) == -1) {
        /* not fatal; just print once-y message if needed */
        /* perror("nanosleep"); */
    }
}

int parse_nonneg(const char *s, int *out)
{
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (!s || *s == '\0' || *end != '\0') return 0;
    if (v < 0 || v > 1000000) return 0; /* basic sanity */
    *out = (int)v;
    return 1;
}
