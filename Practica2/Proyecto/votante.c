/**
 * @file votante.c
 * @brief Funciones para el proceso votante en el sistema de votación.
 *
 * Este archivo contiene las funciones necesarias para generar y registrar votos,
 * así como para manejar la sincronización entre procesos votantes y candidatos.
 *
 * @author Christian y Pablo
 * @date 2025-03-09
 */

#include "votante.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>

#define VOTE_FILE "votes.txt"
#define SEM_NAME2 "/semSync"
#define SEM_NAME3 "/semVote"

void vote() {
    sem_t *sem_vote = sem_open(SEM_NAME3, 0);
    if (sem_vote == SEM_FAILED) {
        perror("Error opening SEM_NAME3 in vote()");
        exit(EXIT_FAILURE);
    }

    srand(getpid());
    char ballot = (rand() % 2) ? 'Y' : 'N';

    sem_wait(sem_vote);

    int fd = open(VOTE_FILE, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        perror("Error opening vote file in vote()");
        sem_post(sem_vote);
        sem_close(sem_vote);
        return;
    }

    ssize_t bytes_written = write(fd, &ballot, 1);
    close(fd);

    sem_post(sem_vote);
    sem_close(sem_vote);
}

void votante() {
    sem_t *sem_sync = sem_open(SEM_NAME2, 0);
    if (sem_sync == SEM_FAILED) {
        perror("Error opening SEM_NAME2 in votante()");
        exit(EXIT_FAILURE);
    }

    sigset_t mask2;
    sigemptyset(&mask2);
    sigaddset(&mask2, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask2, NULL);

    while (1) {
        sem_post(sem_sync);

        int sig;
        sigwait(&mask2, &sig);
        vote();
    }

    sem_close(sem_sync);
}
