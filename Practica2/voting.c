#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FILENAME "voting_pids.txt"

int *pids;  // Array dinámico para almacenar PIDs de los votantes
int N_PROCS;

// Manejador de SIGINT para terminar los votantes
void handle_sigint(int signo) {
    printf("\nRecibido SIGINT. Terminando procesos votantes...\n");
    for (int i = 0; i < N_PROCS; i++) {
        kill(pids[i], SIGTERM);  // Enviar SIGTERM a cada votante
    }
    for (int i = 0; i < N_PROCS; i++) {
        waitpid(pids[i], NULL, 0);  // Esperar que terminen los votantes
    }
    printf("Finishing by signal\n");
    free(pids);
    exit(EXIT_SUCCESS);
}

// Función de cada votante
void voter_process() {
    printf("Votante con PID %d iniciado.\n", getpid());
    while (1) {
        pause();  // Espera señales
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <N_PROCS> <N_SECS>\n", argv[0]);
        return EXIT_FAILURE;
    }

    N_PROCS = atoi(argv[1]);
    int N_SECS = atoi(argv[2]);

    if (N_PROCS <= 0 || N_SECS <= 0) {
        fprintf(stderr, "Los valores deben ser enteros positivos.\n");
        return EXIT_FAILURE;
    }

    pids = (int *)malloc(N_PROCS * sizeof(int));
    if (!pids) {
        perror("Error al asignar memoria");
        return EXIT_FAILURE;
    }

    // Configurar el manejador de SIGINT
    signal(SIGINT, handle_sigint);

    FILE *file = fopen(FILENAME, "w");
    if (!file) {
        perror("Error al abrir el archivo");
        free(pids);
        return EXIT_FAILURE;
    }

    // Crear procesos votantes
    for (int i = 0; i < N_PROCS; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Error en fork");
            free(pids);
            fclose(file);
            return EXIT_FAILURE;
        }
        if (pid == 0) {  // Código del votante
            voter_process();
            exit(0);
        } else {  // Código del proceso principal
            pids[i] = pid;
            fprintf(file, "%d\n", pid);
        }
    }

    fclose(file);
    printf("Sistema listo. Enviando SIGUSR1 a los votantes...\n");

    // Enviar SIGUSR1 a todos los votantes
    for (int i = 0; i < N_PROCS; i++) {
        kill(pids[i], SIGUSR1);
    }

    // Esperar el tiempo máximo de ejecución
    sleep(N_SECS);

    // Si no se recibe SIGINT, terminamos los votantes normalmente
    printf("Tiempo agotado. Terminando procesos votantes...\n");
    handle_sigint(SIGINT);

    return EXIT_SUCCESS;
}
