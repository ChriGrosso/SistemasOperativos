#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pow.h"

// El Monitor lee los pares (target, solución), valida la solución y
// envía una confirmación de vuelta al Minero.
void monitor(int pipe_m2mon_read, int pipe_mon2m_write) {
    long int target, solution;
    int confirmation;
    while (read(pipe_m2mon_read, &target, sizeof(long int)) > 0) {
        if (read(pipe_m2mon_read, &solution, sizeof(long int)) != sizeof(long int)) {
            perror("Error leyendo solución en monitor");
            exit(EXIT_FAILURE);
        }
        if (pow_hash(solution) == target) {
            printf("Solution accepted: %08ld --> %08ld\n", target, solution);
            confirmation = 0;
        } else {
            printf("Solution rejected: %08ld !-> %08ld\n", target, solution);
            confirmation = 1;
        }
        if (write(pipe_mon2m_write, &confirmation, sizeof(int)) != sizeof(int)) {
            perror("Error escribiendo confirmación en monitor");
            exit(EXIT_FAILURE);
        }
    }
}
