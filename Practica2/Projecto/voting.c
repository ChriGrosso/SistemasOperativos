#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#define LOG_FILE "voting_system.log"
#define VOTE_FILE "votes.txt"

pid_t *votantes;
int N_PROCS;
int candidato_pid = -1;
int soy_candidato = 0;
int detener = 0;

// Manejador de SIGINT en el proceso Principal
void manejar_SIGINT(int sig) {
    printf("\nRecibida señal SIGINT. Terminando votantes...\n");

    for (int i = 0; i < N_PROCS; i++) {
        kill(votantes[i], SIGTERM);
    }

    for (int i = 0; i < N_PROCS; i++) {
        waitpid(votantes[i], NULL, 0);
    }

    printf("Finishing by signal\n");

    free(votantes);
    exit(0);
}

// Manejador de SIGTERM en los votantes
void manejar_SIGTERM(int sig) {
    printf("Votante %d (PID %d) terminando...\n", getpid(), getpid());
    exit(0);
}

// Manejador de SIGUSR1 (inicio del sistema o nueva votación)
void manejar_SIGUSR1(int sig) {
    soy_candidato = 0;
}

// Manejador de SIGUSR2 (votar)
void manejar_SIGUSR2(int sig) {
    int voto = rand() % 2; // 0 (N) o 1 (Y)
    int fd = open(VOTE_FILE, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        perror("Error al abrir archivo de votos");
        return;
    }
    char voto_char = voto ? 'Y' : 'N';
    write(fd, &voto_char, 1);
    close(fd);
}

// Función de los procesos votantes
void votante(int id) {
    srand(getpid());
    signal(SIGUSR1, manejar_SIGUSR1);
    signal(SIGUSR2, manejar_SIGUSR2);
    signal(SIGTERM, manejar_SIGTERM);

    while (!detener) {
        pause(); // Espera SIGUSR1 para iniciar

        if (rand() % N_PROCS == 0) {
            soy_candidato = 1;
            candidato_pid = getpid();
            printf("Proceso %d es Candidato.\n", candidato_pid);

            // Reiniciar el archivo de votos
            int fd = open(VOTE_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            close(fd);

            // Notificar a todos los votantes que voten
            for (int i = 0; i < N_PROCS; i++) {
                if (votantes[i] != candidato_pid)
                    kill(votantes[i], SIGUSR2);
            }

            // Esperar votos con polling (1 ms)
            int num_votos = 0;
            char votos[N_PROCS];
            while (num_votos < N_PROCS - 1) {
                usleep(1000);
                int fd = open(VOTE_FILE, O_RDONLY);
                num_votos = read(fd, votos, N_PROCS - 1);
                close(fd);
            }

            // Evaluar resultados
            int positivos = 0, negativos = 0;
            printf("Candidate %d => [ ", candidato_pid);
            for (int i = 0; i < num_votos; i++) {
                printf("%c ", votos[i]);
                if (votos[i] == 'Y') positivos++;
                else negativos++;
            }
            printf("] => %s\n", (positivos > negativos) ? "Accepted" : "Rejected");

            usleep(250000); // Espera 250 ms antes de nueva ronda
            for (int i = 0; i < N_PROCS; i++) {
                kill(votantes[i], SIGUSR1);
            }
        }
    }
}

// Programa principal
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <N_PROCS> <N_SECS>\n", argv[0]);
        return 1;
    }

    N_PROCS = atoi(argv[1]);
    int N_SECS = atoi(argv[2]);

    signal(SIGINT, manejar_SIGINT);

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
            votante(i);
            exit(0);
        } else {
            votantes[i] = pid;
            fprintf(log_file, "Votante %d PID: %d\n", i, pid);
        }
    }

    fclose(log_file);
    sleep(1); // Asegurar que los procesos están listos

    for (int i = 0; i < N_PROCS; i++) {
        kill(votantes[i], SIGUSR1);
    }

    sleep(N_SECS);
    manejar_SIGINT(SIGINT);

    return 0;
}
