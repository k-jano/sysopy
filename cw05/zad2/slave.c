#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define COUNT 100

int main(int argc, char** argv){

    srand(time(NULL));

    if(argc != 3){
        perror("Wrong number of args\n");
        exit(1);
    }


    char* path= argv[1];
    int N = atoi(argv[2]);

    int stream;

    if((stream=open(path, O_WRONLY))==-1){
        perror("Can not open a stream \n");
        exit(1);
    }

    fprintf(stdout, "Process PID: %d\n", getpid());

    for(int i=0; i<N; i++){

        char* buffer_to_write=malloc(COUNT*sizeof(char));
        char* buffer_to_read=malloc(COUNT*sizeof(char));
        size_t size;

        FILE *date_stream;

        if((date_stream=popen("date", "r"))==NULL){
            perror("Can not open date_stream\n");
            exit(1);
        }

        if(getline(&buffer_to_read, &size, date_stream)<0){
            perror("Can not read date_stream");
            exit(1);
        }

        pclose(date_stream);
        
        sprintf(buffer_to_write, "Slave: %d - %s", getpid(), buffer_to_read);
        write(stream, buffer_to_write, strlen(buffer_to_write));

        int random_value=rand()%3+2;
        sleep(random_value);
    }    

    close(stream);

}