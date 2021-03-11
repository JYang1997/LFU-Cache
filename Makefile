IDIR = ../inc
CFLAGS=-I$(IDIR)
CC = gcc
MAIN = src/lfu_cache_main.c
SRC = src/lfu_cache.c
DOTO = $(IDIR)/avltree.o 


all: CACHE

CACHE: $(MAIN)
	$(CC) $(CFLAGS) $(MAIN) $(SRC) $(DOTO) -g3 -lm -o in_cache_lfu

perfect:
	$(CC) -DPERFECT_LFU $(CFLAGS) $(MAIN) $(SRC) $(DOTO) -g3 -lm -o perfect_lfu
fast-perfect:
	$(CC) -DFAST_PERFECT_LFU $(CFLAGS) $(MAIN) $(SRC) $(DOTO) -g3 -lm -o fast_perfect_lfu

clean:
	rm -f *.exe

