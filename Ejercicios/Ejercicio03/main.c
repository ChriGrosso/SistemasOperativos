#include "errno.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

int main(int argc, char*argv[]){
    char filename[100];
    int i;
    strcpy(filename,argv[1]);
    FILE* f;
    f=fopen(filename,"r");
    if(f==NULL)
        //printf("%s\nError Code %d\n",strerror(errno),errno);
        perror("ERR");
    else
        return EXIT_SUCCESS;
}