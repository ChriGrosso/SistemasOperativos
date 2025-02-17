#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pow.h"

void monitor(int fd_read) {
    long int target, solution;
    while (read(fd_read, &target, sizeof(long int)) > 0) {
        read(fd_read, &solution, sizeof(long int));

        if (pow_hash(solution) == target) {
            printf("Solution accepted: %08ld --> %08ld\n", target, solution);
        } else {
            printf("Solution rejected: %08ld !-> %08ld\n", target, solution);
            exit(EXIT_FAILURE);
        }
    }
}
