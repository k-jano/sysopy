#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <pthread.h>

#define CLIENT_MAX_NAME_LEN 30
#define ADD 1
#define SUB 2
#define MUL 3
#define DIV 4
#define LOG 11
#define PING 12
#define CALC 13
#define RES 14

struct expr{
    int operator;
    int arg1;
    int arg2;
}expr;

struct message{
    int type;
    int id;
    char name[CLIENT_MAX_NAME_LEN];
    struct expr my_expr;
}message;

char* name;
int type;
char* address;
unsigned short port;
int my_socket;

void sig_int(int signum){
    printf("SIGINT signal received\n");
    exit(1);
}

void exit_fun(){
    shutdown(my_socket, SHUT_RDWR);
    close(my_socket);
}

void inet_initialization(){
    my_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(my_socket==-1){
        printf("Socket inet initialization\n");
        exit(1);
    }

    struct sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    if(inet_pton(AF_INET, address, &addr_in.sin_addr)==0){
        perror("Wrong address\n");
        exit(1);
    }
    addr_in.sin_port=htons(port);

    if(connect(my_socket, (const struct sockaddr*)&addr_in, sizeof(addr_in))==-1){
        perror("Error while connecting\n");
        exit(1);
    }
}

void local_initialization(){
    my_socket=socket(AF_UNIX, SOCK_STREAM, 0);
    if(my_socket==-1){
        printf("Socket inet initialization\n");
        exit(1);
    }

    struct sockaddr_un addr_un;
    addr_un.sun_family=AF_UNIX;
    sprintf(addr_un.sun_path, "%s", address);

    if(connect(my_socket, (const struct sockaddr*)&addr_un, sizeof(addr_un))==-1){
        perror("Error while connecting\n");
        exit(1);
    }
}

void login(){
    struct message my_mess;
    sprintf(my_mess.name, "%s", name);
    my_mess.type=LOG;
    int tmp;
    if((tmp=write(my_socket, &my_mess, sizeof(my_mess)))<0){
        perror("Error while logging\n");
        exit(1);
    }
}

void calculate(struct expr expr_tmp, int id){
    int result;
    if(expr_tmp.operator==ADD)
        result=expr_tmp.arg1+expr_tmp.arg2;
    else if(expr_tmp.operator==SUB)
        result=expr_tmp.arg1-expr_tmp.arg2;
    else if(expr_tmp.operator==MUL)
        result=expr_tmp.arg1*expr_tmp.arg2;
    else if(expr_tmp.operator==DIV)
        result=expr_tmp.arg1/expr_tmp.arg2;

    struct message my_mess;
    sprintf(my_mess.name, "%s", name);
    my_mess.id=id;
    my_mess.my_expr.arg1=result;
    my_mess.type=RES;
    //printf("%d %d %d Calculated %d\n", expr_tmp.arg1, expr_tmp.operator, expr_tmp.arg2, result);
    if(write(my_socket, &my_mess, sizeof(my_mess))<0){
        perror("Error while replying\n");
        exit(1);
    }
}

void client_routine(){
    while(1){
        struct message my_mess;
        ssize_t bytes_read = recv(my_socket, &my_mess, sizeof(my_mess), MSG_WAITALL);
        if(bytes_read==0){
            printf("Disconnected\n");
            exit(0);
        }

        if (my_mess.type==LOG){
            printf("Name already in use\n");
            exit(1);
        }
        else if(my_mess.type==PING){
            printf("Ping!\n");
            write(my_socket, &my_mess, sizeof(my_mess));
        }
        else if(my_mess.type==CALC){
            printf("Calculation\n");
            calculate(my_mess.my_expr, my_mess.id);
        }
    }
}


int main(int argc, char** argv){
    struct sigaction act;
    act.sa_handler=sig_int;
    sigemptyset(&act.sa_mask);
    if(sigaction(SIGINT, &act, NULL)==-1){
        perror("Can not set SIGINT handler\n");
        exit(1);
    }

    atexit(exit_fun);

    if(argc!=4 && argc!=5){
        perror("Wrong nr of args\n");
        exit(1);
    }

    name=argv[1];
    type=atoi(argv[2]);
    address=argv[3];

    if(type==0){
        port = atoi(argv[4]);
        inet_initialization();
    }
    else
        local_initialization();


    login();
    client_routine();

}