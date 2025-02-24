#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pow.h"

void monitor(int pipe1_read, int pipe2_write) {

    long int target, solution;
    int check;
    
    while (read(pipe1_read, &target, sizeof(long int)) > 0) {
        if (read(pipe1_read, &solution, sizeof(long int)) != sizeof(long int)) {
            perror("Error reading solution");
            exit(EXIT_FAILURE);
        }
        if (pow_hash(solution) == target) {
            printf("Solution accepted: %08ld --> %08ld", target, solution);
            printf("\n");
            check = 0;
        } else {
            printf("Solution rejected: %08ld !-> %08ld", target, solution);
            printf("\n");
            check = 1;
        }
        if (write(pipe2_write, &check, sizeof(int)) != sizeof(int)) {
            perror("Error writing confirmation in monitor");
            exit(EXIT_FAILURE);
        }
    }
}
