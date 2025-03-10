#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

/* Handler function for the signal SIGINT. */
void handler(int sig) {
  printf("Signal number %d received\n", sig);
  fflush(stdout);
}

int main(void) {
  struct sigaction act;

  act.sa_handler = handler;
  sigemptyset(&(act.sa_mask));
  act.sa_flags = 0;

  /*
  if (sigaction(SIGINT, &act, NULL) < 0) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }*/
  for (int i = 1; i <= 31; i++) {
    if (i == 9 || i == 19) continue;  // SIGKILL (9) y SIGSTOP (19) no pueden ser manejadas
    if (sigaction(i, &act, NULL) < 0) {
        printf("Cannot catch signal %d\n", i);
    }
  }
  printf("PID = %d. Waiting for signals...\n", getpid());

  while (1) {
    printf("Waiting Ctrl+C (PID = %d)\n", getpid());
    sleep(9999);
  }
}
