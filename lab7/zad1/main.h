#ifndef GENERAL_H

#define CLIENTS_QUEUE_MAX_SIZE  100


//SIGNALS
#define WAKEUP  SIGUSR1     
#define INVITATION  SIGUSR2     


#define QUEUE_KEY_ID   20  
#define CHAIR_KEY_ID    21  
#define SHAVING_KEY_ID  22  


//Shared memory key
#define SHM_ID  29  


struct barber{

    int max_size;
    int current_size;
    pid_t clients[CLIENTS_QUEUE_MAX_SIZE];
    int all_sits_number;
    pid_t barber_pid;
    pid_t shaved_client_pid;
    int is_sleeping;

};

#endif