/*hehe*/

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


#ifdef __APPLE__
static sem_t *g_fork[NUM_PHILOSOPHERS];
static char   g_fname[NUM_PHILOSOPHERS][64];
static sem_t *g_host;
static char   g_hname[64];
#else
static sem_t  g_fork[NUM_PHILOSOPHERS];
static sem_t  g_host;
#endif


static int g_cycles = 1;

static int left_fork(int who)  { return who; }
static int right_fork(int who) { return (who + 1) % NUM_PHILOSOPHERS; }

static void pick_up(int who, int fork_idx)
{
#ifdef __APPLE__
    if (sem_wait(g_fork[fork_idx]) != 0) { perror("sem_wait fork"); exit(1); }
#else
    if (sem_wait(&g_fork[fork_idx]) != 0) { perror("sem_wait fork"); exit(1); }
#endif
    board_take_fork(who, fork_idx);
}

static void put_down(int who, int fork_idx)
{
    board_drop_fork(who, fork_idx);
#ifdef __APPLE__
    if (sem_post(g_fork[fork_idx]) != 0) { perror("sem_post fork"); exit(1); }
#else
    if (sem_post(&g_fork[fork_idx]) != 0) { perror("sem_post fork"); exit(1); }
#endif
}

static void *philosopher(void *arg)
{
    int who = *(int *)arg;
    int i;
    int lf = left_fork(who);
    int rf = right_fork(who);

    for (i = 0; i < g_cycles; i++) {

        
#ifdef __APPLE__
        if (sem_wait(g_host) != 0) { perror("sem_wait host"); exit(1); }
#else
        if (sem_wait(&g_host) != 0) { perror("sem_wait host"); exit(1); }
#endif

        
        if ((who % 2) == 0) { pick_up(who, lf); pick_up(who, rf); }
        else                { pick_up(who, rf); pick_up(who, lf); }

        
        board_set_state(who, PS_EATING);
        dawdle_ms(1000);

        
        board_set_state(who, PS_CHANGING);

        
        put_down(who, lf);
        put_down(who, rf);

#ifdef __APPLE__
        if (sem_post(g_host) != 0) { perror("sem_post host"); exit(1); }
#else
        if (sem_post(&g_host) != 0) { perror("sem_post host"); exit(1); }
#endif

        board_set_state(who, PS_THINKING);
        dawdle_ms(800);
        
    }

    return NULL;
}

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
        snprintf(g_fname[i], sizeof(g_fname[i]),
            "/dine_fork_%d_%ld", i, (long)getpid());
        sem_unlink(g_fname[i]);
        g_fork[i] = sem_open(g_fname[i], O_CREAT | O_EXCL, 0600, 1U);
        if (g_fork[i] == SEM_FAILED) { perror("sem_open fork"); return 1; }
    }
    snprintf(g_hname, sizeof(g_hname), "/dine_host_%ld", (long)getpid());
    sem_unlink(g_hname);
    g_host = sem_open(g_hname,
                  O_CREAT | O_EXCL,
                  0600,
                  (unsigned)(NUM_PHILOSOPHERS - 1));

    if (g_host == SEM_FAILED) { perror("sem_open host"); return 1; }
#else
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (sem_init(&g_fork[i], 0, 1U) != 0) {
            perror("sem_init fork");
            return 1;
        }

    }
    if (sem_init(&g_host, 0, (unsigned)(NUM_PHILOSOPHERS - 1)) != 0) {
        perror("sem_init host"); return 1;
    }
#endif

    board_init(NUM_PHILOSOPHERS);

    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        ids[i] = i;
        if (pthread_create(&tid[i], NULL, philosopher, &ids[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (i = 0; i < NUM_PHILOSOPHERS; i++) pthread_join(tid[i], NULL);

    board_destroy();

#ifdef __APPLE__
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (g_fork[i]) { sem_close(g_fork[i]); sem_unlink(g_fname[i]); }
    }
    if (g_host) { sem_close(g_host); sem_unlink(g_hname); }
#else
    for (i = 0; i < NUM_PHILOSOPHERS; i++) sem_destroy(&g_fork[i]);
    sem_destroy(&g_host);
#endif

    return 0;
}
