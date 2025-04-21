#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <semaphore.h>
#include "pow.h"

#define MSG_KEY        0x3000
#define SYS_KEY        0x1000
#define MON_KEY        0x2000
#define MAX_MINERS     100
#define TERMINATION_ID -1

typedef struct {
    pid_t  pid;
    int    coins;
} wallet_t;

typedef struct {
    sem_t      mutex;                   // exclusión mutua
    int        next_block_id;          // contador global de bloques
    int        num_miners;             // mineros activos
    pid_t      miners[MAX_MINERS];     // PIDs
    int        coins[MAX_MINERS];      // monedas por minero
} system_state_t;

typedef struct {
    int     id;
    long    target;
    long    solution;
    pid_t   winner_pid;
    int     votes_yes;
    int     votes_total;
    int     num_wallets;
    wallet_t wallets[MAX_MINERS];
} block_t;

typedef struct {
    long    mtype;
    block_t blk;
} msgbuf_t;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ROUNDS> <THREADS>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int rounds = atoi(argv[1]);
    (void)argv;

    // 1) Inicializar/conectar memoria compartida de sistema
    int sys_shmid = shmget(SYS_KEY, sizeof(system_state_t),
                          IPC_CREAT | IPC_EXCL | 0666);
    system_state_t *sys;
    if (sys_shmid >= 0) {
        sys = shmat(sys_shmid, NULL, 0);
        sem_init(&sys->mutex, 1, 1);
        sys->next_block_id = 0;
        sys->num_miners = 0;
    } else {
        sys_shmid = shmget(SYS_KEY, sizeof(system_state_t), 0666);
        sys = shmat(sys_shmid, NULL, 0);
    }

    // 2) Registrar este minero
    sem_wait(&sys->mutex);
    int my_idx = sys->num_miners;
    sys->miners[my_idx] = getpid();
    sys->coins[my_idx]  = 0;
    sys->num_miners++;
    sem_post(&sys->mutex);

    // 3) Preparar cola de mensajes
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid < 0) { perror("msgget"); return EXIT_FAILURE; }

    printf("[%d] Generating blocks ...\n", getpid());

    long target = 0;
    for (int r = 0; r < rounds; ++r) {
        // 4) Asignar ID global y snapshot de wallets
        sem_wait(&sys->mutex);
        int id        = sys->next_block_id++;
        int nm        = sys->num_miners;
        block_t blk;
        blk.id        = id;
        blk.target    = target;
        blk.winner_pid= getpid();
        blk.votes_yes = nm;
        blk.votes_total = nm;
        blk.num_wallets = nm;
        for (int i = 0; i < nm; ++i) {
            blk.wallets[i].pid   = sys->miners[i];
            blk.wallets[i].coins = sys->coins[i];
        }
        sem_post(&sys->mutex);

        // 5) Resolución POW
        long sol = pow_hash(target);
        blk.solution = sol;

        // 6) Actualizar coins del ganador
        sem_wait(&sys->mutex);
        sys->coins[my_idx]++;
        sem_post(&sys->mutex);

        // 7) Enviar bloque
        msgbuf_t msg = { .mtype = 1, .blk = blk };
        if (msgsnd(msgid, &msg, sizeof(block_t), 0) < 0)
            perror("msgsnd");

        target = sol;
        usleep(100000);
    }

    printf("[%d] Finishing\n", getpid());

    // 8) Desregistrar y, si soy el último, enviar terminación
    sem_wait(&sys->mutex);
    sys->num_miners--;
    int last = (sys->num_miners == 0);
    sem_post(&sys->mutex);

    if (last) {
        block_t endb = { .id = TERMINATION_ID };
        msgbuf_t endmsg = { .mtype = 1, .blk = endb };
        msgsnd(msgid, &endmsg, sizeof(block_t), 0);
        msgctl(msgid, IPC_RMID, NULL);
        shmdt(sys);
        shmctl(sys_shmid, IPC_RMID, NULL);
    } else {
        shmdt(sys);
    }

    return EXIT_SUCCESS;
}