#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_PROC 3

//A
// int main(void) {
//   int i;
//   pid_t pid;

//   for (i = 0; i < NUM_PROC; i++) {
//     pid = fork();
//     if (pid < 0) {
//       perror("fork");
//       exit(EXIT_FAILURE);
//     } else if (pid == 0) {
//       printf("Child %d\n", i);
//       exit(EXIT_SUCCESS);
//     } else if (pid > 0) {
//       printf("Parent %d\n", i);
//     }
//   }
//   wait(NULL);
//   exit(EXIT_SUCCESS);
// }

//B

// int main(void) {
//   int i;
//   pid_t pid;

//   for (i = 0; i < NUM_PROC; i++) {
//     pid_t pid_padre=getpid();
//     pid = fork();
//     if (pid < 0) {
//       perror("fork");
//       exit(EXIT_FAILURE);
//     } else if (pid == 0) {
//       printf("Child %d, padre %d\n", getpid,pid_padre);
//       exit(EXIT_SUCCESS);
//     } else if (pid > 0) {
//       printf("Parent %d\n", i);
//     }
//   }
//   wait(NULL);
//   exit(EXIT_SUCCESS);
// }


// int main(void) {
//   int i;
//   pid_t pid;

//   for (i = 0; i < NUM_PROC; i++) {
//     pid_t pid_padre=getpid();
//     pid = fork();
//     if (pid < 0) {
//       perror("fork");
//       exit(EXIT_FAILURE);
//     } else if (pid == 0) {
//       printf("Child %d, padre %d\n", getpid(),pid_padre);
//     } else if (pid > 0) {
//       printf("Parent %d\n", i);
//       wait(NULL);
//       exit(EXIT_SUCCESS);
//     }
//   }
  
// }

int main(void) {
  int i;
  pid_t pid;

  for (i = 0; i < NUM_PROC; i++) {
    pid = fork();
    if (pid < 0) {
      perror("fork");
      exit(EXIT_FAILURE);
    } else if (pid == 0) {
      printf("Child %d\n", i);
      exit(EXIT_SUCCESS);
    } else if (pid > 0) {
      printf("Parent %d\n", i);
    }
    wait(NULL);
  }
  exit(EXIT_SUCCESS);
}