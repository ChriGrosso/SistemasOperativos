#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "pow.h"

typedef struct {
    long int target_ini;
    long int beginning;
    long int finish;
    int discover;
    long int solution;
} MinerData;

void *mine(void *arg) {

    MinerData *data = (MinerData *)arg;
    long int i = 0;
    
    for (i = data->beginning; i < data->finish; i++) {
        if (pow_hash(i) == data->target_ini) {
            data->discover = 1;
            data->solution = i;
            break;
        }
    }
    
    return NULL;
}

void monitor(int pipe1_read, int pipe2_write);

void threadsCreate(pthread_t *threads, MinerData *data, int num_threads, long int target) {

    long int range = POW_LIMIT / num_threads;
    int i = 0;

    for (i = 0; i < num_threads; i++) {
        data[i].target_ini = target;
        data[i].beginning = i * range;
        data[i].finish = (i == num_threads - 1) ? POW_LIMIT : (i + 1) * range;
        data[i].discover = 0;
        pthread_create(&threads[i], NULL, mine, &data[i]);
    }
}

void threadsJoin(pthread_t *threads, MinerData *data, int num_threads, int *found, long int *solution) {

    int i = 0;

    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        if (data[i].discover) {
            *found = 1;
            *solution = data[i].solution;
        }
    }
}

void miner(int rounds, int n_threads, long int target_ini, int pipe1_read, int pipe1_write, int pipe2_read, int pipe2_write) {
           
    int status = 0;
    int round = 0;
    pid_t monitor_pid;

    monitor_pid = fork();
    if (monitor_pid < 0) {
        perror("Error in fork() for monitor");
        exit(EXIT_FAILURE);
    }
    if (monitor_pid == 0) {
        close(pipe1_write);
        close(pipe2_read);
        monitor(pipe1_read, pipe2_write);
        close(pipe1_read);
        close(pipe2_write);
        exit(EXIT_SUCCESS);
    }

    close(pipe1_read);
    close(pipe2_write);

    for (round = 0; round < rounds; round++) {

        pthread_t threads[n_threads];
        MinerData data[n_threads];
        int discover = 0;
        long int solution = -1;
        int confirmation;
        int status;

        threadsCreate(threads, data, n_threads, target_ini);
        threadsJoin(threads, data, n_threads, &discover, &solution);

        if (discover) {
            if (write(pipe1_write, &target_ini, sizeof(long int)) != sizeof(long int) || write(pipe1_write, &solution, sizeof(long int)) != sizeof(long int)) {
                perror("Error writing to monitor");
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "No solution discovered\n");
            exit(EXIT_FAILURE);
        }

        if (read(pipe2_read, &confirmation, sizeof(int)) != sizeof(int)) {
            perror("Error reading from monitor");
            exit(EXIT_FAILURE);
        }
        if (confirmation != 0) {
            printf("The solution has been invalidated\n");
            close(pipe1_write);
            close(pipe2_read);
            waitpid(monitor_pid, &status, 0);
            if (WIFEXITED(status))
                printf("Monitor exited with status %d\n", WEXITSTATUS(status));
            else
                printf("Monitor exited unexpectedly\n");
            exit(EXIT_FAILURE);
        }
        target_ini = solution;
    }

    close(pipe1_write);
    close(pipe2_read);

    waitpid(monitor_pid, &status, 0);
    if (WIFEXITED(status))
        printf("Monitor exited with status %d\n", WEXITSTATUS(status));
    else
        printf("Monitor exited unexpectedly\n");
}
