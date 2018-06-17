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

#define UNIX_PATH_MAX    108
#define MAX_CLIENT_NUMBER 15
#define CLIENT_MAX_NAME_LEN 30
#define MAX_EVENTS_NUMBER 7
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

typedef struct client{
    char name[CLIENT_MAX_NAME_LEN];
    int descriptor;
    int ping;
}client;

int tcp_port;
char socket_path[UNIX_PATH_MAX];
int inet_socket, local_socket;
int epoll_descriptor;
pthread_mutex_t main_mutex;
pthread_t network;
pthread_t ping;
pthread_t input;
client my_clients[MAX_CLIENT_NUMBER];

void sig_int(int signum){
    printf("SIGINT signal received\n");
    exit(1);
}

void exit_fun(){
    close(inet_socket);
    close(local_socket);
    close(epoll_descriptor);
    pthread_mutex_destroy(&main_mutex);
    unlink(socket_path);
}

void register_client(struct epoll_event event){
    pthread_mutex_lock(&main_mutex);;
    for(int i=0; i<MAX_CLIENT_NUMBER; i++){
        if(my_clients[i].ping==-1){
            my_clients[i].ping=0;
            struct sockaddr new_addr;
            socklen_t new_addr_length= sizeof(new_addr);
            my_clients[i].descriptor = accept(event.data.fd, &new_addr, &new_addr_length);

            struct epoll_event e;
            e.events = EPOLLIN | EPOLLET;
            e.data.fd = my_clients[i].descriptor;
            if(epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, my_clients[i].descriptor, &e)==-1){
                printf("Failed to creata epoll for client\n");
                my_clients[i].ping=-1;
            }
            break;
        }
    }
    pthread_mutex_unlock(&main_mutex);
}

void shutdown_client(struct epoll_event event){
    pthread_mutex_lock(&main_mutex);
    shutdown(event.data.fd, SHUT_RDWR);
    close(event.data.fd);

    for(int i=0; i<MAX_CLIENT_NUMBER; i++){
        if(my_clients[i].ping!=-1 && event.data.fd == my_clients[i].descriptor){
            my_clients[i].ping= -1;
            for(int j=0; j<CLIENT_MAX_NAME_LEN; j++)
                my_clients[i].name[j]=0;
        }
    }
    pthread_mutex_unlock(&main_mutex);
}

void receive_message(struct epoll_event event){
    struct message my_mess;
    ssize_t bytes_read = read(event.data.fd, &my_mess, sizeof(my_mess));

    if(bytes_read==0){
        printf("Closing connection\n");
        shutdown_client(event);
        return ;
    }
    else {
        if(my_mess.type==LOG){
            pthread_mutex_lock(&main_mutex);
            for(int i=0; i<MAX_CLIENT_NUMBER; i++){
                if(my_clients[i].ping!=-1 && strcmp(my_clients[i].name, my_mess.name)==0){
                    pthread_mutex_unlock(&main_mutex);
                    write(event.data.fd, &my_mess, sizeof(my_mess));
                    shutdown_client(event);
                    return ;
                }
            }
            for(int i=0; i<MAX_CLIENT_NUMBER; i++){
                if(my_clients[i].ping!=-1 &&event.data.fd == my_clients[i].descriptor){
                    printf("Logged: %s\n", my_mess.name);
                    sprintf(my_clients[i].name, "%s", my_mess.name);
                    pthread_mutex_unlock(&main_mutex);
                }
            }
        }
        else if(my_mess.type==PING){
            pthread_mutex_lock(&main_mutex);
            my_clients[my_mess.id].ping= 0;
            pthread_mutex_unlock(&main_mutex);
        }
        else if(my_mess.type==RES){
            printf("Result: %d of id %d\n", my_mess.my_expr.arg1, my_mess.id);
            fflush(stdout);
        }
    }
}

void *network_routine(){
    for(int i=0; i<MAX_CLIENT_NUMBER; i++)
        my_clients[i].ping=-1;
    struct epoll_event my_events[MAX_EVENTS_NUMBER];

    while(1){
        //printf("Here I am\n");
        int event_count = epoll_wait(epoll_descriptor, my_events, MAX_EVENTS_NUMBER, -1);
        for(int i=0; i<event_count; i++){
            if(my_events[i].data.fd == inet_socket || my_events[i].data.fd== local_socket){
                //printf("Registering\n");
                register_client(my_events[i]);
                //printf("Registered\n");
            }
            else{
                //printf("Jakis komunikat??\n");
                receive_message(my_events[i]);
            }
        }
    }
}


void *ping_routine(){
    while(1){
        struct message my_mess;
        my_mess.type=PING;
        pthread_mutex_lock(&main_mutex);
        for(int i=0; i<MAX_CLIENT_NUMBER; i++){
            if(my_clients[i].ping==0){
                my_clients[i].ping=1;
                my_mess.id=i;
                write(my_clients[i].descriptor, &my_mess, sizeof(my_mess));
                //printf("Pinging!!\n");
            }
        }
        pthread_mutex_unlock(&main_mutex);
        sleep(2);
        pthread_mutex_lock(&main_mutex);
        for(int i=0; i<MAX_CLIENT_NUMBER; i++){
            if(my_clients[i].ping==1){
                my_clients[i].ping=-1;
                for(int j=0; j<CLIENT_MAX_NAME_LEN; j++)
                    my_clients[i].name[j]=0;
            }
        }
        pthread_mutex_unlock(&main_mutex);
        sleep(8);
    }
}

