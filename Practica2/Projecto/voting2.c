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
#define VOTE_FILE "votes.txt"
#define SEM_NAME "/example_sem"

pid_t *votantes;
int N_PROCS;
static int volatile candidate=-1;

void votante(int idp, sem_t *sem) {
    sigset_t mask1, mask2, omask;
    int sig;

    // Blocca SIGUSR1 prima di aspettarlo
    sigemptyset(&mask1);
    sigaddset(&mask1, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask1, &omask);
    
    // Attende il segnale SIGUSR1
    sigwait(&mask1, &sig);
    printf("Signal ricevuto %d\n", getpid());

    sem_wait(sem); // Protegge la sezione critica

    if (candidate == -1) {
        candidate = getpid();
        printf("Proceso candidato: %d\n", getpid());

        for (int i = 0; i < N_PROCS; i++) {
            printf("Invio SIGUSR2 a %d\n", votantes[i]); // Debug
            if (getpid() != votantes[i]) {
                kill(votantes[i], SIGUSR2);
            }
        }
        sem_post(sem);
    } else {
        // Blocca SIGUSR2 PRIMA di rilasciare il semaforo per evitare perdite
        sigemptyset(&mask2);
        sigaddset(&mask2, SIGUSR2);
        sigprocmask(SIG_BLOCK, &mask2, NULL);

        sem_post(sem); // Rilascia il semaforo prima di sospendersi

        printf("Proceso votante %d in attesa di SIGUSR2...\n", getpid());
        sigsuspend(&mask2); // Attende SIGUSR2
        printf("Proceso votante %d ha ricevuto SIGUSR2.\n", getpid());
    }

    // Ripristina la maschera originale dei segnali
    sigprocmask(SIG_SETMASK, &omask, NULL);
}


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

    //Gestion Semaforo
    sem_t *sem = NULL;

    if ((sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) ==
      SEM_FAILED) {
    perror("sem_open");
    exit(EXIT_FAILURE);
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
            perror("Error en fork");
            exit(1);
        } else if (pid == 0) {
            //Figlio esegue codice di votante
            votante(i,sem);
            exit(0);
        } else {
            //Padre salva pid processo nel file
            votantes[i] = pid;
            fprintf(log_file, "Votante %d PID: %d\n", i, pid);
        }
    }

    //Il padre invia a tutti i figli SIGUSR1
    for (int i = 0; i < N_PROCS; i++) {
        kill(votantes[i], SIGUSR1);
    }
    while(1){
        
    }

    /*
    for (int i = 0; i < N_PROCS; i++) {
        waitpid(votantes[i], NULL, 0);
    }

    printf("Finishing by signal\n");

    free(votantes);
    exit(0);
    */

}