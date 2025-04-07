#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define MSG_KEY 0x5678
#define TERMINATION_OBJECTIVE -1

typedef struct {
    long objective;
    long solution;
    int accepted;
} block_t;

typedef struct {
    long mtype;
    block_t block;
} msgbuf_t;

void error_exit(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

long generate_solution(long objective) {
    return objective + (rand() % 100000000 + 1);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ROUNDS> <LAG_ms>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int rounds = atoi(argv[1]);
    int lag = atoi(argv[2]);
    int messageId;
    block_t actual;
    msgbuf_t message;
    int i = 0;
   
    srand(time(NULL));
   
    messageId = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (messageId == -1) {
        error_exit("msgget");
    }
   
    actual.objective = 0;
    
    message.mtype = 1;
   
    printf("[%d] Generating blocks ...\n", getpid());
   
    for (i = 0; i < rounds; i++) {
        actual.solution = generate_solution(actual.objective);
        message.block = actual;
       
        if (msgsnd(messageId, &message, sizeof(block_t), 0) == -1) {
            error_exit("msgsnd");
        }
       
        actual.objective = actual.solution;
       
        usleep(lag * 1000);
    }
   
    actual.objective = TERMINATION_OBJECTIVE;
    message.block = actual;
    if (msgsnd(messageId, &message, sizeof(block_t), 0) == -1) {
        error_exit("msgsnd termination");
    }
   
    sleep(1);
   
    printf("[%d] Finishing\n", getpid());
    return 0;
}
