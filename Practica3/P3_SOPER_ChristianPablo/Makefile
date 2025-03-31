CC = gcc
CFLAGS = -D_GNU_SOURCE -Wall -Wextra
LDFLAGS = -pthread -lrt
TARGETS = miner monitor

all: $(TARGETS)

miner: miner.c
	$(CC) $(CFLAGS) -o miner miner.c $(LDFLAGS)

monitor: monitor.c
	$(CC) $(CFLAGS) -o monitor monitor.c $(LDFLAGS)

clean:
	rm -f $(TARGETS) *.o