#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "board.h"
#include "util.h"

#define MAXP 64

static int g_n     = 0;
static int g_colw  = 13;

static enum ph_state g_state[MAXP];
static int           g_fork_owner[MAXP];

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;


static int left_fork(int who)  { return who; }
static int right_fork(int who) { return (who + 1) % g_n; }

static int holds_fork(int who, int f) { return g_fork_owner[f] == who; }

static int holds_both(int who)
{
    return holds_fork(who, left_fork(who)) &&
           holds_fork(who, right_fork(who));
}

static int holds_any(int who)
{
    return holds_fork(who, left_fork(who)) ||
           holds_fork(who, right_fork(who));
}

static enum ph_state effective_state(int who)
{
    if (g_state[who] == PS_EATING && !holds_both(who))   return PS_CHANGING;
    if (g_state[who] == PS_THINKING && holds_any(who))   return PS_CHANGING;
    return g_state[who];
}


static void rule_line(void)
{
    int i, k;

    putchar('|');
    for (i = 0; i < g_n; i++) {
        for (k = 0; k < g_colw; k++) putchar('=');
        putchar('|');
    }
    putchar('\n');
}

static void header_line(void)
{
    int i, j, left_pad, right_pad;

    rule_line();

    putchar('|');
    for (i = 0; i < g_n; i++) {
        
        left_pad  = (g_colw - 1) / 2;
        right_pad = g_colw - (left_pad + 1);

        for (j = 0; j < left_pad; j++) putchar(' ');
        putchar((char)('A' + i));
        for (j = 0; j < right_pad; j++) putchar(' ');
        putchar('|');
    }
    putchar('\n');

    rule_line();
}

static void print_row_unlocked(void)
{
    int i, f, pos;
    char forks[64];
    const char *name;

    putchar('|');
    for (i = 0; i < g_n; i++) {
        /* forks bitmap */
        putchar(' ');
        pos = 0;
        for (f = 0; f < g_n && pos < (int)sizeof(forks) - 1; f++) {
            forks[pos++] = holds_fork(i, f) ? (char)('0' + (f % 10)) : '-';
        }
        forks[pos] = '\0';
        fputs(forks, stdout);
        putchar(' ');

        /* state (blank when “changing”) padded to 5 chars */
        switch (effective_state(i)) {
        case PS_EATING:   name = "Eat";   break;
        case PS_THINKING: name = "Think"; break;
        default:          name = "";      break;
        }
        printf("%-5s ", name);

        putchar('|');
    }
    putchar('\n');
}


void board_init(int n)
{
    int i, f;

    if (n <= 0 || n > MAXP) {
        fprintf(stderr, "board_init: bad n=%d\n", n);
        exit(1);
    }

    g_n    = n;
    g_colw = g_n + 8;   

    for (i = 0; i < g_n; i++) g_state[i] = PS_CHANGING;
    for (f = 0; f < g_n; f++) g_fork_owner[f] = -1;

    header_line();
    print_row_unlocked();
}

void board_destroy(void)
{
    
}

void board_set_state(int who, enum ph_state s)
{
    pthread_mutex_lock(&g_lock);
    g_state[who] = s;
    print_row_unlocked();
    pthread_mutex_unlock(&g_lock);
}

void board_take_fork(int who, int fork_idx)
{
    pthread_mutex_lock(&g_lock);
    g_fork_owner[fork_idx] = who;
    print_row_unlocked();
    pthread_mutex_unlock(&g_lock);
}

void board_drop_fork(int who, int fork_idx)
{
    pthread_mutex_lock(&g_lock);
    (void)who; /* owner already implied by fork_idx */
    g_fork_owner[fork_idx] = -1;
    print_row_unlocked();
    pthread_mutex_unlock(&g_lock);
}
