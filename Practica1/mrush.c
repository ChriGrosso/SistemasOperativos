#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "pow.h"

#define MAX_PIPE 2

void miner(int rounds, int num_threads, long int target_ini, int pipe1_read, int pipe1_write, int pipe2_read, int pipe2_write);

void validate_input(int argc, char *argv[], long int *target_ini, int *rounds, int *n_threads) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <TARGET_INI> <ROUNDS> <N_THREADS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    *target_ini = atol(argv[1]);
    *rounds = atoi(argv[2]);
    *n_threads = atoi(argv[3]);

    if (*rounds <= 0 || *n_threads <= 0) {
        fprintf(stderr, "Rounds and/or n_threads need to be a positive number.\n");
        exit(EXIT_FAILURE);
    }
}

void create_pipes(int pipe1[2], int pipe2[2]) {

    if (pipe(pipe1) == -1) {
        perror("Error creating pipe1");
        exit(EXIT_FAILURE);
    }
    if (pipe(pipe2) == -1) {
        perror("Error creating pipe2");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {

    long int target;
    int rounds, n_threads;
    int pipe1[MAX_PIPE], pipe2[MAX_PIPE];
    pid_t miner_pid;
    int status;

    validate_input(argc, argv, &target, &rounds, &n_threads);

    create_pipes(pipe1, pipe2);

    miner_pid = fork();
    if (miner_pid < 0) {
        perror("Error in fork()");
        exit(EXIT_FAILURE);
    }

    if (miner_pid == 0) {
        miner(rounds, n_threads, target, pipe1[0], pipe1[1], pipe2[0], pipe2[1]);
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        exit(EXIT_SUCCESS);
    }

    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    waitpid(miner_pid, &status, 0);
    if (WIFEXITED(status))
        printf("Miner exited with status %d\n", WEXITSTATUS(status));
    else
        printf("Miner exited unexpectedly\n");

    return 0;
}
