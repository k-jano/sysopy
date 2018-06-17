#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>


const int args_limit=20;
int actual_line=0;

void convert_white_signs(char* buffer) {

    int i=0;
    while(buffer[i]!='\0'){
        if(isspace(buffer[i])){
            buffer[i]=' ';
        }
        i++;
    }
}

void read_line(char *command){

    convert_white_signs(command);

    char* first_arg=strtok(command, " ");

    if(first_arg==NULL){
        return ;
    }

    char* args[args_limit+1];
    args[0]=first_arg;
    int actual_arg=1;
    while((args[actual_arg]=strtok(NULL, " "))!=NULL){
        actual_arg++;
        

        if(actual_arg>args_limit){
            printf("Too many args, acutal arg_limit is:%d\n ", args_limit);
            exit(1);
        }
    }
    int new_process=fork();

    if(new_process<0){
        printf("Fork failed\n");
        exit(1);
    }

    else if(new_process==0){
        int new_exec;
        new_exec=execvp(first_arg, args);
        if(new_exec==-1){
            exit(1);
        }
    }

    else if(new_process>0){
        int status;
        wait(&status);
        if(status!=0){
            printf("Error: Command %s in line %d\n", first_arg, actual_line);
            exit(1);
        }
    }

    printf("\n\n");
}


int main(int argc, char** argv){

    if(argc!=2){
        printf("Wrong number of arguments");
        exit(1);
    }

    FILE* file= fopen(argv[1], "r");

    char* command;
    size_t size=0;
    size_t read;
    
    while((read=getline(&command, &size, file))!= -1){
        actual_line++;
        read_line(command);
    }
     
}