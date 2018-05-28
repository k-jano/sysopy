#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include "info.h"

void die(char *s)
{
  perror(s);
  exit(1);
}

typedef struct msgbuf
{
  long    mtype;
  char    mtext[MESS_MAX_SIZE];
}message;

//Global variables
int server_id;
int clients_id[CLIENTS_LIMIT];
int clients_numbers;
int clients_pid[CLIENTS_LIMIT];

int main_flag;

void create_main_queue(){
  char* home= getenv("HOME");

  key_t server_key;

  if((server_key=ftok(home, ID))==-1)
    die("ftok");

  if((server_id=msgget(server_key, IPC_CREAT | IPC_EXCL | S_IWUSR | S_IRUSR))==-1)
    die("msget");  
}

static void fun_exit(){
  if(server_id!=1){
    if(msgctl(server_id, IPC_RMID, NULL)==-1)
      die("msgctl");
  }
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

void send_message(int msqid, struct msgbuf sbuf, size_t buflen){
  if (msgsnd(msqid, &sbuf, buflen, IPC_NOWAIT) < 0)
    die("msgsnd");
}

void receive_response(int msqid, struct msgbuf rcvbuf, int type){
  if (msgrcv(msqid, &rcvbuf, MESS_MAX_SIZE, type, 0) < 0)
    die("msgrcv");
  printf("%s\n", rcvbuf.mtext);
}

void first_message_operation(struct msgbuf rcvbuffer){
  
  
  //Copy
  char* command=(char*)malloc(sizeof(rcvbuffer.mtext));
  sprintf(command, "%s", rcvbuffer.mtext);

  //First arg
  char* first_arg = strtok(command, " ");
  int pid_tmp=atoi(first_arg);
  
  
  //Msqid
  char* msqid_rcvd=strtok(NULL, " ");
  int msqid_tmp=atoi(msqid_rcvd);

  if(clients_numbers<CLIENTS_LIMIT){
    int place=-1;
    for(int i=0; i<CLIENTS_LIMIT; i++){
      if(clients_pid[i]==-1){
        place=i;
        break;
      }
    }

    if(place==-1)
      return;

    clients_id[place]=msqid_tmp;
    clients_pid[place]=pid_tmp;
    printf("Approvement: %d\n", clients_pid[place]);

    //Sending first response
    struct msgbuf sbuf;
    size_t buflen;
    sbuf.mtype = FIRST_MESSAGE;
    sprintf(sbuf.mtext, "Approval");
    buflen = strlen(sbuf.mtext) + 1 ;

    send_message(clients_id[place], sbuf, buflen);

    clients_numbers++;

  }
  else{

    printf("Rejected: %d\n", pid_tmp);

    //Sending first response
    struct msgbuf sbuf;
    size_t buflen;
    sbuf.mtype = REJECT;
    sprintf(sbuf.mtext, "Rejected");
    buflen = strlen(sbuf.mtext) + 1 ;

    
    send_message(msqid_tmp, sbuf, buflen);

    clients_numbers=CLIENTS_LIMIT;
  }

  printf("Clients number: %d\n", clients_numbers);   
}

void mirror_operation(struct msgbuf rcvbuffer){

  //Copy
  char* command=(char*)malloc(sizeof(rcvbuffer.mtext));
  sprintf(command, "%s", rcvbuffer.mtext);

  //First arg
  char* first_arg = strtok(command, " ");
  int pid_tmp=atoi(first_arg);

  strtok(NULL, " ");

  char* text=strtok(NULL, "/0");
  char* ans=(char*)malloc(sizeof(text));

  //Creating answer
  int counter=0;
  while(text[counter]!='\n' && text[counter]!='\0')
    counter++;

  for(int i=counter-2; i>=0; i--)
    ans[i]=text[counter-i-2];

  ans[counter-1]='\0';

  //Sending answer
  struct msgbuf sbuf;
  size_t buflen;
  sbuf.mtype = MIRROR;
  sprintf(sbuf.mtext, "%s", ans);
  buflen = strlen(sbuf.mtext) + 1 ;


  for(int i=0; i<CLIENTS_LIMIT; i++){
    if(clients_pid[i]==pid_tmp){
      send_message(clients_id[i], sbuf, buflen);
      break;
    }
  }    
}

void calc_operation(struct msgbuf rcvbuffer){
  //Copy
  char* command=(char*)malloc(sizeof(rcvbuffer.mtext));
  sprintf(command, "%s", rcvbuffer.mtext);

  //Converting
  convert_white_signs(command);

  //First arg
  char* first_arg = strtok(command, " ");
  int pid_tmp=atoi(first_arg);

  //Rest of args
  strtok(NULL, " ");
  char* operation=strtok(NULL, " ");
  char* first_str=strtok(NULL, " ");
  char* second_str=strtok(NULL, " ");

  int first_int=atoi(first_str);
  int second_int=atoi(second_str);


  char* ans=(char*)malloc(sizeof(command));

  //Calculating
  int result=0;
  if(strcmp(operation, "ADD")==0)
    result=first_int+second_int;
  else if(strcmp(operation, "SUB")==0)
    result=first_int-second_int;
  else if(strcmp(operation, "MUL")==0)    
    result=first_int*second_int;
  else if(strcmp(operation, "DIV")==0){
    if(second_int!=0)
      result=first_int/second_int;
  }  
  else
    sprintf(ans, "Wrong operation\n");

  if(second_int!=0)
    sprintf(ans, "%d", result);
  else
    sprintf(ans, "Can not div by 0!"); 

  //Sending answer
  struct msgbuf sbuf;
  size_t buflen;
  sbuf.mtype = CALC;
  sprintf(sbuf.mtext, "%s", ans);
  buflen = strlen(sbuf.mtext) + 1 ;

  for(int i=0; i<CLIENTS_LIMIT; i++){
    if(clients_pid[i]==pid_tmp){
      send_message(clients_id[i], sbuf, buflen);
      break;
    }
  }

}

void time_operation(struct msgbuf rcvbuffer){
  //Copy
  char* command=(char*)malloc(sizeof(rcvbuffer.mtext));
  sprintf(command, "%s", rcvbuffer.mtext);


  //First arg
  char* first_arg = strtok(command, " ");
  int pid_tmp=atoi(first_arg);

  char* ans=(char*)malloc(100*sizeof(char));
  time_t actualTime;
  time(&actualTime);
  struct tm* tmActual=localtime(&actualTime);
  strftime(ans, MESS_MAX_SIZE, "%c", tmActual);

  //Sending answer
  struct msgbuf sbuf;
  size_t buflen;
  sbuf.mtype = TIME;
  sprintf(sbuf.mtext, "%s", ans);
  buflen = strlen(sbuf.mtext) + 1 ;

  for(int i=0; i<CLIENTS_LIMIT; i++){
    if(clients_pid[i]==pid_tmp){
      send_message(clients_id[i], sbuf, buflen);
      break;
    }
  }

}

void client_end_operation(struct msgbuf rcvbuffer){
  //Copy
  char* command=(char*)malloc(sizeof(rcvbuffer.mtext));
  sprintf(command, "%s", rcvbuffer.mtext);


  //First arg
  char* first_arg = strtok(command, " ");
  int pid_tmp=atoi(first_arg);

  int index;
  for(int i=0; i<CLIENTS_LIMIT; i++){
    if(clients_pid[i]==pid_tmp){
      index=i;
      break;
    }  
  }

  clients_id[index]=-1;
  clients_pid[index]=-1;
  clients_numbers--;
  printf("Remove client: %d\n", pid_tmp);
  printf("Clients number: %d\n", clients_numbers);
}

void sig_int(int signum){
  die("Koniec pracy serwera\n");
}


int main()
{
  struct sigaction act;
  act.sa_handler=sig_int;
  sigemptyset(&act.sa_mask);
  act.sa_flags=0;
  if(sigaction(SIGINT, &act, NULL)==-1)
    die("sigaction"); 


  server_id=-1;
  main_flag=0;

  for(int i=0; i<CLIENTS_LIMIT; i++){
    clients_id[i]=-1;
    clients_pid[i]=-1;
  }  

  if(atexit(fun_exit)!=0)
    die("atexit");

  create_main_queue();
  clients_numbers=0;
  printf("Clinets number: %d\n", clients_numbers);

  while(main_flag==0){
    struct msgbuf rcvbuffer;
    //Receive first message
    if (msgrcv(server_id, &rcvbuffer, MESS_MAX_SIZE, 0, 0) < 0)
      die("msgrcv"); 


    if(rcvbuffer.mtype==FIRST_MESSAGE)
      first_message_operation(rcvbuffer);

    else if(rcvbuffer.mtype==MIRROR)
      mirror_operation(rcvbuffer);

    else if(rcvbuffer.mtype==CALC)
      calc_operation(rcvbuffer);

    else if(rcvbuffer.mtype==TIME)
      time_operation(rcvbuffer);

    else if(rcvbuffer.mtype==CLIENT_END)
      client_end_operation(rcvbuffer);

    else if(rcvbuffer.mtype==END)
      main_flag=-1;  
        
  }
  
  for(int i=0; i<CLIENTS_LIMIT; i++){
    if(clients_pid[i]!=-1){
      kill(clients_pid[i], SIGINT);
    }
  }
  printf("Server close\n");  
  exit(0);
}