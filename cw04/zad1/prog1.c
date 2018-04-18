#define _XOPEN_SOURCE
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int active;

//Function printing time
void print_time(){    
    time_t actualTime;
    time(&actualTime);
    struct tm* tmActual=localtime(&actualTime);
    char date_to_print[100];
    strftime(date_to_print, 100, "%H:%M:%S", tmActual);
    printf("Actual time: %s\n", date_to_print);    
}

//Function handling SIGINT signal
void sig_int(int signum){
    printf("Odebrano sygnał SIGINT\n");
    exit(1);
}

//Function handling SIGTSTP signal
void sig_tstp(int signum){
    if (active==1){
        printf("Oczekuję na CTRL+Z - kontynuacja albo CTR+C - zakonczenie programu\n");
        active=0;
        fflush(stdout);      
    }
    else
        active=1;
}

void set_handlers(){
    if(signal(SIGINT, sig_int)==SIG_ERR){
        perror("Wystąpił błąd przy przyjmowaniu SIGINT\n");
        exit(1);
    }    

    struct sigaction act;
    act.sa_handler=sig_tstp;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    if(sigaction(SIGTSTP, &act, NULL)==-1){
        perror("Wystąpił błąd przy przyjmowaniu SIGTSTP\n");
        exit(1);
    }
}

int main() {

    set_handlers();
   
    active=1;
    while(1){
        if(active==1){
            print_time();
            sleep(1);
        }
    }    
}