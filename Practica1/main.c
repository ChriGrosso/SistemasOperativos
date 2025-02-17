#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "pow.h"

int main(int argc , char * argv []) {
    pthread_t thread[20];
    //9997697
    int t,r,n,o,error,i;
    if (argc < 3) {
        fprintf(stderr, "Error in the input parameters:\n\n");
        fprintf(stderr, "%s <TARGET_INI> <ROUNDS> <N_THREADS>\n", argv[0]);
        exit(-1);
    } 
    t = atol(argv[1]);
    r = atoi(argv[2]);
    n = atoi(argv[3]);
    for(i=1;i<=r;i++){//numero de rondas
        
    }
    // for(int i = 0; i<20;i++){
    //     error = pthread_create(&thread[i], NULL, pow_hash,o);
    //     if (error != 0) {
    //         fprintf(stderr, "pthread_create: %s\n", strerror(error));
    //         exit(EXIT_FAILURE);
    //     }
    // }
    return 0;
}

int minar(int from, int to, int res){
    int i,error;
    int * output;
    pthread_t thread;
    
    for(i=from; i<to; i++){
        error = pthread_create(&thread, NULL, pow_hash,i);
        if (error != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            exit(EXIT_FAILURE);
        }
        // Attendere la terminazione del thread e ottenere il valore restituito
    if (pthread_join(thread, (void**)&output) != 0) {
        perror("Errore nella join del thread");
        return EXIT_FAILURE;
    }
    }
}
