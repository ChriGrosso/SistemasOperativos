#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>
#include "pow.h"

#define MQ_NAME        "/block_queue"
#define SHM_NAME       "/system_shm"
#define MAX_MINERS     100
#define TERMINATION_ID -1

typedef struct {
    pid_t  pid;
    int    coins;
} wallet_t;

typedef struct {
    sem_t   mutex;
    int     next_block_id;
    int     num_miners;
    pid_t   miners[MAX_MINERS];
    int     coins[MAX_MINERS];
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

// wrapper para sem_wait que reintenta tras EINTR
static void sem_wait_eintr(sem_t *s) {
    while (sem_wait(s) == -1 && errno == EINTR) { /* retry */ }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ROUNDS> <THREADS>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int rounds = atoi(argv[1]);

    // 1) shm POSIX
    int fd = shm_open(SHM_NAME, O_CREAT|O_EXCL|O_RDWR, 0666);
    system_state_t *sys;
    if (fd >= 0) {
        ftruncate(fd, sizeof *sys);
        sys = mmap(NULL, sizeof *sys, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        sem_init(&sys->mutex, 1, 1);
        sys->next_block_id = 0;
        sys->num_miners    = 0;
    } else {
        fd = shm_open(SHM_NAME, O_RDWR, 0666);
        sys = mmap(NULL, sizeof *sys, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    }

    // 2) registro
    sem_wait_eintr(&sys->mutex);
    int idx = sys->num_miners;
    sys->miners[idx] = getpid();
    sys->coins[idx]  = 0;
    sys->num_miners++;
    sem_post(&sys->mutex);

    // 3) cola de mensajes POSIX
    struct mq_attr attr = { .mq_flags = 0, .mq_maxmsg = 10, .mq_msgsize = sizeof(block_t) };
    mqd_t mq = mq_open(MQ_NAME, O_CREAT|O_WRONLY, 0666, &attr);
    if (mq == (mqd_t)-1) { perror("mq_open"); return EXIT_FAILURE; }

    printf("[%d] Generating blocks ...\n", getpid());
    long target = 0;
    for (int r = 0; r < rounds; ++r) {
        long sol = pow_hash(target);

        sem_wait_eintr(&sys->mutex);
        int id = sys->next_block_id++;
        sys->coins[idx]++;
        int nm = sys->num_miners;
        block_t blk = {
            .id           = id,
            .target       = target,
            .solution     = sol,
            .winner_pid   = getpid(),
            .votes_yes    = nm,
            .votes_total  = nm,
            .num_wallets  = nm
        };
        for (int i = 0; i < nm; ++i) {
            blk.wallets[i].pid   = sys->miners[i];
            blk.wallets[i].coins = sys->coins[i];
        }
        sem_post(&sys->mutex);

        mq_send(mq, (char*)&blk, sizeof blk, 0);
        target = sol;
        usleep(100000);
    }

    printf("[%d] Finishing\n", getpid());

    sem_wait_eintr(&sys->mutex);
    sys->num_miners--;
    int last = (sys->num_miners == 0);
    sem_post(&sys->mutex);

    if (last) {
        block_t endb = { .id = TERMINATION_ID };
        mq_send(mq, (char*)&endb, sizeof endb, 0);
        mq_close(mq); mq_unlink(MQ_NAME);
        munmap(sys, sizeof *sys); shm_unlink(SHM_NAME);
    } else {
        mq_close(mq);
        munmap(sys, sizeof *sys);
    }

    return EXIT_SUCCESS;
}
