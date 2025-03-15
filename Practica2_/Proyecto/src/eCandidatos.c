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

void votante();
void candidate(int N_PROCS);


void elige_candidato(int N_PROCS) {
    sem_t *sem1 = sem_open(SEM_NAME1, 0); // Il figlio apre il semaforo già esistente

    if (sem1 == SEM_FAILED) {
        perror("Errore in sem_open (figlio 1)");
        exit(1);
    }

    sigset_t mask1, omask;
    int sig;
    FILE *file;
    int candidato=-1;

    // Blocca SIGUSR1 prima di aspettarlo
    sigemptyset(&mask1);
    sigaddset(&mask1, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask1, &omask);
    
    // Attende SIGUSR1
    sigwait(&mask1, &sig);
    //printf("Signal ricevuto %d\n", getpid());
    
    // 🔒 Entra nella sezione critica
    sem_wait(sem1);

    // Se il file non esiste, lo creiamo con -1
    file = fopen(FILE_CANDIDATO, "r"); // Prova ad aprire il file in sola lettura

    if (file == NULL) {
        // Il file non esiste, quindi lo creiamo e scriviamo -1
        file = fopen(FILE_CANDIDATO, "w"); // Apri in scrittura (crea il file se non esiste)
        if (file == NULL) {
            perror("Errore nella creazione del file");
        }
        fprintf(file, "-1\n");
        fclose(file);
    }

    //Leggo il numero da FILE
    file = fopen(FILE_CANDIDATO, "r");
    if (fscanf(file, "%d", &candidato)!=1){
        perror("Errore nella lettura del candidato");
    }

    if (candidato == -1) {
        // 📝 Scrive il proprio PID come candidato
        file = fopen(FILE_CANDIDATO, "w");
        if (file != NULL) {
            candidato = getpid();
            fprintf(file, "%d\n", candidato);
            fclose(file);
            printf("Proceso candidato: %d\n", candidato);
        }
    }
    // 🔓 Esce dalla sezione critica
    sem_post(sem1);

    if (candidato != getpid()) {
        votante();
    } 
    else {
        candidate(N_PROCS);
    }    
}