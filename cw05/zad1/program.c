#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#define limit 5
#define sec_limit 100

const int args_limit=20;
int actual_line=0;

pid_t final;


char tab_s[limit][sec_limit];
int tab_s_counter;

void clear(){
    for(int i=0; i<limit; i++){
        for(int j=0; j<sec_limit; j++)
            tab_s[i][j]='\0';
    }
}

void convert_white_signs(char* buffer){
    int i=0;
    while(buffer[i]!='\0'){
        if(isspace(buffer[i])){
            buffer[i]=' ';
        }
        i++;
    }
}

void convert_line(char* buffer){
    tab_s_counter=0;
    int position=0;
    int i=0;
    int flag=1;
    for(int j=0; j<limit && flag==1; j++){
        while(buffer[i]!='|'){
            tab_s[tab_s_counter][position]=buffer[i];
            position++;
            i++;
            if(buffer[i+1]=='\0'){
                flag=0;
                break;
            }
        }
        tab_s_counter++;
        position=0;
        i++;
    }    
}

void read_line(char *command){
    clear();
    convert_line(command);
    
    
    for(int i=0; i<tab_s_counter; i++){
        convert_white_signs(tab_s[i]);
    }

    char* first_arg[limit];
    char* args[limit][args_limit];

    for(int i=0; i<tab_s_counter; i++){

        first_arg[i]=strtok(tab_s[i], " ");

        if(first_arg==NULL){
            return ;
        }

        args[i][0]=first_arg[i];
        int actual_arg=1;
        while((args[i][actual_arg]=strtok(NULL, " "))!=NULL){
            actual_arg++;
        

            if(actual_arg>args_limit){
                printf("Too many args, acutal arg_limit is:%d\n ", args_limit);
                exit(1);
            }
        }
    }

    int fd[tab_s_counter-1][2];
    for(int i=0; i<tab_s_counter-1; i++){
        pipe(fd[i]);
    }    

    for(int i=0; i<tab_s_counter; i++){    
        int new_process=fork();
        if(i==tab_s_counter-1){
            final=new_process;
        }
        if(new_process<0){
            printf("Fork failed\n");
            exit(1);
        }

        else if(new_process==0){
            if(i!=0 && i!=tab_s_counter-1){
                close(fd[i-1][1]);
                dup2(fd[i-1][0], STDIN_FILENO);
            }

            if(i==tab_s_counter-1){

                dup2(fd[i-1][0], STDIN_FILENO);

                for(int i=0; i<tab_s_counter-1; i++){
                    close(fd[i][0]);
                    close(fd[i][1]);
                }
                
            }

            if(i!=tab_s_counter-1){
                close(fd[i][0]);
                dup2(fd[i][1], STDOUT_FILENO);
            }
            
            int new_exec;
            new_exec=execvp(first_arg[i], args[i]);
            if(new_exec==-1){
                exit(1);
            }
        }
    }
    for(int i=0; i<tab_s_counter-1; i++){
        close(fd[i][0]);
        close(fd[i][1]);
    }

    waitpid(final, NULL, 0);

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
        fprintf(stderr, "Line %d\n", actual_line);
        read_line(command);
        fprintf(stderr, "\n");
    }      
}