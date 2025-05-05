#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define MQ_NAME       "/block_queue"
#define SHM_NAME      "/monitor_shm"
#define MAX_MINERS    100
#define BUFFER_SIZE   5
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
    block_t buffer[BUFFER_SIZE];
    int     in, out;
    sem_t   empty, full, mutex;
} mon_shm_t;

static void sem_wait_eintr(sem_t *s) {
    while (sem_wait(s) == -1 && errno == EINTR) { /* retry */ }
}

int main(void) {
    // 1) POSIX shm para buffer
    int fd = shm_open(SHM_NAME, O_CREAT|O_EXCL|O_RDWR, 0666);
    mon_shm_t *shm;
    if (fd >= 0) {
        ftruncate(fd, sizeof *shm);
        shm = mmap(NULL, sizeof *shm, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        sem_init(&shm->empty, 1, BUFFER_SIZE);
        sem_init(&shm->full,  1, 0);
        sem_init(&shm->mutex, 1, 1);
        shm->in = shm->out = 0;
    } else {
        fd = shm_open(SHM_NAME, O_RDWR, 0666);
        shm = mmap(NULL, sizeof *shm, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    }

    // fork: padre(comprobador) / hijo(monitor)
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(EXIT_FAILURE); }

    if (pid > 0) {
        // Comprobador
        mqd_t mq = mq_open(MQ_NAME, O_RDONLY);
        if (mq == (mqd_t)-1) { perror("mq_open"); exit(EXIT_FAILURE); }
        printf("[%d] Checking blocks ...\n", getpid());
        while (1) {
            block_t blk;
            if (mq_receive(mq, (char*)&blk, sizeof blk, NULL) < 0) {
                perror("mq_receive");
                break;
            }
            sem_wait_eintr(&shm->empty);
            sem_wait_eintr(&shm->mutex);
            shm->buffer[shm->in] = blk;
            shm->in = (shm->in + 1) % BUFFER_SIZE;
            sem_post(&shm->mutex);
            sem_post(&shm->full);

            if (blk.id == TERMINATION_ID) break;
        }
        printf("[%d] Finishing\n", getpid());
        mq_close(mq); mq_unlink(MQ_NAME);
        sem_destroy(&shm->empty);
        sem_destroy(&shm->full);
        sem_destroy(&shm->mutex);
        munmap(shm, sizeof *shm); shm_unlink(SHM_NAME);
        wait(NULL);
        return EXIT_SUCCESS;

    } else {
        // Monitor de impresión
        block_t b;
        while (1) {
            sem_wait_eintr(&shm->full);
            sem_wait_eintr(&shm->mutex);
            b = shm->buffer[shm->out];
            shm->out = (shm->out + 1) % BUFFER_SIZE;
            sem_post(&shm->mutex);
            sem_post(&shm->empty);

            if (b.id == TERMINATION_ID) break;

            printf("Id : %04d\n", b.id);
            printf("Winner : %d\n", b.winner_pid);
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
