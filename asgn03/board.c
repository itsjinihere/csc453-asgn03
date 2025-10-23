/* Sreeeee */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>

#include "board.h"
#include "util.h"

#define MAXP 64

static int g_n = 0; 
static int g_w = 0;    

static enum ph_state g_state[MAXP];
static int g_fork_owner[MAXP];  
static int g_has_left[MAXP];
static int g_has_right[MAXP];

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static int left_fork(int who)  { return who; }
static int right_fork(int who) { return (who + 1) % g_n; }


static void rstrip(char *s)
{
    size_t n;
    if (!s) return;
    n = strlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t')) s[--n] = '\0';
}

static void print_now(const char *line)
{
    puts(line);
    fflush(stdout);
}


static void build_fork_bitmap(int i, char *out, size_t cap)
{
    int f;
    size_t lim = (cap == 0) ? 0 : cap - 1;
    if (cap == 0) return;

    for (f = 0; f < g_n && (size_t)f < lim; f++) {
        out[f] = (g_fork_owner[f] == i) ? (char)('0' + (f % 10)) : '-';
    }
    out[(g_n < (int)lim) ? g_n : (int)lim] = '\0';
}


static enum ph_state effective_state(int i)
{
    int both = (g_has_left[i] && g_has_right[i]);
    int any  = (g_has_left[i] || g_has_right[i]);

    if (g_state[i] == PS_EATING && !both)   return PS_CHANGING;
    if (g_state[i] == PS_THINKING && any)   return PS_CHANGING;
    return g_state[i];
}


static void print_rule(void)
{
    char buf[1024];
    size_t pos = 0, lim = sizeof(buf) - 1;
    int i, k;

    if (pos < lim) buf[pos++] = '|';
    for (i = 0; i < g_n; i++) {
        for (k = 0; k < g_w && pos < lim; k++) buf[pos++] = '=';
        if (pos < lim) buf[pos++] = '|';
    }
    buf[pos] = '\0';
    print_now(buf);
}


static void print_header(void)
{
    char buf[1024];
    size_t pos = 0, lim = sizeof(buf) - 1;
    int i, j;

    print_rule();

    if (pos < lim) buf[pos++] = '|';
    for (i = 0; i < g_n; i++) {
        int left_pad = (g_w - 1) / 2; 
        for (j = 0; j < left_pad && pos < lim; j++) buf[pos++] = ' ';
        if (pos < lim) buf[pos++] = (char)('A' + i);
        while (((pos + 1) % (g_w + 1)) != 0 && pos < lim) buf[pos++] = ' ';
        if (pos < lim) buf[pos++] = '|';
    }
    buf[pos] = '\0';
    print_now(buf);

    print_rule();
}



static void print_line_unlocked(void)
{
    char buf[1024];
    size_t pos = 0, lim = sizeof(buf) - 1;
    int i;

    if (pos < lim) buf[pos++] = '|';

    for (i = 0; i < g_n; i++) {
        char forks[72];
        const char *state = "";
        char cell[96];
        int used, pad;
        enum ph_state es = effective_state(i);

        if (es == PS_EATING)        state = "Eat";
        else if (es == PS_THINKING) state = "Think";

        build_fork_bitmap(i, forks, sizeof(forks));

        if (state[0] != '\0')
            used = snprintf(cell, sizeof(cell), "%s %s", forks, state);
        else
            used = snprintf(cell, sizeof(cell), "%s", forks);

        if (used < 0) used = 0;
        if (used > g_w) used = g_w;

        if (pos + (size_t)used < lim) {
            memcpy(buf + pos, cell, (size_t)used);
            pos += (size_t)used;
        } else {
            while (pos < lim && used-- > 0) buf[pos++] = ' ';
        }
        for (pad = used; pad < g_w && pos < lim; pad++) buf[pos++] = ' ';
        if (pos < lim) buf[pos++] = '|';
    }

    buf[pos] = '\0';
    rstrip(buf);
    print_now(buf);
}



void board_init(int n)
{
    int i;

    if (n <= 0 || n > MAXP) {
        fprintf(stderr, "board_init: bad n=%d\n", n);
        exit(1);
    }
    g_n = n;
    g_w = g_n + 5 + 3;              

    for (i = 0; i < g_n; i++) {
        g_state[i] = PS_CHANGING;
        g_has_left[i] = 0;
        g_has_right[i] = 0;
        g_fork_owner[i] = -1;
    }

    print_header();
    print_line_unlocked();          
}

void board_destroy(void)
{
    
}

void board_set_state(int who, enum ph_state s)
{
    pthread_mutex_lock(&g_lock);
    if (g_state[who] != s) {        
        g_state[who] = s;
        print_line_unlocked();
    }
    pthread_mutex_unlock(&g_lock);
}

void board_take_fork(int who, int fork_idx)
{
    int lf = left_fork(who);
    int rf = right_fork(who);

    pthread_mutex_lock(&g_lock);
    g_fork_owner[fork_idx] = who;
    if (fork_idx == lf) g_has_left[who]  = 1;
    if (fork_idx == rf) g_has_right[who] = 1;
    print_line_unlocked();
    pthread_mutex_unlock(&g_lock);
}

void board_drop_fork(int who, int fork_idx)
{
    int lf = left_fork(who);
    int rf = right_fork(who);

    pthread_mutex_lock(&g_lock);
    g_fork_owner[fork_idx] = -1;
    if (fork_idx == lf) g_has_left[who]  = 0;
    if (fork_idx == rf) g_has_right[who] = 0;
    print_line_unlocked();
    pthread_mutex_unlock(&g_lock);
}
