#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "pow.h"

typedef struct {
    long int target;
    long int start;
    long int end;
    int found;
    long int solution;
} MinerData;

// Función que realiza la búsqueda en un rango determinado.
void *mine(void *arg) {
    MinerData *data = (MinerData *)arg;
    for (long int i = data->start; i < data->end; i++) {
        if (pow_hash(i) == data->target) {
            data->found = 1;
            data->solution = i;
            break;
        }
    }
    return NULL;
}

// Declaración de la función monitor (implementada en monitor.c)
void monitor(int pipe_m2mon_read, int pipe_mon2m_write);

void miner(int rounds, int num_threads, long int target,
           int pipe_m2mon_read, int pipe_m2mon_write,
           int pipe_mon2m_read, int pipe_mon2m_write) {

    // Se crea el proceso Monitor.
    pid_t monitor_pid = fork();
    if (monitor_pid < 0) {
        perror("Error en fork() de monitor");
        exit(EXIT_FAILURE);
    }
    if (monitor_pid == 0) {
        // Proceso Monitor:
        close(pipe_m2mon_write);  // Monitor solo lee desde m2mon.
        close(pipe_mon2m_read);   // Monitor solo escribe en mon2m.
        monitor(pipe_m2mon_read, pipe_mon2m_write);
        close(pipe_m2mon_read);
        close(pipe_mon2m_write);
        exit(EXIT_SUCCESS);
    }
    // En el proceso Minero:
    close(pipe_m2mon_read);  // Minero solo escribe en m2mon.
    close(pipe_mon2m_write); // Minero solo lee desde mon2m.

    for (int round = 0; round < rounds; round++) {
        pthread_t threads[num_threads];
        MinerData data[num_threads];

        long int range = POW_LIMIT / num_threads;
        int found = 0;
        long int solution = -1;

        // Se crean los hilos repartiendo el espacio de búsqueda.
        for (int i = 0; i < num_threads; i++) {
            data[i].target = target;
            data[i].start = i * range;
            data[i].end = (i == num_threads - 1) ? POW_LIMIT : (i + 1) * range;
            data[i].found = 0;
            pthread_create(&threads[i], NULL, mine, &data[i]);
        }

        // Se espera a que terminen los hilos.
        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
            if (data[i].found) {
                found = 1;
                solution = data[i].solution;
            }
        }

        if (found) {
            // Envío de target y solución al Monitor.
            if (write(pipe_m2mon_write, &target, sizeof(long int)) != sizeof(long int)) {
                perror("Error escribiendo target a monitor");
                exit(EXIT_FAILURE);
            }
            if (write(pipe_m2mon_write, &solution, sizeof(long int)) != sizeof(long int)) {
                perror("Error escribiendo solución a monitor");
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "No solution found\n");
            exit(EXIT_FAILURE);
        }

        // Se espera la confirmación del Monitor.
        int confirmation;
        if (read(pipe_mon2m_read, &confirmation, sizeof(int)) != sizeof(int)) {
            perror("Error leyendo confirmación de monitor");
            exit(EXIT_FAILURE);
        }
        if (confirmation != 0) {
            printf("The solution has been invalidated\n");
            // Es importante cerrar los pipes para que el Monitor reciba EOF y pueda terminar.
            close(pipe_m2mon_write);
            close(pipe_mon2m_read);
            int status;
            waitpid(monitor_pid, &status, 0);
            if (WIFEXITED(status))
                printf("Monitor exited with status %d\n", WEXITSTATUS(status));
            else
                printf("Monitor exited unexpectedly\n");
            exit(EXIT_FAILURE);
        }
        // Actualiza el target para la siguiente ronda.
        target = solution;
    }

    // Se cierra la comunicación para indicar al Monitor que no hay más datos.
    close(pipe_m2mon_write);
    close(pipe_mon2m_read);

    // Se espera a que finalice el Monitor y se muestra su código de salida.
    int status;
    waitpid(monitor_pid, &status, 0);
    if (WIFEXITED(status))
        printf("Monitor exited with status %d\n", WEXITSTATUS(status));
    else
        printf("Monitor exited unexpectedly\n");
}

