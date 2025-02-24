/**
 * @file miner.c
 * @brief Implementación del proceso Minero usando hilos y procesos.
 *
 * Este archivo implementa la lógica del proceso Minero, que divide el espacio de búsqueda
 * entre varios hilos para resolver el POW. Además, crea un proceso Monitor para validar
 * la solución encontrada a través de pipes.
 *
 * @author Christian y Pablo
 * @date 2025-02-21
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "pow.h"

/**
 * @brief Estructura que contiene los datos para cada hilo de minado.
 *
 * - target_ini: Target inicial para el POW.
 * - beginning: Inicio del rango asignado al hilo.
 * - finish: Fin del rango asignado al hilo.
 * - discover: Indicador de si se encontró la solución en ese rango.
 * - solution: Solución hallada, en caso de encontrarla.
 */
typedef struct {
    long int target_ini;
    long int beginning;
    long int finish;
    int discover;
    long int solution;
} Miner;

/**
 * @brief Función de búsqueda para cada hilo.
 *
 * Recorre el rango [beginning, finish) y comprueba si para algún valor se cumple que
 * pow_hash(valor) == target_ini. Si se encuentra, se marca discover y se almacena la solución.
 *
 * @param arg Puntero a una estructura Miner.
 * @return Siempre retorna NULL.
 */
void *mine(void *arg) {

    Miner *data = (Miner *)arg;
    long int i = 0;
   
    for (i = data->beginning; i < data->finish; i++) {
        if (pow_hash(i) == data->target_ini) {
            data->discover = 1;
            data->solution = i;
            break;
        }
    }
   
    return NULL;
}

/**
 * @brief Declaración de la función monitor.
 *
 * Implementada en monitor.c.
 *
 * @param pipe1_read Descriptor de lectura del primer pipe.
 * @param pipe2_write Descriptor de escritura del segundo pipe.
 */
void monitor(int pipe1_read, int pipe2_write);

/**
 * @brief Crea los hilos para el minado.
 *
 * Divide el espacio de búsqueda en partes iguales para cada hilo y los crea.
 *
 * @param threads Array de hilos (pthread_t).
 * @param data Array de estructuras Miner.
 * @param num_threads Número de hilos a crear.
 * @param target Valor target para la búsqueda.
 */
void threadsCreate(pthread_t *threads, Miner *data, int num_threads, long int target) {

    long int range = POW_LIMIT / num_threads;
    int i = 0;

    for (i = 0; i < num_threads; i++) {
        data[i].target_ini = target;
        data[i].beginning = i * range;
        data[i].finish = (i == num_threads - 1) ? POW_LIMIT : (i + 1) * range;
        data[i].discover = 0;
        pthread_create(&threads[i], NULL, mine, &data[i]);
    }
}

/**
 * @brief Une los hilos y comprueba si se encontró la solución.
 *
 * Espera a que cada hilo finalice y, si alguno encontró la solución, actualiza la variable found
 * y almacena la solución encontrada.
 *
 * @param threads Array de hilos (pthread_t).
 * @param data Array de estructuras Miner.
 * @param num_threads Número de hilos.
 * @param found Puntero a entero que se actualiza a 1 si se encontró la solución.
 * @param solution Puntero a long int donde se almacena la solución.
 */
void threadsJoin(pthread_t *threads, Miner *data, int num_threads, int *found, long int *solution) {

    int i = 0;

    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        if (data[i].discover) {
            *found = 1;
            *solution = data[i].solution;
        }
    }
}

/**
 * @brief Función principal del proceso Minero.
 *
 * Realiza un número de rondas de minado. En cada ronda, crea hilos para buscar la solución,
 * envía el par (target, solución) al Monitor a través de pipes, espera la confirmación y
 * actualiza el target para la siguiente ronda.
 *
 * @param rounds Número de rondas de minado.
 * @param n_threads Número de hilos a utilizar.
 * @param target_ini Target inicial para el minado.
 * @param pipe1_read Descriptor de lectura del primer pipe.
 * @param pipe1_write Descriptor de escritura del primer pipe.
 * @param pipe2_read Descriptor de lectura del segundo pipe.
 * @param pipe2_write Descriptor de escritura del segundo pipe.
 */
void miner(int rounds, int n_threads, long int target_ini, int pipe1_read, int pipe1_write, int pipe2_read, int pipe2_write) {
           
    int status = 0;
    int round = 0;
    pid_t monitor_pid;

    monitor_pid = fork();
    if (monitor_pid < 0) {
        perror("Error in fork() for monitor");
        exit(EXIT_FAILURE);
    }
    if (monitor_pid == 0) {
        close(pipe1_write);
        close(pipe2_read);
        monitor(pipe1_read, pipe2_write);
        close(pipe1_read);
        close(pipe2_write);
        exit(EXIT_SUCCESS);
    }

    close(pipe1_read);
    close(pipe2_write);

    for (round = 0; round < rounds; round++) {

        pthread_t threads[n_threads];
        Miner data[n_threads];
        int discover = 0;
        long int solution = -1;
        int confirmation;
        int status;

        threadsCreate(threads, data, n_threads, target_ini);
        threadsJoin(threads, data, n_threads, &discover, &solution);

        if (discover) {
            if (write(pipe1_write, &target_ini, sizeof(long int)) != sizeof(long int) || write(pipe1_write, &solution, sizeof(long int)) != sizeof(long int)) {
                perror("Error writing to monitor");
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "No solution discovered\n");
            exit(EXIT_FAILURE);
        }

        if (read(pipe2_read, &confirmation, sizeof(int)) != sizeof(int)) {
            perror("Error reading from monitor");
            exit(EXIT_FAILURE);
        }
        if (confirmation != 0) {
            printf("The solution has been invalidated\n");
            close(pipe1_write);
            close(pipe2_read);
            waitpid(monitor_pid, &status, 0);
            if (WIFEXITED(status))
                printf("Monitor exited with status %d\n", WEXITSTATUS(status));
            else
                printf("Monitor exited unexpectedly\n");
            exit(EXIT_FAILURE);
        }
        target_ini = solution;
    }

    close(pipe1_write);
    close(pipe2_read);

    waitpid(monitor_pid, &status, 0);
    if (WIFEXITED(status))
        printf("Monitor exited with status %d\n", WEXITSTATUS(status));
    else
        printf("Monitor exited unexpectedly\n");
}
