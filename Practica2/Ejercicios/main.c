#include <stdio.h>

int main(){
    int i=0;

    for(;i<3;i++){
        wait();
        if(i%2) fork();
        if(fork())
            wait();
        printf('a'+i+'\n');   
    }
}