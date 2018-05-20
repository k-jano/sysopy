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

//Semaphores
int queue_id;
int chair_id;
int shaving_id;


//Shared memory
int shmid;
struct barber *my_barber;

//Arguments
int number_of_clients;
int number_of_shave;

//Flag
int waiting_flag;



//Adding process to queue
void add_to_queue(pid_t client_pid) {
    my_barber->clients[my_barber->current_size]=client_pid;
    my_barber->current_size++;

}

//Printing message
void print_message(char *msg) {

    struct timespec tmp_ts;


    clock_gettime(CLOCK_MONOTONIC, &tmp_ts);
    printf("%ld s %ld mikros   Pid %d: %s", tmp_ts.tv_sec, tmp_ts.tv_nsec / 1000, getpid(), msg);
}

//INVITATION HANDLER
void sig(int signum) {

    waiting_flag = 1;
    print_message("INVITATION signal\n");
}

//Decrease semaphore value
void decrease(int sem_id) {

    struct sembuf buf_tmp;
    buf_tmp.sem_num = 0;
    buf_tmp.sem_op = -1;
    buf_tmp.sem_flg = SEM_UNDO;
    semop(sem_id, &buf_tmp, 1);
}

//Increase semaphore vallue
void increase(int sem_id) {

    struct sembuf buf_tmp;
    buf_tmp.sem_num = 0;
    buf_tmp.sem_op = 1;
    buf_tmp.sem_flg = SEM_UNDO;
    semop(sem_id, &buf_tmp, 1);
}

//Waiting to achieve 0
void wait_for_lock(int sem_id) {

    struct sembuf buf_tmp;
    buf_tmp.sem_num = 0;
    buf_tmp.sem_op = 0;
    buf_tmp.sem_flg = 0;
    semop(sem_id, &buf_tmp, 1);
}

//Opening shared memory and semaphores
void open_barber() {

    char *tmp_path = getenv("HOME");

    // Shared memory segment opening
    key_t shm_key = ftok(tmp_path, SHM_ID);
    shmid = shmget(shm_key, 0, 0);
    my_barber = (struct barber*) shmat(shmid, 0, 0);

    if(my_barber == (void*)-1) {
        perror("Shared memory error");
        exit(1);
    }

    // Semaphores opening
    key_t queue_key = ftok(tmp_path, QUEUE_KEY_ID);
    queue_id = semget(queue_key, 0, 0);

    key_t chair_key = ftok(tmp_path, CHAIR_KEY_ID);
    chair_id = semget(chair_key, 0, 0);

    key_t shaving_key = ftok(tmp_path, SHAVING_KEY_ID);
    shaving_id = semget(shaving_key, 0, 0);
    
}

//Sending waking up to barber
void waking_up() {

    print_message("Send waking to Barber\n");
    kill(my_barber -> barber_pid, WAKEUP);
}

//Taking process in queue
void taking_place_in_queue() {

    add_to_queue(getpid()); 
    print_message("Sitting in the waiting room\n");
}

//Waiting in queue for invitation
void waiting(){
    sigset_t new_mask, old_mask;
    sigfillset(&new_mask);
    sigdelset(&new_mask, SIGINT);
    sigdelset(&new_mask, INVITATION);
    sigprocmask(SIG_SETMASK, &new_mask, &old_mask); 

    waiting_flag = 0;

    while(waiting_flag == 0)
        sigsuspend(&old_mask);

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}

//Taking place on chair
void taking_place_on_chair() {


    print_message("Sitting on the chair\n");
    my_barber -> shaved_client_pid = getpid();
    decrease(chair_id);
}

//Shaving procedure
void shaving() {

    wait_for_lock(shaving_id);  
    increase(chair_id);       
    print_message("Leaving Barber sheaven\n");

}

//Entering barber
int enter_barber() {

    decrease(queue_id);    

    if(my_barber -> is_sleeping == 1) {
        waking_up();
        taking_place_on_chair();
        increase(queue_id);    
        shaving();
    }

    else {

        if(my_barber -> current_size < my_barber->max_size) {
            taking_place_in_queue();
            increase(queue_id);
            waiting();         
            taking_place_on_chair();
            shaving();
        }

        else {
            print_message("There is no place in waiting room\n");
            increase(queue_id);    
            return 0;
        }
    }

    return 1;
}

int main(int argc, char** argv) {

    struct sigaction act;
    act.sa_handler = sig;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if(sigaction(INVITATION, &act, NULL) == -1) {

        perror("INVITE handler");
        exit(1);
    }

    if(argc != 3) {
        printf("Wrong nr of args\n");
        exit(1);
    }

    number_of_clients = atoi(argv[1]);

    number_of_shave = atoi(argv[2]);

    open_barber();

    for(int i = 0; i <number_of_clients; i++) {
        pid_t client = fork();

        if(client == 0) {

            int counter= 0;

            while(counter != number_of_shave)
                counter+= enter_barber();


            return 0;
        }
    }

    for(int i = 0; i < number_of_clients; i++)
        wait(NULL);
}