void *input_routine(){
    int counter=0;
    while(1){
        //int first_arg;
        //int second_arg;
        //char operator;
        //char tmp;
        //int scanned = scanf("%d %c %d", &first_arg, &operator, &second_arg);
        //if(scanned!=3){
        //    while((tmp=getchar())!='\n' && tmp != EOF);
        //    printf("Wrong format of args, scanned: %d\n", scanned);
        //    continue;
        //}
        char* buffer = NULL;
        size_t buffer_size = 0;
        char *tokens[3];
        char *tmp;
        int bytes_read;
        bytes_read=getline(&buffer, &buffer_size, stdin);
        if(bytes_read==-1)
            continue;

        tmp = strtok(buffer, " \t\n");
        if(tmp==NULL)
            continue;

        tokens[0] = tmp;

        for(int i=1; i<3; i++){
            tmp = strtok(NULL, " \t\n");
            if(tmp == NULL)
                continue;
            else
                tokens[i]=tmp;
        }
        //printf("%s %s %s", tokens[0], tokens[1], tokens[2]);
        struct message my_mess;
        my_mess.my_expr.arg1=atoi(tokens[0]);
        my_mess.my_expr.arg2=atoi(tokens[2]);
        my_mess.id=counter;
        my_mess.type=CALC;
        if(strcmp(tokens[1], "+")==0)
            my_mess.my_expr.operator=ADD;
        else if(strcmp(tokens[1], "-")==0)
            my_mess.my_expr.operator=SUB;
        else if(strcmp(tokens[1], "*")==0)
            my_mess.my_expr.operator=MUL;
        else if(strcmp(tokens[1], "/")==0)
            my_mess.my_expr.operator=DIV;


        int local_flag=1;
        while(local_flag){
            pthread_mutex_lock(&main_mutex);
            for(int i=0; i<MAX_CLIENT_NUMBER; i++){
                if(my_clients[i].ping!=-1){
                    if(write(my_clients[i].descriptor, &my_mess, sizeof(my_mess))<=0)
                        printf("Cannot send message to %d client\n", i);
                    else{
                        local_flag=0;
                        break;
                    }
                }
            }
            pthread_mutex_unlock(&main_mutex);
        }
        counter++;
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

    if(argc!=3){
        perror("Wrong nr of args\n");
        exit(1);
    }

    //Parsing

    tcp_port=atoi(argv[1]);
    sprintf(socket_path, "%s", argv[2]);

    printf("Args: %d TCP_PORT, %s Socket_Path\n", tcp_port, socket_path);

    //Socket Initialization

    //Inet

    struct sockaddr_in addr_in;
    inet_socket = socket(AF_INET, SOCK_STREAM, 0);
    addr_in.sin_family= AF_INET;
    addr_in.sin_port = htons(tcp_port);
    addr_in.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(inet_socket, (struct sockaddr *)&addr_in, sizeof(addr_in))!=0){
        perror("Error while inet binding\n");
        exit(1);
    }

    //printf("Ready INET??\n");

    //Local
    struct sockaddr_un addr_un;
    local_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    addr_un.sun_family = AF_UNIX;
    sprintf(addr_un.sun_path, "%s", socket_path);

    if(bind(local_socket, (struct sockaddr *)&addr_un, sizeof(addr_un))!=0){
        perror("Error while local binding\n");
        exit(1);
    }

    //printf("Ready LOCAL??\n");

    //Listening
    int listen_inet_tmp=listen(inet_socket, MAX_CLIENT_NUMBER);
    if(listen_inet_tmp!=0){
        perror("Error while inet listening\n");
        exit(1);
    }

    int listen_local_tmp=listen(local_socket, MAX_CLIENT_NUMBER);
    if(listen_local_tmp!=0){
        perror("Error while local listening\n");
        exit(1);
    }

    //printf("Is listening OK?\n");

    //Epoll
    epoll_descriptor=epoll_create1(0);
    if(epoll_descriptor==-1){
        perror("Error while epoll create\n");
        exit(1);
    }

    //printf("Is main epoll ok??\n");

    struct epoll_event tmp;
    tmp.events=EPOLLIN | EPOLLET;
    tmp.data.fd = inet_socket;
    if(epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, inet_socket, &tmp)==-1){
        perror("Error while epoll inet ctl");
        exit(1);
    }

    tmp.data.fd=local_socket;
    if(epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, local_socket, &tmp)==-1){
        perror("Error while epoll local ctl");
        exit(1);
    }

    //printf("Everything is set\n");

    //Mutex
    pthread_mutex_init(&main_mutex, NULL);

    pthread_create(&network, NULL, network_routine, NULL);

    pthread_create(&ping, NULL, ping_routine, NULL);

    pthread_create(&input, NULL, input_routine, NULL);

    while(1){

    }
}