#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "pow.h"

// Declaración de la función miner con la nueva firma.
void miner(int rounds, int num_threads, long int initial_target,
           int pipe_m2mon_read, int pipe_m2mon_write,
           int pipe_mon2m_read, int pipe_mon2m_write);

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <TARGET_INI> <ROUNDS> <N_THREADS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    long int target = atol(argv[1]);
    int rounds = atoi(argv[2]);
    int num_threads = atoi(argv[3]);

    // Se validan los paramteros introducidos
    if (rounds <= 0 || num_threads <= 0) {
        fprintf(stderr, "Rounds and number of threads must be positive integers.\n");
        exit(EXIT_FAILURE);
    }

    // Creamos dos pipes: uno para Minero->Monitor y otro para Monitor->Minero.
    int pipe_m2mon[2];
    int pipe_mon2m[2];
    if (pipe(pipe_m2mon) == -1) {
        perror("Error creando la tubería m2mon");
        exit(EXIT_FAILURE);
    }
    if (pipe(pipe_mon2m) == -1) {
        perror("Error creando la tubería mon2m");
        exit(EXIT_FAILURE);
    }

    pid_t miner_pid = fork();
    if (miner_pid < 0) {
        perror("Error en fork()");
        exit(EXIT_FAILURE);
    }

    if (miner_pid == 0) {
        // En el proceso Minero, pasamos ambos extremos de cada pipe.
        miner(rounds, num_threads, target,
              pipe_m2mon[0], pipe_m2mon[1],
              pipe_mon2m[0], pipe_mon2m[1]);
        // Una vez finalizado, cerramos los descriptores heredados.
        close(pipe_m2mon[0]);
        close(pipe_m2mon[1]);
        close(pipe_mon2m[0]);
        close(pipe_mon2m[1]);
        exit(EXIT_SUCCESS);
    }

    // El proceso Principal cierra sus copias de los pipes.
    close(pipe_m2mon[0]);
    close(pipe_m2mon[1]);
    close(pipe_mon2m[0]);
    close(pipe_mon2m[1]);

    int status;
    waitpid(miner_pid, &status, 0);
    if (WIFEXITED(status))
        printf("Miner exited with status %d\n", WEXITSTATUS(status));
    else
        printf("Miner exited unexpectedly\n");

    return 0;
}
