#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define SHM_NAME "/shm_example"
#define MESSAGE "Test message"

int main(int argc, char *argv[]) {
  int fd;
  struct stat statbuf;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <FILE>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  
  fd = open(argv[1], O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }
  dprintf(fd, "%s", MESSAGE);
  /* Get size of the file. */
  fstat(fd,&statbuf);
  long size;
  size=statbuf.st_size;
  printf("%ld\n",size);

  /* Truncate the file to size 5. */
  ftruncate(fd,5);
  
  fstat(fd,&statbuf);
  size=statbuf.st_size;
  printf("%ld\n",size);

  lseek(fd, 0, SEEK_SET); // vuelve al principio del archivo

  char buffer[100] = {0};
  ssize_t n = read(fd, buffer, sizeof(buffer) - 1);

  if (n == -1) {
      perror("read");
      exit(EXIT_FAILURE);
  }

  printf("Contenido del fichero: '%s'\n", buffer);
  
  close(fd);
  shm_unlink(argv[1]);
  exit(EXIT_SUCCESS);
}
