#include "candidate.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LOG_FILE "voting_system.log"
#define FILE_CANDIDATO "candidate.log"
#define VOTE_FILE "votes.txt"
#define SEM_NAME2 "/semSync"
#define SEM_NAME4 "/semRonda"

void wait_for_votes(int N_PROCS) {
    int fd, votes_count = 0;
    char votes_buffer[N_PROCS];

    while (votes_count < (N_PROCS - 1)) {
        usleep(1000);
        fd = open(VOTE_FILE, O_RDONLY);
        if (fd < 0) {
            continue;
        }
        votes_count = read(fd, votes_buffer, N_PROCS - 1);
        close(fd);
    }

    int yes_count = 0, no_count = 0;
    printf("Candidate %d => [ ", getpid());
    for (int i = 0; i < votes_count; i++) {
        printf("%c ", votes_buffer[i]);
        if (votes_buffer[i] == 'Y') yes_count++;
        else no_count++;
    }
    printf("] => %s\n", (yes_count > no_count) ? "Accepted" : "Rejected");
}

void candidate(int N_PROCS) {
    sem_t *sem_sync = sem_open(SEM_NAME2, 0);
    sem_t *sem_ronda = sem_open(SEM_NAME4, 0);

    if (sem_sync == SEM_FAILED || sem_ronda == SEM_FAILED) {
        perror("Error opening semaphores in candidate()");
        exit(EXIT_FAILURE);
    }

    pid_t *voters_array = malloc(N_PROCS * sizeof(pid_t));
    if (!voters_array) {
        perror("Error allocating memory for voters_array in candidate()");
        exit(EXIT_FAILURE);
    }

    while (1) {
        for (int i = 0; i < N_PROCS - 1; i++) {
            sem_wait(sem_sync);
        }

        FILE *fp_log = fopen(LOG_FILE, "r");
        if (!fp_log) {
            perror("Error opening LOG_FILE in candidate()");
            exit(EXIT_FAILURE);
        }

        int count = 0;
        while (count < N_PROCS && fscanf(fp_log, "%d", &voters_array[count]) == 1) {
            count++;
        }
        fclose(fp_log);

        for (int i = 0; i < count; i++) {
            if (voters_array[i] > 0 && voters_array[i] != getpid()) {
                kill(voters_array[i], SIGUSR2);
            }
        }

        wait_for_votes(N_PROCS);

        remove(FILE_CANDIDATO);
        remove(VOTE_FILE);

        usleep(250000);
        sem_post(sem_ronda);
    }

    free(voters_array);
    sem_close(sem_sync);
    sem_close(sem_ronda);
}
