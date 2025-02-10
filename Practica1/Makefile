# Opzioni di compilazione
CFLAGS = -Wall -Wextra -g

# Compilazione del programma
practica1: main.o pow.o
	gcc $(CFLAGS) -o practica1 main.o pow.o

# Compilazione dei file oggetto
main.o: main.c pow.h
	gcc $(CFLAGS) -c main.c

pow.o: pow.c pow.h
	gcc $(CFLAGS) -c pow.c

# Esecuzione del programma con argomenti
run: practica1
	./practica1 -r 10 -h 10 -o 10

# Pulizia dei file generati
clean:
	rm -f *.o practica1
