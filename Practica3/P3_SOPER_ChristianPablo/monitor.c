#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 6
#define SHM_KEY 0x1234
#define MSG_KEY 0x5678
#define TERMINATION_OBJECTIVE -1

typedef struct {
    long objective;
    long solution;
    int accepted;
} block_t;

typedef struct {
    block_t buffer[BUFFER_SIZE];
    int in;
    int out;
    sem_t sem_empty;
    sem_t sem_fill;
    sem_t sem_mutex;
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
    int messageId;
    msgbuf_t message;
    block_t block;
   
    shmid = shmget(SHM_KEY, sizeof(shm_struct), IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1) {
        shmid = shmget(SHM_KEY, sizeof(shm_struct), 0666);
        if (shmid == -1) {
            error_exit("shmget");
        }
    } else {
        is_comprobador = 1;
    }
   
    shm = (shm_struct *) shmat(shmid, NULL, 0);
    if (shm == (void *) -1) {
        error_exit("shmat");
    }
   
    if (is_comprobador) {
        printf("[%d] Checking blocks ...\n", getpid());
        shm->in = 0;
        shm->out = 0;
        if (sem_init(&shm->sem_empty, 1, BUFFER_SIZE) == -1) error_exit("sem_init sem_empty");
        if (sem_init(&shm->sem_fill, 1, 0) == -1) error_exit("sem_init sem_fill");
        if (sem_init(&shm->sem_mutex, 1, 1) == -1) error_exit("sem_init sem_mutex");
       
        messageId = msgget(MSG_KEY, IPC_CREAT | 0666);
        if (messageId == -1) {
            error_exit("msgget in comprobador");
        }
       
        while (1) {
            if (msgrcv(messageId, &message, sizeof(block_t), 0, 0) == -1) {
                error_exit("msgrcv");
            }
            if (message.block.objective == TERMINATION_OBJECTIVE) {
                sem_wait(&shm->sem_empty);
                sem_wait(&shm->sem_mutex);
                shm->buffer[shm->in] = message.block;
                shm->in = (shm->in + 1) % BUFFER_SIZE;
                sem_post(&shm->sem_mutex);
                sem_post(&shm->sem_fill);
                
                if (msgctl(messageId, IPC_RMID, NULL) == -1){
                    perror("msgct elimination");
                }
                break;
            }
           
            if (message.block.solution >= message.block.objective)
                message.block.accepted = 1;
            else
                message.block.accepted = 0;
           
            sem_wait(&shm->sem_empty);
            sem_wait(&shm->sem_mutex);
            shm->buffer[shm->in] = message.block;
            shm->in = (shm->in + 1) % BUFFER_SIZE;
            sem_post(&shm->sem_mutex);
            sem_post(&shm->sem_fill);
           
            usleep(lag * 1000);
        }
       
        sem_destroy(&shm->sem_empty);
        sem_destroy(&shm->sem_fill);
        sem_destroy(&shm->sem_mutex);
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);
        printf("[%d] Finishing\n", getpid());
       
    } else {
        printf("[%d] Printing blocks ...\n", getpid());
        while (1) {
            sem_wait(&shm->sem_fill);
            sem_wait(&shm->sem_mutex);
            block = shm->buffer[shm->out];
            shm->out = (shm->out + 1) % BUFFER_SIZE;
            sem_post(&shm->sem_mutex);
            sem_post(&shm->sem_empty);
           
            if (block.objective == TERMINATION_OBJECTIVE)
                break;
           
            if (block.accepted)
                printf("Solution accepted : %08ld --> %08ld\n", block.objective, block.solution);
            else
                printf("Solution rejected : %08ld !-> %08ld\n", block.objective, block.solution);
           
            usleep(lag * 1000);
        }
        shmdt(shm);
        printf("[%d] Finishing\n", getpid());
    }
    return 0;
}
