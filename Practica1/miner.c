#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "pow.h"

typedef struct {
    long int target;
    long int start;
    long int end;
    int found;
    long int solution;
} MinerData;

void *mine(void *arg) {
    MinerData *data = (MinerData *)arg;
    for (long int i = data->start; i < data->end; i++) {
        if (pow_hash(i) == data->target) {
            data->found = 1;
            data->solution = i;
            return NULL;
        }
    }
    return NULL;
}

void miner(int rounds, int num_threads, int fd_write) {
    long int target;
    read(fd_write, &target, sizeof(long int)); // Lee el target inicial

    for (int round = 0; round < rounds; round++) {
        pthread_t threads[num_threads];
        MinerData data[num_threads];

        long int range = POW_LIMIT / num_threads;
        int found = 0;
        long int solution = -1;

        for (int i = 0; i < num_threads; i++) {
            data[i].target = target;
            data[i].start = i * range;
            data[i].end = (i + 1) * range;
            data[i].found = 0;
            pthread_create(&threads[i], NULL, mine, &data[i]);
        }

        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
            if (data[i].found) {
                found = 1;
                solution = data[i].solution;
            }
        }

        if (found) {
            write(fd_write, &target, sizeof(long int));
            write(fd_write, &solution, sizeof(long int));
        } else {
            fprintf(stderr, "No solution found\n");
            exit(EXIT_FAILURE);
        }

        target = solution;
    }
