#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define MSG_KEY 0x5678        // Debe coincidir con la clave usada en monitor.c
#define TERMINATION_TARGET -1 // Bloque especial para indicar finalización

typedef struct {
    long target;
    long solution;
    int accepted; // Este campo se establecerá en el Comprobador
} block_t;

typedef struct {
    long mtype;
    block_t block;
} msgbuf_t;

void error_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Función para simular la prueba de trabajo (Proof-of-Work)
// Para efectos de la práctica se genera una solución aleatoria mayor que el target.
long generate_solution(long target) {
    return target + (rand() % 100000000 + 1);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ROUNDS> <LAG_ms>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int rounds = atoi(argv[1]);
    int lag = atoi(argv[2]);
   
    // Inicializar semilla aleatoria.
    srand(time(NULL));
   
    // Creamos la cola de mensajes.
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        error_exit("msgget");
    }
   
    block_t current;
    current.target = 0; // Objetivo inicial
   
    msgbuf_t msg;
    msg.mtype = 1;
   
    printf("[%d] Generating blocks ...\n", getpid());
   
    for (int i = 0; i < rounds; i++) {
        current.solution = generate_solution(current.target);
        // El campo 'accepted' se deja para que lo establezca el Comprobador.
        msg.block = current;
       
        if (msgsnd(msgid, &msg, sizeof(block_t), 0) == -1) {
            error_exit("msgsnd");
        }
       
        // El siguiente objetivo es la solución hallada.
        current.target = current.solution;
       
        usleep(lag * 1000); // Espera de LAG milisegundos.
    }
   
    // Enviar el bloque de terminación.
    current.target = TERMINATION_TARGET;
    msg.block = current;
    if (msgsnd(msgid, &msg, sizeof(block_t), 0) == -1) {
        error_exit("msgsnd termination");
    }
   
    // Se espera un instante para asegurar que el Comprobador procesa el mensaje.
    sleep(1);
    // Se elimina la cola de mensajes.
    //msgctl(msgid, IPC_RMID, NULL);
   
    printf("[%d] Finishing\n", getpid());
    return 0;
}