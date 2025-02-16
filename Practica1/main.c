#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "pow.h"

int main(int argc , char * argv []) {
    int i,r,h,o;
    if (argc < 3) {
        fprintf(stderr, "Error in the input parameters:\n\n");
        fprintf(stderr, "%s -r <int> -h <int> -p <int>\n", argv[0]);
        fprintf(stderr, "Where:\n");
        fprintf(stderr, "-r: numero de rondas de minado que debe realizar\n");
        fprintf(stderr, "-h: numero de hilos que debe utilizar\n");
        fprintf(stderr, "-o: objetivo del problema inicial que debe resolver\n");
        exit(-1);
    }
    for(i = 1; i < argc ; i++) {
        if (strcmp(argv[i], "-r") == 0)
            r = atoi(argv[++i]);
        else if (strcmp(argv[i], "-h") == 0)
            h = atoi(argv[++i]);
        else if (strcmp(argv[i], "-o") == 0)
            o = atoi(argv[++i]);
        else {
            fprintf(stderr, "Parameter %s is invalid\n", argv[i]);
            exit(-1);
        }
    }
    
    return 0;
}