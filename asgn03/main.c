#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#ifdef __APPLE__
#include <fcntl.h>
#include <sys/stat.h>
#endif

#include "board.h"
#include "util.h"

#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 5
#endif

#if NUM_PHILOSOPHERS < 2
#error "Need at least 2 philosophers"
#endif

/* ---------- fork + “host” gate ---------- */
#ifdef __APPLE__
static sem_t *s_fork[NUM_PHILOSOPHERS];
static char   s_fname[NUM_PHILOSOPHERS][64];
static sem_t *s_host;
static char   s_hname[64];
#else
static sem_t  s_fork[NUM_PHILOSOPHERS];
static sem_t  s_host;
#endif

static int g_cycles = 1;

static int left_fork(int who)  { return who; }
static int right_fork(int who) { return (who + 1) % NUM_PHILOSOPHERS; }

/* take / drop a single fork, with board update printed as one change */
static void take_one(int who, int fork_idx)
{
#ifdef __APPLE__
    if (sem_wait(s_fork[fork_idx]) != 0) { perror("sem_wait fork"); exit(1); }
#else
    if (sem_wait(&s_fork[fork_idx]) != 0) { perror("sem_wait fork"); exit(1); }
#endif
    board_take_fork(who, fork_idx);   /* <- one atomic change + print */
}

static void drop_one(int who, int fork_idx)
{
    board_drop_fork(who, fork_idx);   /* <- one atomic change + print */
#ifdef __APPLE__
    if (sem_post(s_fork[fork_idx]) != 0) { perror("sem_post fork"); exit(1); }
#else
    if (sem_post(&s_fork[fork_idx]) != 0) { perror("sem_post fork"); exit(1); }
#endif
}

/* ---------- worker ---------- */
static void *philosopher(void *arg)
{
    int who = *(int *)arg;
    int lf  = left_fork(who);
    int rf  = right_fork(who);
    int i;

    /* show “changing” before we start doing things */
    board_set_state(who, PS_CHANGING);

    for (i = 0; i < g_cycles; i++) {

        /* gate: only N-1 eaters try to grab forks */
#ifdef __APPLE__
        if (sem_wait(s_host) != 0) { perror("sem_wait host"); exit(1); }
#else
        if (sem_wait(&s_host) != 0) { perror("sem_wait host"); exit(1); }
#endif

        /* small asymmetry helps scheduling look nicer */
        if ((who & 1) == 0) { take_one(who, lf); take_one(who, rf); }
        else                { take_one(who, rf); take_one(who, lf); }

        board_set_state(who, PS_EATING);   /* one change */
        dawdle_ms(1000);

        /* flip to “changing” before putting forks back */
        board_set_state(who, PS_CHANGING); /* one change */

        drop_one(who, lf);
        drop_one(who, rf);

#ifdef __APPLE__
        if (sem_post(s_host) != 0) { perror("sem_post host"); exit(1); }
#else
        if (sem_post(&s_host) != 0) { perror("sem_post host"); exit(1); }
#endif

        /* only now it’s consistent to show “Think” (no forks held) */
        board_set_state(who, PS_THINKING); /* one change */
        dawdle_ms(800);

        /* go back to in-between */
        board_set_state(who, PS_CHANGING); /* one change */
    }

    return NULL;
}

/* ---------- driver ---------- */
int main(int argc, char **argv)
{
    pthread_t tid[NUM_PHILOSOPHERS];
    int ids[NUM_PHILOSOPHERS];
    int i;

    if (argc == 2) {
        if (!parse_nonneg(argv[1], &g_cycles)) {
            fprintf(stderr, "usage: %s [nonneg_cycles]\n", argv[0]);
            return 1;
        }
    } else if (argc > 2) {
        fprintf(stderr, "usage: %s [nonneg_cycles]\n", argv[0]);
        return 1;
    }

    srand((unsigned)time(NULL) ^ (unsigned)getpid());

#ifdef __APPLE__
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        snprintf(s_fname[i], sizeof(s_fname[i]), "/dine_fork_%d_%ld",
                 i, (long)getpid());
        sem_unlink(s_fname[i]);
        s_fork[i] = sem_open(s_fname[i], O_CREAT | O_EXCL, 0600, 1U);
        if (s_fork[i] == SEM_FAILED) { perror("sem_open fork"); return 1; }
    }
    snprintf(s_hname, sizeof(s_hname), "/dine_host_%ld", (long)getpid());
    sem_unlink(s_hname);
    s_host = sem_open(s_hname, O_CREAT | O_EXCL, 0600,
                      (unsigned)(NUM_PHILOSOPHERS - 1));
    if (s_host == SEM_FAILED) { perror("sem_open host"); return 1; }
#else
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (sem_init(&s_fork[i], 0, 1U) != 0) {
            perror("sem_init fork");
            return 1;
        }
    }
    if (sem_init(&s_host, 0, (unsigned)(NUM_PHILOSOPHERS - 1)) != 0) {
        perror("sem_init host"); return 1;
    }
#endif

    board_init(NUM_PHILOSOPHERS);

    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        ids[i] = i;
        if (pthread_create(&tid[i], NULL, philosopher, &ids[i]) != 0) {
            perror("pthread_create"); return 1;
        }
    }
    for (i = 0; i < NUM_PHILOSOPHERS; i++) pthread_join(tid[i], NULL);

    board_destroy();

#ifdef __APPLE__
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (s_fork[i]) {
            sem_close(s_fork[i]);
            sem_unlink(s_fname[i]);
        }
    }
    if (s_host) { sem_close(s_host); sem_unlink(s_hname); }
#else
    for (i = 0; i < NUM_PHILOSOPHERS; i++) sem_destroy(&s_fork[i]);
    sem_destroy(&s_host);
#endif
    return 0;
}
