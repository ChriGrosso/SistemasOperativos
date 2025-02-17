#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "pow.h"

void miner(int rounds, int num_threads, int fd_write);
void monitor(int fd_read);

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <TARGET_INI> <ROUNDS> <N_THREADS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    long int target = atol(argv[1]);
    int rounds = atoi(argv[2]);
    int num_threads = atoi(argv[3]);

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("Error creando la tubería");
        exit(EXIT_FAILURE);
    }

    pid_t miner_pid = fork();
    if (miner_pid < 0) {
        perror("Error en fork()");
        exit(EXIT_FAILURE);
    }

    if (miner_pid == 0) { // Proceso Minero
        close(pipefd[0]);  // Cierra el extremo de lectura
        miner(rounds, num_threads, pipefd[1]);
        close(pipefd[1]);
        exit(EXIT_SUCCESS);
    }

    pid_t monitor_pid = fork();
    if (monitor_pid < 0) {
        perror("Error en fork()");
        exit(EXIT_FAILURE);
    }

    if (monitor_pid == 0) { // Proceso Monitor
        close(pipefd[1]);  // Cierra el extremo de escritura
        monitor(pipefd[0]);
        close(pipefd[0]);
        exit(EXIT_SUCCESS);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    int status;
    waitpid(miner_pid, &status, 0);
    printf("Miner exited with status %d\n", WIFEXITED(status) ? WEXITSTATUS(status) : -1);

    waitpid(monitor_pid, &status, 0);
    printf("Monitor exited with status %d\n", WIFEXITED(status) ? WEXITSTATUS(status) : -1);

    return 0;
}
