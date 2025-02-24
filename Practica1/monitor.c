/**
 * @file monitor.c
 * @brief Implementación del proceso Monitor.
 *
 * Este archivo implementa la función Monitor, que lee pares (target, solución)
 * enviados por el proceso Minero a través de un pipe, valida la solución mediante
 * la función pow_hash, y envía una confirmación de vuelta al Minero.
 *
 * @author Christian y Pablo
 * @date 2025-02-21
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pow.h"

/**
 * @brief Función Monitor.
 *
 * Lee de forma continua los pares (target, solución) desde el pipe, valida la solución
 * comprobando que pow_hash(solución) sea igual al target, imprime un mensaje de aceptación
 * o rechazo, y escribe una confirmación (0 para aceptada, 1 para rechazada) en el otro pipe.
 *
 * @param pipe1_read Descriptor de lectura del pipe de entrada.
 * @param pipe2_write Descriptor de escritura del pipe para enviar la confirmación.
 */
void monitor(int pipe1_read, int pipe2_write) {

    long int target, solution;
    int check;
   
    while (read(pipe1_read, &target, sizeof(long int)) > 0) {
        if (read(pipe1_read, &solution, sizeof(long int)) != sizeof(long int)) {
            perror("Error reading solution");
            exit(EXIT_FAILURE);
        }
        if (pow_hash(solution) == target) {
            printf("Solution accepted: %08ld --> %08ld\n", target, solution);
            check = 0;
        } else {
            printf("Solution rejected: %08ld !-> %08ld\n", target, solution);
            check = 1;
        }
        if (write(pipe2_write, &check, sizeof(int)) != sizeof(int)) {
            perror("Error writing confirmation in monitor");
            exit(EXIT_FAILURE);
        }
    }
}
