/**
 * @file eCandidatos.c
 * @brief Funciones para la selección de candidatos en el sistema de votación.
 *
 * Este archivo contiene las funciones necesarias para elegir candidatos y manejar
 * la sincronización entre procesos candidatos y votantes.
 *
 * @author Christian y Pablo
 * @date 2025-03-09
 */

#include "eCandidatos.h"
#include "votante.h"
#include "candidate.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FILE_CANDIDATO "candidate.log"
#define SEM_NAME1 "/semCandidate"

void elige_candidato(int N_PROCS) {
    sem_t *sem_candidate = sem_open(SEM_NAME1, 0);
    if (sem_candidate == SEM_FAILED) {
        perror("Error opening sem_candidate in elige_candidato()");
        exit(EXIT_FAILURE);
    }

    sigset_t mask1;
    sigemptyset(&mask1);
    sigaddset(&mask1, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask1, NULL);

    while (1) {
        int sig;
        sigwait(&mask1, &sig);

        sem_wait(sem_candidate);

        FILE *fp_candidate = fopen(FILE_CANDIDATO, "r");
        if (!fp_candidate) {
            fp_candidate = fopen(FILE_CANDIDATO, "w");
            if (!fp_candidate) {
                perror("Error creating candidate.log");
            }
            fprintf(fp_candidate, "-1\n");
            fclose(fp_candidate);
        } else {
            int candidato_val = 0;
            int result = fscanf(fp_candidate, "%d", &candidato_val);
            fclose(fp_candidate);
            if (candidato_val == -1) {
                fp_candidate = fopen(FILE_CANDIDATO, "w");
                if (fp_candidate) {
                    fprintf(fp_candidate, "%d\n", getpid());
                    fclose(fp_candidate);
                    sem_post(sem_candidate);
                    sem_close(sem_candidate);
                    candidate(N_PROCS);
                }
            } else {
                sem_post(sem_candidate);
                sem_close(sem_candidate);
                votante();
            }
        }
    }
}
