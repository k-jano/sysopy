#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char** argv){

    if(argc !=2){
        perror("Wrong number of args\n");
        exit(1);
    }

    char* path= argv[1];

    if(mkfifo(path, S_IRUSR | S_IWUSR)==-1){
        perror("Can not create a stream \n");
        exit(1);
    }

    FILE *stream;

    if((stream=fopen(path, "r"))==NULL){
        perror("Can not open a stream \n");
        exit(1);
    }

    size_t size=0;
    char* buffer;

    while((getline(&buffer, &size, stream))>0)
        printf("%s", buffer);
    
    fclose(stream);
}
