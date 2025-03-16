# Simple Makefile for the voting system project

CC = gcc
CFLAGS = -Wall -Wextra -O2

# Object files
OBJS = main.o candidate.o eCandidatos.o votante.o

# Target executable
TARGET = voting

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

clean:
	rm -f *.o $(TARGET)
	rm -f voting_system.log candidate.log votes.txt

