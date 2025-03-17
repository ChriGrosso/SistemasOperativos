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

#define SEM_NAME1 "/semCandidate" // Protects candidate.log
#define SEM_NAME2 "/semSync"      // Sync between candidate & voters
#define SEM_NAME3 "/semVote"      // Protects votes file
#define SEM_NAME4 "/semRonda"     // Sync with parent

static pid_t *votantes;
int N_PROCS;

void handler_SIGALRM(int sig) {
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

    // Signal handling
    struct sigaction sa_alrm, sa_int;
    sa_alrm.sa_handler = handler_SIGALRM;
    sigemptyset(&sa_alrm.sa_mask);
    sa_alrm.sa_flags = 0;
    sigaction(SIGALRM, &sa_alrm, NULL);

    sa_int.sa_handler = handler_SIGINT;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    // Start alarm
    if (alarm(N_SECS)) {
        fprintf(stderr, "Warning: previously established alarm overwritten.\n");
    }

    // Create semaphores
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

    // Prepare arrays
    votantes = malloc(N_PROCS * sizeof(pid_t));
    if (!votantes) {
        perror("Error allocating memory for votantes");
        exit(EXIT_FAILURE);
    }

    // Clean or create initial files
    remove(FILE_CANDIDATO);
    remove(LOG_FILE);
    remove(VOTE_FILE);

    FILE *fp_candidate = fopen(FILE_CANDIDATO, "w");
    if (!fp_candidate) {
        perror("Error creating candidate.log");
    } else {
        fprintf(fp_candidate, "-1\n");  // No candidate assigned initially
        fclose(fp_candidate);
    }

    FILE *fp_log = fopen(LOG_FILE, "w");
    if (!fp_log) {
        perror("Error creating voting_system.log");
        free(votantes);
        exit(EXIT_FAILURE);
    }

    // Create child processes
    for (int i = 0; i < N_PROCS; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Error in fork()");
            free(votantes);
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // Child process
            fclose(fp_log);
            elige_candidato(N_PROCS);
            // Should never return
            exit(0);
        } else {
            // Parent process
            votantes[i] = pid;
            fprintf(fp_log, "%d\n", pid);
            fflush(fp_log);
        }
    }
    fclose(fp_log);

    // Main loop: sends SIGUSR1 to all children, waits for sem4, then repeats
    while (1) {
        // Send SIGUSR1 to all child processes
        for (int i = 0; i < N_PROCS; i++) {
            kill(votantes[i], SIGUSR1);
        }

        // Wait for the candidate to post sem4
        sem4 = sem_open(SEM_NAME4, 0);
        if (sem4 == SEM_FAILED) {
            perror("Error re-opening sem4 in main loop");
            break;
        }
        sem_wait(sem4);
        sem_close(sem4);
    }
}
