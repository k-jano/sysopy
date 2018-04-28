#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include "info.h"


typedef struct msgbuf
{
    long    mtype;
    char    mtext[MESS_MAX_SIZE];
}message;

//Global private queue
int msqid_local;
//key_t key_local;
int msgflg_local;

//Global server queue 
int msqid;
int msgflg;
key_t key;

void die(char *s)
{
  perror(s);
  exit(1);
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

void connect_to_server(){
    char* home= getenv("HOME");

    if((key=ftok(home, ID))==-1)
        die("ftok");

    //Global message queue
    if ((msqid = msgget(key, 0)) < 0)
      die("msgget");
}

void send_message(int msqid, struct msgbuf sbuf, size_t buflen){
    if (msgsnd(msqid, &sbuf, buflen, IPC_NOWAIT) < 0)
        die("msgsnd");
}

void receive_response(int msqid, struct msgbuf rcvbuf, int type){
    if (msgrcv(msqid, &rcvbuf, MESS_MAX_SIZE, type, 0) < 0)
        die("msgrcv");

    if(rcvbuf.mtype==REJECT){
        printf("Rejected\n");
        exit(1);    
    }    

    printf("%s\n", rcvbuf.mtext);
}

static void exit_fun(){
    if(msqid_local!=-1){
        if(msqid!=1){
            
            struct msgbuf sbuf;
            size_t buflen;
            sbuf.mtype = CLIENT_END;
            sprintf(sbuf.mtext, "%d", (int)getpid());  
            buflen = strlen(sbuf.mtext) + 1 ;

            send_message(msqid, sbuf, buflen);

        }

        msgctl(msqid_local, IPC_RMID, NULL);
    }
}


void create_local(){

    //Global message queue
    if ((msqid_local = msgget(IPC_PRIVATE, S_IRUSR | S_IWUSR )) < 0)
      die("msgget");
}

void sig_int(int signum){
    exit_fun();
    die("Koniec pracy klienta\n");
}

int main(int argc, char** argv)
{
    if(argc!=2)
        die("Wrong nr of args\n");


    struct sigaction act;
    act.sa_handler=sig_int;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    if(sigaction(SIGINT, &act, NULL)==-1)
        die("sigaction");    

    msqid=-1;
    msqid_local=-1;

    //Local message queue
    create_local();

    //Conecting to server
    connect_to_server(msqid, ID);

    //Sending first message
    struct msgbuf sbuf;
    size_t buflen;
    sbuf.mtype = FIRST_MESSAGE;
    sprintf(sbuf.mtext, "%d %d", (int)getpid(), (int)msqid_local);  
    printf("%s\n", sbuf.mtext);
    buflen = strlen(sbuf.mtext) + 1 ;

    send_message(msqid, sbuf, buflen);

    
    //Receiving response
    struct msgbuf rcvbuffer;
    receive_response(msqid_local, rcvbuffer, 0);

    char* name1=(char*)malloc(100*sizeof(char));
    sprintf(name1, "%s", argv[1]);
    FILE* file=fopen(name1, "r");

    char* command;
    size_t size=0;
    size_t read;
    while((read=getline(&command, &size, file))!= -1){
        convert_white_signs(command);
        char* command_cp=(char*)malloc(sizeof(command));
        char* command_final=(char*)malloc(sizeof(command)+5*sizeof(command));
        sprintf(command_cp, "%s", command);

        char* first_arg = strtok(command_cp, " ");
        sprintf(command_final, "%d %s", (int)getpid(), command);
        struct msgbuf tmp_buf;
        size_t tmp_buflen;
        
        printf("Line: %s\n", command_final);
        if(strcmp(first_arg, "MIRROR")==0){
            tmp_buf.mtype=MIRROR;
            sprintf(tmp_buf.mtext, "%s", command_final);
            tmp_buflen=strlen(tmp_buf.mtext)+1;
            send_message(msqid, tmp_buf, tmp_buflen);
    
            receive_response(msqid_local, rcvbuffer, MIRROR);            
        }   
        else if(strcmp(first_arg, "CALC")==0){
            tmp_buf.mtype=CALC;
            sprintf(tmp_buf.mtext, "%s", command_final);
            tmp_buflen=strlen(tmp_buf.mtext)+1;
            send_message(msqid, tmp_buf, tmp_buflen);

            receive_response(msqid_local, rcvbuffer, CALC);
        }
        else if(strcmp(first_arg, "TIME")==0){
            tmp_buf.mtype=TIME;
            sprintf(tmp_buf.mtext, "%s", command_final);
            tmp_buflen=strlen(tmp_buf.mtext)+1;
            send_message(msqid, tmp_buf, tmp_buflen);

            receive_response(msqid_local, rcvbuffer, TIME);
        } 
        else if(strcmp(first_arg, "END")==0){
            tmp_buf.mtype=END;
            sprintf(tmp_buf.mtext, "%s", command_final);
            tmp_buflen=strlen(tmp_buf.mtext)+1;
            send_message(msqid, tmp_buf, tmp_buflen);

            exit_fun();
            exit(1);
        }
        else
            printf("It is something else\n");

        //ADDED SLEEP TO TEST A LOT OF CLIENTS
        sleep(4);
    }

    exit_fun();
    exit(0);
}