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

void candidate(int N_PROCS){
    sem_t *sem2 = sem_open(SEM_NAME2, 0); // Il figlio apre il semaforo già esistente
    pid_t *votantes;
    votantes = malloc(N_PROCS * sizeof(pid_t));

    if (sem2 == SEM_FAILED) {
        perror("Errore in sem_open (figlio 1)");
        exit(1);
    }
    // Il candidato aspetta che tutti i votanti siano pronti
    printf("📌 Processo %d è il candidato! Attendo che tutti siano pronti...\n", getpid());
    fflush(stdout);
    for (int i = 0; i < N_PROCS - 1; i++) {
        sem_wait(sem2);
    }

    // 🔹 Apertura file per leggere i PID dei votanti
    FILE *file = fopen(LOG_FILE, "r");
    if (file == NULL) {
        perror("❌ Errore nell'apertura del file");
        exit(1);
    }

    int i = 0;
    while (i < N_PROCS && fscanf(file, "%d", (int *)&votantes[i]) == 1) {
        i++;
    }
    fclose(file);  // Chiudere il file dopo la lettura

    // 🔥 Invia SIGUSR2 ai votanti
    printf("📩 Il candidato %d sta inviando SIGUSR2 ai votanti...\n", getpid());
    fflush(stdout);
    for (i = 0; i < N_PROCS; i++) {
        if (votantes[i] > 0 && votantes[i] != getpid()) {
            printf("📤 Inviando SIGUSR2 a %d\n", votantes[i]);
            fflush(stdout);
            kill(votantes[i], SIGUSR2);
        }
    }
}