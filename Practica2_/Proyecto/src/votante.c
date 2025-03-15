#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>

#define LOG_FILE "voting_system.log"
#define FILE_CANDIDATO "candidate.log"
#define VOTE_FILE "votes.txt"
#define SEM_NAME1 "/semCandidate"
#define SEM_NAME2 "/semSync"

void votante(){
    sigset_t mask2;
    int sig;
    sem_t *sem2 = sem_open(SEM_NAME2, 0); // Il figlio apre il semaforo già esistente

    if (sem2 == SEM_FAILED) {
        perror("Errore in sem_open (figlio 1)");
        exit(1);
    }
    // Se non è candidato, segnala che è pronto e aspetta SIGUSR2
        sem_post(sem2);
        
        sigemptyset(&mask2);
        sigaddset(&mask2, SIGUSR2);
        sigprocmask(SIG_BLOCK, &mask2, NULL);

        printf("🟡 Processo votante %d in attesa di SIGUSR2...\n", getpid());
        fflush(stdout);

        sigwait(&mask2, &sig);  // Attende il segnale

        printf("🔔 Processo votante %d ha ricevuto SIGUSR2.\n", getpid());
        fflush(stdout);
}