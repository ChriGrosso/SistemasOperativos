#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>

#define MON_KEY         0x2000
#define MSG_KEY         0x3000
#define MAX_MINERS      100
#define MON_BUFFER      5
#define TERMINATION_ID -1

typedef struct {
    int     id;
    long    target;
    long    solution;
    pid_t   winner_pid;
    int     votes_yes;
    int     votes_total;
    int     num_wallets;
    struct { pid_t pid; int coins; } wallets[MAX_MINERS];
} block_t;

typedef struct {
    block_t buffer[MON_BUFFER];
    int     in, out;
    sem_t   empty, full, mutex;
} mon_shm_t;

typedef struct {
    long    mtype;
    block_t blk;
} msgbuf_t;

int main(void) {
    // 1) Crear o conectar el segmento de memoria compartida de monitoreo
    int shmid = shmget(MON_KEY, sizeof(mon_shm_t), IPC_CREAT | IPC_EXCL | 0666);
    mon_shm_t *shm;
    if (shmid >= 0) {
        shm = shmat(shmid, NULL, 0);
        sem_init(&shm->empty, 1, MON_BUFFER);
        sem_init(&shm->full,  1, 0);
        sem_init(&shm->mutex, 1, 1);
        shm->in = shm->out = 0;
    } else {
        shmid = shmget(MON_KEY, sizeof(mon_shm_t), 0666);
        shm = shmat(shmid, NULL, 0);
    }

    // 2) Fork: proceso padre = Comprobador, hijo = Monitor de impresión
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        // ---- Comprobador ----
        int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
        if (msgid < 0) {
            perror("msgget");
            exit(EXIT_FAILURE);
        }

        printf("[%d] Checking blocks ...\n", getpid());
        msgbuf_t msg;
        while (1) {
            // Recibir bloque (bloqueante)
            if (msgrcv(msgid, &msg, sizeof(block_t), 0, 0) < 0) {
                perror("msgrcv");
                break;
            }
            // Insertar en buffer
            sem_wait(&shm->empty);
            sem_wait(&shm->mutex);
            shm->buffer[shm->in] = msg.blk;
            shm->in = (shm->in + 1) % MON_BUFFER;
            sem_post(&shm->mutex);
            sem_post(&shm->full);

            // Si es bloque de terminación, salir
            if (msg.blk.id == TERMINATION_ID) {
                break;
            }
        }

        printf("[%d] Finishing\n", getpid());
        // Limpieza
        msgctl(msgid, IPC_RMID, NULL);
        sem_destroy(&shm->empty);
        sem_destroy(&shm->full);
        sem_destroy(&shm->mutex);
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);

        // Esperar a que el hijo termine
        wait(NULL);
        return EXIT_SUCCESS;
    } else {
        // ---- Monitor de impresión ----
        block_t b;
        while (1) {
            sem_wait(&shm->full);
            sem_wait(&shm->mutex);
            b = shm->buffer[shm->out];
            shm->out = (shm->out + 1) % MON_BUFFER;
            sem_post(&shm->mutex);
            sem_post(&shm->empty);

            if (b.id == TERMINATION_ID) {
                break;
            }

            printf("Id : %04d\n",      b.id);
            printf("Winner : %d\n",    b.winner_pid);
            printf("Target : %08ld\n", b.target);
            printf("Solution : %08ld ( validated )\n", b.solution);
            printf("Votes : %d/%d\n", b.votes_yes, b.votes_total);
            printf("Wallets :");
            for (int i = 0; i < b.num_wallets; ++i) {
                printf(" %d:%02d", b.wallets[i].pid, b.wallets[i].coins);
            }
            printf("\n\n");
        }
        return EXIT_SUCCESS;
    }
}
