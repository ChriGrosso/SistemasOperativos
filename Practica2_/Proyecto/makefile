CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -pthread -lrt

OBJS = main.o candidate.o eCandidatos.o votante.o

TARGET = voting

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f *.o $(TARGET)
	rm -f voting_system.log candidate.log votes.txt
