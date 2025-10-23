# Makefile — CPE 453 A3 (Dining Philosophers)
CC      = gcc
UNAME_S := $(shell uname -s)

# Default philosopher count (override via: make NPH=7)
NPH ?= 5

# Common compile flags
CFLAGS  = -Wall -Wextra -std=c89 -O2 -g \
          -D_POSIX_C_SOURCE=200809L \
          -DNUM_PHILOSOPHERS=$(NPH) \
          -pthread

# Link libs (Darwin has no librt)
ifeq ($(UNAME_S),Darwin)
  LDLIBS = -pthread
else
  LDLIBS = -pthread -lrt
endif

PROG = dine
OBJS = main.o board.o util.o
HDRS = board.h util.h

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDLIBS)

main.o: main.c $(HDRS)
board.o: board.c board.h util.h
util.o: util.c util.h

clean:
	rm -f $(PROG) $(OBJS) out.txt run*.txt

# quick local run
run: $(PROG)
	./$(PROG) 50

# light smoke
test: $(PROG)
	./$(PROG) 100 > out.txt
	@echo "Saved output to out.txt"

# heavier run (good for CSL)
stress: $(PROG)
	./$(PROG) 500 > run500.txt
	@echo "Saved output to run500.txt"

.PHONY: all clean run test stress
