/**
 * @file mrush.c
 * @brief Función principal para el sistema de minado.
 *
 * Este archivo se encarga de validar los parámetros de entrada, crear los pipes necesarios,
 * forkar el proceso Minero y esperar a su finalización.
 *
 * @author Christian y Pablo
 * @date 2025-02-21
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "pow.h"

#define MAX_PIPE 2

/**
 * @brief Declaración de la función miner.
 *
 * Implementada en miner.c.
 *
 * @param rounds Número de rondas de minado.
 * @param num_threads Número de hilos a utilizar.
 * @param target_ini Target inicial para el minado.
 * @param pipe1_read Descriptor de lectura del primer pipe.
 * @param pipe1_write Descriptor de escritura del primer pipe.
 * @param pipe2_read Descriptor de lectura del segundo pipe.
 * @param pipe2_write Descriptor de escritura del segundo pipe.
 */
void miner(int rounds, int num_threads, long int target_ini, int pipe1_read, int pipe1_write, int pipe2_read, int pipe2_write);

/**
 * @brief Valida los parámetros de entrada.
 *
 * Verifica que se hayan proporcionado los argumentos correctos y que rounds y n_threads sean positivos.
 *
 * @param argc Número de argumentos.
 * @param argv Arreglo de argumentos.
 * @param target_ini Puntero para almacenar el target inicial.
 * @param rounds Puntero para almacenar el número de rondas.
 * @param n_threads Puntero para almacenar el número de hilos.
 */
void validate_input(int argc, char *argv[], long int *target_ini, int *rounds, int *n_threads) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <TARGET_INI> <ROUNDS> <N_THREADS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    *target_ini = atol(argv[1]);
    *rounds = atoi(argv[2]);
    *n_threads = atoi(argv[3]);

    if (*rounds <= 0 || *n_threads <= 0) {
        fprintf(stderr, "Rounds and/or n_threads need to be a positive number.\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Crea dos pipes para la comunicación entre procesos.
 *
 * @param pipe1 Array de dos enteros para el primer pipe.
 * @param pipe2 Array de dos enteros para el segundo pipe.
 */
void create_pipes(int pipe1[2], int pipe2[2]) {

    if (pipe(pipe1) == -1) {
        perror("Error creating pipe1");
        exit(EXIT_FAILURE);
    }
    if (pipe(pipe2) == -1) {
        perror("Error creating pipe2");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Función principal.
 *
 * Parsea los argumentos, crea los pipes, forka el proceso Minero y espera a que éste finalice.
 *
 * @param argc Número de argumentos.
 * @param argv Arreglo de argumentos.
 * @return EXIT_SUCCESS si todo sale correctamente, EXIT_FAILURE en caso contrario.
 */
int main(int argc, char *argv[]) {

    long int target;
    int rounds, n_threads;
    int pipe1[MAX_PIPE], pipe2[MAX_PIPE];
    pid_t miner_pid;
    int status;

    validate_input(argc, argv, &target, &rounds, &n_threads);

    create_pipes(pipe1, pipe2);

    miner_pid = fork();
    if (miner_pid < 0) {
        perror("Error in fork()");
        exit(EXIT_FAILURE);
    }

    if (miner_pid == 0) {
        miner(rounds, n_threads, target, pipe1[0], pipe1[1], pipe2[0], pipe2[1]);
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        exit(EXIT_SUCCESS);
    }

    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    waitpid(miner_pid, &status, 0);
    if (WIFEXITED(status))
        printf("Miner exited with status %d\n", WEXITSTATUS(status));
    else
        printf("Miner exited unexpectedly\n");

    return 0;
}
