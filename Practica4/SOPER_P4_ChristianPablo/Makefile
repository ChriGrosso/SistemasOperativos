CC      = gcc
CFLAGS  = -Wall -Wextra -std=gnu11 -O2 -pthread -D_GNU_SOURCE
LDFLAGS = -lrt

TARGETS = miner monitor

all: $(TARGETS)

miner: miner.c pow.c
	$(CC) $(CFLAGS) -o miner miner.c pow.c $(LDFLAGS)

monitor: monitor.c
	$(CC) $(CFLAGS) -o monitor monitor.c $(LDFLAGS)

clean:
	rm -f $(TARGETS) *.o