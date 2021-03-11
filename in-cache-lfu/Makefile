IDIR = ../../inc
CFLAGS=-I$(IDIR)
CC = gcc
MAIN = lfu_cache_main.c
SRC = lfu_cache.c
DOTO = $(IDIR)/murmur3.o $(IDIR)/entropy.o $(IDIR)/libpcg_random.a


all: CACHE

CACHE: $(MAIN)
	$(CC) $(CFLAGS) $(MAIN) $(SRC) $(DOTO) -g3 -lm -o in_cache_lfu

perfect:
	$(CC) -DPERFECT_LFU $(CFLAGS) $(MAIN) $(SRC) $(DOTO) -g3 -lm -o perfect_lfu
fast_perfect:

clean:
	rm -f mru_stack

