#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>


const int args_limit=20;
int actual_line=0;

void print_times(struct timeval start, struct timeval end){
    double tmp_start = ((double)start.tv_sec)+(((double)start.tv_usec)/1000000);
    double tmp_end = ((double)end.tv_sec)+(((double)end.tv_usec)/1000000);
    double difference=tmp_end-tmp_start;

    int minutes=(int)(difference/60);
    double seconds=difference-minutes*60;
    printf("%dm%.4fs\n", minutes, seconds);
}

void convert_white_signs(char* buffer) {

    int i=0;
    while(buffer[i]!='\0'){
        if(isspace(buffer[i])){
            buffer[i]=' ';
        }
        i++;
    }
}

void read_line(char *command, struct rlimit* cpu_rlimit, struct rlimit* vmem_rlimit){

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


    struct rusage start, end;
    struct timeval sys_start, sys_end, user_start, user_end;

    if(getrusage(RUSAGE_CHILDREN, &start)==-1){
        printf("Can not obtain resource for children \n");
        exit(1);
    }
    
    

    int new_process=fork();

    if(new_process<0){

        printf("Fork failed\n");
        exit(1);
    }

    else if(new_process==0){
        setrlimit(RLIMIT_CPU, cpu_rlimit);
        setrlimit(RLIMIT_AS, vmem_rlimit);

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

    
    if(getrusage(RUSAGE_CHILDREN, &end)==-1){
        printf("Can not obtain resource for children \n");
        exit(1);
    }
    
    sys_start = start.ru_stime;
    user_start = start.ru_utime;
    sys_end = end.ru_stime;
    user_end = end.ru_utime;
    printf("Command '%s' in line %d\n", first_arg, actual_line);
    printf("System: ");
    print_times(sys_start, sys_end);
    printf("User: ");
    print_times(user_start, user_end);

    printf("\n\n");
}


int main(int argc, char** argv){

    if(argc!=4){
        printf("Wrong number of arguments\n");
        exit(1);
    }

    FILE* file= fopen(argv[1], "r");

    struct rlimit cpu_rlimit, vmem_rlimit;

    cpu_rlimit.rlim_max=atoi(argv[2]);
    cpu_rlimit.rlim_cur=cpu_rlimit.rlim_max;

    vmem_rlimit.rlim_max=atoi(argv[3])*1048576;
    vmem_rlimit.rlim_cur=vmem_rlimit.rlim_max;


    

    char* command;
    size_t size=0;
    size_t read;
    
    while((read=getline(&command, &size, file))!= -1){
        actual_line++;
        read_line(command, &cpu_rlimit, &vmem_rlimit);
    }
     
}