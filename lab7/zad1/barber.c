#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ipc.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "main.h"

//Shared memory
int shmid;
struct barber *my_barber;

//Semaphores
int queue_id;
int chair_id;
int shaving_id;


//Sleeping flag
int sleeping_flag;

//Arguments
int places;

//SEMUN
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} semun;

//Initializing queue
void initialize_queue(int size) {

    my_barber->max_size = size;
    my_barber->current_size = 0;
    for(int i=0; i<my_barber->max_size; i++){
        my_barber->clients[i]=-1;
    }
}


//Getting PID of longer waiting
pid_t get_longer_waiting() {

   if(my_barber->current_size<=0)
       return -1;
   
   
   pid_t result=my_barber->clients[0];
   for(int i=0; i<my_barber->current_size; i++){
       my_barber->clients[i]=my_barber->clients[i+1];
   }
   my_barber->current_size--;

   return result;
}

//Printing time message
void print_message(char *msg) {

    struct timespec tmp_ts;


    clock_gettime(CLOCK_MONOTONIC, &tmp_ts);
    printf("%ld s %ld mikros   pid %d: %s", tmp_ts.tv_sec, tmp_ts.tv_nsec / 1000, getpid(), msg);
}

//Locking semaphore
void decrease(int sem_id) {

    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = SEM_UNDO;

    semop(sem_id, &buf, 1);
}

//Unlocking semaphore
void increase(int sem_id) {

    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = 1;
    buf.sem_flg = SEM_UNDO;

    semop(sem_id, &buf, 1);
}

//Waiting to reach 0 value
void wait_for_lock(int sem_id) {

    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = 0;
    buf.sem_flg = 0;

    semop(sem_id, &buf, 1);
}

//Initialization of shared memory and semaphores
void initialize_barber() {

    char *tmp_path = getenv("HOME");

    // Shared memory initialization
    key_t shm_key = ftok(tmp_path, SHM_ID);
    shmid = shmget(shm_key, sizeof(struct barber), IPC_CREAT | IPC_EXCL | S_IWRITE | S_IREAD);
    my_barber = (struct barber*) shmat(shmid, 0, 0);

    if(my_barber == (void*)-1) {
        perror("Error while shared memory initialization");
        exit(1);
    }

    my_barber -> all_sits_number = places;
    my_barber -> barber_pid = getpid();
    my_barber -> is_sleeping = 0;
    my_barber -> shaved_client_pid = -1;
    initialize_queue(places);




    // Semaphores initializotion
    key_t queue_key = ftok(tmp_path, QUEUE_KEY_ID);
    queue_id = semget(queue_key, 1, IPC_CREAT | IPC_EXCL | S_IWRITE | S_IREAD);
    semctl(queue_id, 0, SETVAL, 0);


    key_t chair_key = ftok(tmp_path, CHAIR_KEY_ID);
    chair_id = semget(chair_key, 1, IPC_CREAT | IPC_EXCL | S_IWRITE | S_IREAD);
    semctl(chair_id, 0, SETVAL, 0);

    key_t shaving_key = ftok(tmp_path, SHAVING_KEY_ID);
    shaving_id = semget(shaving_key, 1, IPC_CREAT | IPC_EXCL | S_IWRITE | S_IREAD);
    semctl(shaving_id, 0, SETVAL, 0);

}

//Sleeping
void go_sleep() {

    my_barber -> is_sleeping = 1;



    print_message("Barber going sleep\n");
    
    
    sleeping_flag=0;
    increase(queue_id); 

    sigset_t new_mask, old_mask;
    sigfillset(&new_mask);
    sigdelset(&new_mask, SIGINT);
    sigdelset(&new_mask, WAKEUP);
    sigprocmask(SIG_SETMASK, &new_mask, &old_mask);

    while(sleeping_flag == 0)
        sigsuspend(&old_mask);

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
    my_barber -> is_sleeping = 0;
}

//Shaving procedure
void shave() {

    increase(shaving_id);            
    increase(chair_id);         


    wait_for_lock(chair_id);    

    char* tmp=malloc(50*sizeof(char));

    sprintf(tmp, "Starting shaving %d\n", my_barber->shaved_client_pid);
    print_message(tmp);          
    sprintf(tmp, "Finishing shaving %d\n", my_barber->shaved_client_pid);
    print_message(tmp);         

    decrease(shaving_id);    
    decrease(chair_id);     
}

//Sending invitation
int invite_next_client() {

    decrease(queue_id);    

    pid_t next_client_pid = get_longer_waiting();

    if(next_client_pid == -1)   
        return 0;


    char* tmp=malloc(50*sizeof(char));
    sprintf(tmp, "Inviting %d\n", next_client_pid);
    print_message(tmp);
    kill(next_client_pid, INVITATION);
    increase(queue_id);
    return 1;
}

//EXIT, deleting all shared memory and semaphores
static void exit_fun() {

    if(shmctl(shmid, IPC_RMID, NULL) == -1)
        perror("Error occurred while deleting shared memory segment");


    if(semctl(queue_id, 0, IPC_RMID, semun) == -1)
        perror("Error occurred while deleting waiting room semaphore");
    

    if(semctl(chair_id, 0, IPC_RMID, semun) == -1)
        perror("Error occurred while deleting barber's chair semaphore'");
    

    if(semctl(shaving_id, 0, IPC_RMID, semun) == -1)
        perror("Error occurred while deleting shaving control semaphore");

}

//SIGINT HANDLER
void sig_int(int signum) {

    printf("SIGINT signal\n");
    exit(1);
}

//WAKEUP HANDLER
void sig_wakeup(int signum) {

    print_message("WAKEUP signal\n");
    sleeping_flag = 1;
}


int main(int argc, char** argv) {


    sleeping_flag = 0;

    if (atexit(exit_fun) != 0) {

        perror("Atexit\n");
        exit(1);
    }

    // struct sigaction
    struct sigaction act;
    act.sa_handler = sig_int;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if(sigaction(SIGINT, &act, NULL) == -1) {

        perror("SIGINT handler\n");
        exit(1);
    }


    //struct sigaction
    struct sigaction act2;
    act2.sa_handler = sig_wakeup;
    sigemptyset(&act2.sa_mask);
    act2.sa_flags = 0;
    if(sigaction(WAKEUP, &act2, NULL) == -1) {

        perror("WAKEUP handler");
        exit(1);
    }

    if(argc != 2) {
        printf("Wrong nr of args\n");
        exit(1);
    }

    places = atoi(argv[1]);

    initialize_barber();

    while(1) {

        go_sleep();
        shave();

        while(invite_next_client() == 1)
            shave();
    }
}