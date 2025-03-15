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

static pid_t *votantes;
int N_PROCS;


void elige_candidato(int N_PROCS);

// Terminazione per ricezione SIGALRM
void handler_SIGALRM(int sig) {
    for (int i = 0; i < N_PROCS; i++) {
        kill(votantes[i], SIGTERM);
    }

    for (int i = 0; i < N_PROCS; i++) {
        waitpid(votantes[i], NULL, 0);
    }

    printf("Finishing by alarm\n");

    free(votantes);   
    exit(EXIT_SUCCESS);
}

// Terminazione per ricezione SIGINT
void handler_SIGINT(int sig) {
    for (int i = 0; i < N_PROCS; i++) {
        kill(votantes[i], SIGTERM);
    }

    for (int i = 0; i < N_PROCS; i++) {
        waitpid(votantes[i], NULL, 0);
    }

    printf("Finishing by signal\n");

    free(votantes);
    exit(EXIT_SUCCESS);    
}

// Programa principal
int main(int argc, char *argv[]) {
    struct sigaction sa1, sa2;
    remove(FILE_CANDIDATO);
    
    // Gestione parametri
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <N_PROCS> <N_SECS>\n", argv[0]);
        return 1;
    }
    
    N_PROCS = atoi(argv[1]);
    int N_SECS = atoi(argv[2]);
    
    //Gestione SIGINT
    sa1.sa_handler = handler_SIGINT;
    sigemptyset(&(sa1.sa_mask));
    sa1.sa_flags = 0;
    if (sigaction(SIGINT, &sa1, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    //Gestione SIGALRM
    sa2.sa_handler = handler_SIGALRM;
    sigemptyset(&(sa2.sa_mask));
    sa2.sa_flags = 0;
    if (sigaction(SIGALRM, &sa2, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    //ALARM STARTS
    if (alarm(N_SECS)) {
        fprintf(stderr, "There is a previously established alarm\n");
    }

    sem_t *sem1 = sem_open(SEM_NAME1, O_CREAT | O_EXCL, 0666, 1);
    if (sem1 == SEM_FAILED) {
        perror("Errore in sem_open (padre 1)");
        exit(1);
    }
    sem_t *sem2 = sem_open(SEM_NAME2, O_CREAT | O_EXCL, 0666, 0);
    if (sem2 == SEM_FAILED) {
        perror("Errore in sem_open (padre 2)");
        exit(1);
    }
    
    votantes = malloc(N_PROCS * sizeof(pid_t));
    FILE *log_file = fopen(LOG_FILE, "w");
    if (!log_file) {
        perror("Error al abrir el archivo de log");
        return 1;
    }

    printf("Iniciando sistema con %d procesos votantes...\n", N_PROCS);

    for (int i = 0; i < N_PROCS; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Errore in fork");
            exit(1);
        }

        if (pid > 0) {
            // Processo padre: salva il PID del figlio
            votantes[i] = pid;
            fprintf(log_file,/* "Votante %d PID: */"%d\n",/* i, */pid);
            printf("Votante %d PID: %d -- Scritto da %d\n", i, pid, getpid());
            fflush(log_file); // Scrive immediatamente nel file
        } else {
            // Processo figlio: chiude il file di log per evitare scritture multiple
            fclose(log_file);
            elige_candidato(N_PROCS);
            exit(0);
        }
    }

    //Il padre invia a tutti i figli SIGUSR1
    for (int i = 0; i < N_PROCS; i++) {
        kill(votantes[i], SIGUSR1);
    }
    sleep(10);
    
    for (int i = 0; i < N_PROCS; i++) {
        waitpid(votantes[i], NULL, 0);
    }

    sem_close(sem1);
}

