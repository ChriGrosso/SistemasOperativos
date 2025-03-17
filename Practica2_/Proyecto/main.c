/**
 * @file main.c
 * @brief Función principal para la ejecución del sistema de votación.
 *
 * Este archivo se encarga de inicializar los semáforos, manejar las señales,
 * crear los procesos votantes y candidatos, y coordinar el ciclo de votación.
 *
 * @autor Christian y Pablo
 * @fecha 2025-03-09
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

#include "eCandidatos.h"

#define LOG_FILE "voting_system.log"
#define FILE_CANDIDATO "candidate.log"
#define VOTE_FILE "votes.txt"

#define SEM_NAME1 "/semCandidate"
#define SEM_NAME2 "/semSync"
#define SEM_NAME3 "/semVote"
#define SEM_NAME4 "/semRonda"

static pid_t *votantes;
int N_PROCS;

void handler_SIGALRM(int sig) {
    (void)sig;
    printf("Finishing by alarm\n");
    sem_unlink(SEM_NAME1);
    sem_unlink(SEM_NAME2);
    sem_unlink(SEM_NAME3);
    sem_unlink(SEM_NAME4);

    for (int i = 0; i < N_PROCS; i++) {
        kill(votantes[i], SIGTERM);
    }
    for (int i = 0; i < N_PROCS; i++) {
        waitpid(votantes[i], NULL, 0);
    }

    free(votantes);   
    exit(EXIT_SUCCESS);
}

void handler_SIGINT(int sig) {
    (void)sig;
    sem_unlink(SEM_NAME1);
    sem_unlink(SEM_NAME2);
    sem_unlink(SEM_NAME3);
    sem_unlink(SEM_NAME4);

    for (int i = 0; i < N_PROCS; i++) {
        kill(votantes[i], SIGTERM);
    }
    
    printf("Finishing by signal\n");
    for (int i = 0; i < N_PROCS; i++) {
        waitpid(votantes[i], NULL, 0);
    }

    free(votantes);
    exit(EXIT_SUCCESS);  
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <N_PROCS> <N_SECS>\n", argv[0]);
        return 1;
    }

    N_PROCS = atoi(argv[1]);
    int N_SECS = atoi(argv[2]);

    struct sigaction sa_alrm, sa_int;
    sa_alrm.sa_handler = handler_SIGALRM;
    sigemptyset(&sa_alrm.sa_mask);
    sa_alrm.sa_flags = 0;
    sigaction(SIGALRM, &sa_alrm, NULL);

    sa_int.sa_handler = handler_SIGINT;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    if (alarm(N_SECS)) {
        fprintf(stderr, "Warning: previously established alarm overwritten.\n");
    }

    sem_t *sem1 = sem_open(SEM_NAME1, O_CREAT, 0666, 1);
    if (sem1 == SEM_FAILED) {
        perror("Error opening sem1 (candidate file protection)");
        exit(EXIT_FAILURE);
    }
    sem_close(sem1);

    sem_t *sem2 = sem_open(SEM_NAME2, O_CREAT, 0666, 0);
    if (sem2 == SEM_FAILED) {
        perror("Error opening sem2 (sync candidate/voter)");
        exit(EXIT_FAILURE);
    }
    sem_close(sem2);

    sem_t *sem3 = sem_open(SEM_NAME3, O_CREAT, 0666, 1);
    if (sem3 == SEM_FAILED) {
        perror("Error opening sem3 (vote file protection)");
        exit(EXIT_FAILURE);
    }
    sem_close(sem3);

    sem_t *sem4 = sem_open(SEM_NAME4, O_CREAT, 0666, 0);
    if (sem4 == SEM_FAILED) {
        perror("Error opening sem4 (round sync)");
        exit(EXIT_FAILURE);
    }
    sem_close(sem4);

    votantes = malloc(N_PROCS * sizeof(pid_t));
    if (!votantes) {
        perror("Error allocating memory for votantes");
        exit(EXIT_FAILURE);
    }

    remove(FILE_CANDIDATO);
    remove(LOG_FILE);
    remove(VOTE_FILE);

    FILE *fp_candidate = fopen(FILE_CANDIDATO, "w");
    if (!fp_candidate) {
        perror("Error creating candidate.log");
    } else {
        fprintf(fp_candidate, "-1\n");
        fclose(fp_candidate);
    }

    FILE *fp_log = fopen(LOG_FILE, "w");
    if (!fp_log) {
        perror("Error creating voting_system.log");
        free(votantes);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < N_PROCS; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Error in fork()");
            free(votantes);
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            fclose(fp_log);
            elige_candidato(N_PROCS);
            exit(0);
        } else {
            votantes[i] = pid;
            fprintf(fp_log, "%d\n", pid);
            fflush(fp_log);
        }
    }
    fclose(fp_log);

    while (1) {
        for (int i = 0; i < N_PROCS; i++) {
            kill(votantes[i], SIGUSR1);
        }

        sem4 = sem_open(SEM_NAME4, 0);
        if (sem4 == SEM_FAILED) {
            perror("Error re-opening sem4 in main loop");
            break;
        }
        sem_wait(sem4);
        sem_close(sem4);
    }
}
