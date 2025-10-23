#ifndef BOARD_H
#define BOARD_H

#include <semaphore.h>

enum ph_state { PS_CHANGING = 0, PS_EATING = 1, PS_THINKING = 2 };

void board_init(int n);
void board_destroy(void);

/* one-change-per-line printers (already serialized inside) */
void board_set_state(int who, enum ph_state s);
void board_take_fork(int who, int fork_idx);
void board_drop_fork(int who, int fork_idx);

#endif
