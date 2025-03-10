#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>

#define FILENAME "votes.txt"
#define WAIT_MS 1
#define ROUND_WAIT_MS 250

int my_pid;
int *voter_pids;
int total_voters;
volatile sig_atomic_t system_ready = 0;
volatile sig_atomic_t ready_to_vote = 0;
sem_t *sem_vote;

void handle_SIGUSR1(int signo) {
    system_ready = 1;
}

void handle_SIGUSR2(int signo) {
    ready_to_vote = 1;
}

void handle_SIGINT(int signo) {
    printf("\nRicevuto SIGINT. Terminazione...\n");
    for (int i = 0; i < total_voters; i++) {
        kill(voter_pids[i], SIGTERM);
    }
    for (int i = 0; i < total_voters; i++) {
        waitpid(voter_pids[i], NULL, 0);
    }
    sem_unlink("/vote_semaphore");
    printf("Finishing by signal\n");
    exit(EXIT_SUCCESS);
}

void handle_SIGALRM(int signo) {
    printf("Tempo massimo raggiunto. Terminazione...\n");
    for (int i = 0; i < total_voters; i++) {
        kill(voter_pids[i], SIGTERM);
    }
    for (int i = 0; i < total_voters; i++) {
        waitpid(voter_pids[i], NULL, 0);
    }
    sem_unlink("/vote_semaphore");
    printf("Finishing by alarm\n");
    exit(EXIT_SUCCESS);
}

void wait_ms(int ms) {
    usleep(ms * 1000);
}

int select_candidate() {
    return voter_pids[0] == my_pid;
}

void cast_vote() {
    sem_wait(sem_vote);
    int fd = open(FILENAME, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        perror("Errore apertura file");
        exit(EXIT_FAILURE);
    }
    char vote = (rand() % 2) ? 'Y' : 'N';
    write(fd, &vote, 1);
    close(fd);
    sem_post(sem_vote);
}

void count_votes() {
    sem_wait(sem_vote);
    int fd = open(FILENAME, O_RDONLY);
    if (fd < 0) {
        perror("Errore apertura file");
        exit(EXIT_FAILURE);
    }

    char votes[total_voters + 1];
    memset(votes, 0, total_voters + 1);
    read(fd, votes, total_voters);
    close(fd);
    sem_post(sem_vote);

    int yes = 0, no = 0;
    for (int i = 0; i < total_voters; i++) {
        if (votes[i] == 'Y') yes++;
        else no++;
    }

    printf("Candidate %d => [ %s ] => %s\n", my_pid, votes, (yes > no) ? "Accepted" : "Rejected");
}

void voter_process() {
    signal(SIGUSR1, handle_SIGUSR1);
    signal(SIGUSR2, handle_SIGUSR2);
    signal(SIGTERM, exit);

    srand(time(NULL) + getpid());

    while (1) {
        while (!system_ready) pause();

        if (select_candidate()) {
            printf("Sono il candidato: %d\n", my_pid);

            int fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            close(fd);

            for (int i = 0; i < total_voters; i++) {
                kill(voter_pids[i], SIGUSR2);
            }

            int votes_collected = 0;
            while (votes_collected < total_voters) {
                wait_ms(WAIT_MS);
                votes_collected++;
            }

            count_votes();
            wait_ms(ROUND_WAIT_MS);
            system_ready = 0;
        } else {
            while (!ready_to_vote) pause();
            cast_vote();
            ready_to_vote = 0;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <N_PROCS> <N_SECS>\n", argv[0]);
        return EXIT_FAILURE;
    }

    total_voters = atoi(argv[1]);
    int N_SECS = atoi(argv[2]);

    if (total_voters <= 0) {
        fprintf(stderr, "Numero di votanti non valido.\n");
        return EXIT_FAILURE;
    }

    voter_pids = malloc(total_voters * sizeof(int));
    if (!voter_pids) {
        perror("Errore allocazione memoria");
        return EXIT_FAILURE;
    }

    sem_vote = sem_open("/vote_semaphore", O_CREAT, 0644, 1);

    FILE *file = fopen("voting_pids.txt", "w");
    if (!file) {
        perror("Errore apertura file");
        free(voter_pids);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < total_voters; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Errore in fork");
            free(voter_pids);
            fclose(file);
            return EXIT_FAILURE;
        }
        if (pid == 0) {
            my_pid = getpid();
            voter_process();
            exit(EXIT_SUCCESS);
        } else {
            voter_pids[i] = pid;
            fprintf(file, "%d\n", pid);
        }
    }

    fclose(file);
    signal(SIGINT, handle_SIGINT);
    signal(SIGALRM, handle_SIGALRM);
    alarm(N_SECS);

    printf("Sistema pronto. Avvio votazione...\n");

    for (int i = 0; i < total_voters; i++) {
        kill(voter_pids[i], SIGUSR1);
    }

    while (1) {
        pause();
    }

    return EXIT_SUCCESS;
}
