#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 6
#define SHM_KEY 0x1234        // Clave para la memoria compartida (se puede generar con ftok)
#define MSG_KEY 0x5678        // Clave para la cola de mensajes (debe coincidir en miner.c)
#define TERMINATION_TARGET -1 // Valor especial para indicar terminación

typedef struct {
    long target;
    long solution;
    int accepted; // 1 si el bloque es correcto, 0 en caso contrario
} block_t;

typedef struct {
    block_t buffer[BUFFER_SIZE]; // Buffer circular
    int in;   // Índice para producción
    int out;  // Índice para consumo
    sem_t sem_empty; // Contabiliza espacios vacíos
    sem_t sem_fill;  // Contabiliza bloques disponibles
    sem_t sem_mutex; // Controla la exclusión mutua
} shm_struct;

typedef struct {
    long mtype;
    block_t block;
} msgbuf_t;

void error_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <LAG_ms>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int lag = atoi(argv[1]);
   
    int shmid;
    shm_struct *shm;
    int is_comprobador = 0;
   
    // Intentamos crear el segmento de memoria compartida.
    shmid = shmget(SHM_KEY, sizeof(shm_struct), IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1) {
        // Si ya existe, este proceso se comporta como Monitor.
        shmid = shmget(SHM_KEY, sizeof(shm_struct), 0666);
        if (shmid == -1) {
            error_exit("shmget");
        }
    } else {
        // Si se creó correctamente, este proceso es el Comprobador.
        is_comprobador = 1;
    }
   
    // Adjuntamos el segmento de memoria compartida.
    shm = (shm_struct *) shmat(shmid, NULL, 0);
    if (shm == (void *) -1) {
        error_exit("shmat");
    }
   
    if (is_comprobador) {
        printf("[%d] Checking blocks ...\n", getpid());
        // Inicializamos el buffer y los semáforos en la memoria compartida.
        shm->in = 0;
        shm->out = 0;
        if (sem_init(&shm->sem_empty, 1, BUFFER_SIZE) == -1) error_exit("sem_init sem_empty");
        if (sem_init(&shm->sem_fill, 1, 0) == -1) error_exit("sem_init sem_fill");
        if (sem_init(&shm->sem_mutex, 1, 1) == -1) error_exit("sem_init sem_mutex");
       
        // Abrimos la cola de mensajes para recibir mensajes del Minero.
        int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
        if (msgid == -1) {
            error_exit("msgget in comprobador");
        }
       
        msgbuf_t msg;
        while (1) {
            // Recibimos un mensaje (bloque) desde la cola.
            if (msgrcv(msgid, &msg, sizeof(block_t), 0, 0) == -1) {
                error_exit("msgrcv");
            }
            // Si se recibe el bloque especial de terminación, se pasa al buffer y se sale.
            if (msg.block.target == TERMINATION_TARGET) {
                sem_wait(&shm->sem_empty);
                sem_wait(&shm->sem_mutex);
                shm->buffer[shm->in] = msg.block;
                shm->in = (shm->in + 1) % BUFFER_SIZE;
                sem_post(&shm->sem_mutex);
                sem_post(&shm->sem_fill);
                break;
            }
           
            // Comprobación simple: se considera correcto si solution >= target.
            if (msg.block.solution >= msg.block.target)
                msg.block.accepted = 1;
            else
                msg.block.accepted = 0;
           
            // Inserción en el buffer compartido (operación de productor).
            sem_wait(&shm->sem_empty);
            sem_wait(&shm->sem_mutex);
            shm->buffer[shm->in] = msg.block;
            shm->in = (shm->in + 1) % BUFFER_SIZE;
            sem_post(&shm->sem_mutex);
            sem_post(&shm->sem_fill);
           
            usleep(lag * 1000); // Espera de LAG milisegundos
        }
       
        // Liberación de recursos: destrucción de semáforos y eliminación del segmento.
        sem_destroy(&shm->sem_empty);
        sem_destroy(&shm->sem_fill);
        sem_destroy(&shm->sem_mutex);
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);
        printf("[%d] Finishing\n", getpid());
       
    } else {
        printf("[%d] Printing blocks ...\n", getpid());
        // Proceso Monitor: extrae bloques del buffer y los muestra.
        block_t block;
        while (1) {
            sem_wait(&shm->sem_fill);
            sem_wait(&shm->sem_mutex);
            block = shm->buffer[shm->out];
            shm->out = (shm->out + 1) % BUFFER_SIZE;
            sem_post(&shm->sem_mutex);
            sem_post(&shm->sem_empty);
           
            // Si es el bloque de terminación, se sale del ciclo.
            if (block.target == TERMINATION_TARGET)
                break;
           
            // Se muestra el bloque según si fue aceptado o rechazado.
            if (block.accepted)
                printf("Solution accepted : %08ld --> %08ld\n", block.target, block.solution);
            else
                printf("Solution rejected : %08ld !-> %08ld\n", block.target, block.solution);
           
            usleep(lag * 1000);
        }
        shmdt(shm);
        printf("[%d] Finishing\n", getpid());
    }
    return 0;
}