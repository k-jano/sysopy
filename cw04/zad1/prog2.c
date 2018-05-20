#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

pid_t child_id;
const char name[100]="/home/kj/Dokumenty/sysopy/lab4/zad1/test_bash.sh";

//Function running script in new process
void run_script(){
    pid_t pid_tmp=fork();

    if(pid_tmp<0){
        printf("Can not execute fork()\n");
        exit(1);
    }

    else if(pid_tmp>0)
        child_id=pid_tmp;

    else{
        if(execl(name, "", NULL)<0){
            printf("Can not start program\n");
            exit(1);
        }        
    }
}

//Function checking if script is running
int is_running(){
    if(child_id==-1)
        return 0;
    else{
        kill(child_id, SIGKILL);
        child_id=-1;
        return 1;
    }
}

//Function handling SIGINT signal
void sig_int(int signum){
    if(child_id!=-1)
        kill(child_id, SIGKILL);
    printf("Odebrano sygnał SIGINT\n");
    exit(1);
}

//Function handling SIGTSTP signal
void sig_tstp(int signum){
    if (is_running()==1){
        printf("Oczekuję na CTRL+Z - kontynuacja albo CTR+C - zakonczenie programu\n");
        fflush(stdout);      
    }
    else
        run_script();
}

void set_handlers(){
    if(signal(SIGINT, sig_int)==SIG_ERR){
        printf("Wystąpił błąd przy przyjmowaniu SIGINT\n");
        exit(1);
    }

    struct sigaction act;
    act.sa_handler=sig_tstp;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    if(sigaction(SIGTSTP, &act, NULL)==-1){
        printf("Wystąpił błąd przy przyjmowaniu SIGTSTP\n");
        exit(1);
    }
}

int main(){
    
    set_handlers();
    run_script();

    while (1){
        pause();
    }
}