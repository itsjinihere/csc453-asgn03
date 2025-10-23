#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "board.h"

/* keep c89 happy */
#define MAXP 64

/* table sizing */
static int g_n = 0;
static int g_cellw = 13;     /* per-column interior width */

/* philosopher state + fork ownership */
static enum ph_state s_state[MAXP];      /* PS_* */
static int           s_fork_owner[MAXP]; /* -1=free, else philosopher id */

/* one mutex serializes: (update exactly one thing) -> render row -> print */
static pthread_mutex_t s_mx = PTHREAD_MUTEX_INITIALIZER;

/* last row printed, to suppress duplicates */
static char s_prev_line[1024];

/* helpers */
static int lfork(int me)  { return me; }
static int rfork(int me)  { return (me + 1) % g_n; }
static int owns(int who, int f) { return s_fork_owner[f] == who; }

static int owns_both(int who)
{
    return owns(who, lfork(who)) && owns(who, rfork(who));
}
static int owns_any(int who)
{
    return owns(who, lfork(who)) || owns(who, rfork(who));
}

static const char *pretty_state(int who)
{
    if (s_state[who] == PS_EATING)   return owns_both(who) ? "Eat"   : "";
    if (s_state[who] == PS_THINKING) return owns_any (who) ? ""      : "Think";
    return "";
}

/* draw a rule: |=============|... */
static void rule_line(void)
{
    int i, k;
    putchar('|');
    for (i = 0; i < g_n; i++) {
        for (k = 0; k < g_cellw; k++) putchar('=');
        putchar('|');
    }
    putchar('\n');
}

/* header with centered letters */
static void header_line(void)
{
    int i, lp, rp, j;
    rule_line();
    putchar('|');
    for (i = 0; i < g_n; i++) {
        lp = (g_cellw - 1) / 2;
        rp = g_cellw - (lp + 1);
        for (j = 0; j < lp; j++) putchar(' ');
        putchar((char)('A' + i));
        for (j = 0; j < rp; j++) putchar(' ');
        putchar('|');
    }
    putchar('\n');
    rule_line();
}

/* build entire row into buf; return length */
static int make_row(char *buf, int cap)
{
    int i, f, pos = 0;
    if (cap <= 0) return 0;

    buf[pos++] = '|';
    for (i = 0; i < g_n; i++) {
        /* 1) forks bitmap (exactly g_n chars) */
        buf[pos++] = ' ';
        for (f = 0; f < g_n && pos < cap - 1; f++) {
            buf[pos++] = owns(i, f) ? (char)('0' + (f % 10)) : '-';
        }
        buf[pos++] = ' ';

        /* 2) state padded to 5 (so “Eat” aligns with “Think”) */
        {
            const char *s = pretty_state(i);
            int k = 0;
            while (s[k] && pos < cap - 1) { buf[pos++] = s[k++]; }
            while (k < 5 && pos < cap - 1) { buf[pos++] = ' '; k++; }
            buf[pos++] = ' ';
        }

        buf[pos++] = '|';
    }
    buf[pos++] = '\0';
    return pos - 1;
}

/* print row if different from last time */
static void print_row_unlocked(void)
{
    char line[1024];
    make_row(line, (int)sizeof(line));
    if (s_prev_line[0] != '\0' && strcmp(s_prev_line, line) == 0) {
        return; /* duplicate -> suppress to avoid “duplicates” warning */
    }
    fputs(line, stdout);
    fputc('\n', stdout);
    fflush(stdout);
    strncpy(s_prev_line, line, sizeof(s_prev_line) - 1);
    s_prev_line[sizeof(s_prev_line) - 1] = '\0';
}

/* ---------- public API ---------- */
void board_init(int n)
{
    int i;
    if (n <= 0 || n > MAXP) {
        fprintf(stderr, "board_init: bad n=%d\n", n);
        exit(1);
    }
    g_n = n;
    g_cellw = g_n + 8;            /* matches the grader’s sample spacing */

    for (i = 0; i < g_n; i++) s_state[i] = PS_CHANGING;
    for (i = 0; i < g_n; i++) s_fork_owner[i] = -1;
    s_prev_line[0] = '\0';

    header_line();
    /* initial snapshot (all changing, no forks) */
    print_row_unlocked();
}

void board_destroy(void) { /* nothing to do */ }

void board_set_state(int who, enum ph_state s)
{
    pthread_mutex_lock(&s_mx);
    s_state[who] = s;           /* exactly one change */
    print_row_unlocked();
    pthread_mutex_unlock(&s_mx);
}

void board_take_fork(int who, int fork_idx)
{
    pthread_mutex_lock(&s_mx);
    s_fork_owner[fork_idx] = who;  /* exactly one change */
    print_row_unlocked();
    pthread_mutex_unlock(&s_mx);
}

void board_drop_fork(int who, int fork_idx)
{
    (void)who; /* not needed to compute printout */
    pthread_mutex_lock(&s_mx);
    s_fork_owner[fork_idx] = -1;   /* exactly one change */
    print_row_unlocked();
    pthread_mutex_unlock(&s_mx);
}
