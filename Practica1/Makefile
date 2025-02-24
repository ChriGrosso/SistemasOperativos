CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = mrush
SOURCES = mrush.c miner.c monitor.c pow.c

.PHONY: all test clean

all: $(TARGET)

$(TARGET):
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

test: $(TARGET)
	@echo "Eseguo $(TARGET) con tre argomenti: 50 3 10"
	./$(TARGET) 50 3 10

clean:
	rm -f $(TARGET) 