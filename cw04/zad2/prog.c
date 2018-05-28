
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define to_parent_signal SIGRTMAX
#define to_child_signal SIGRTMAX-1


int children_number;
int request_limit;
int children_finished_ok;

//Groups
int show_create=1;
int show_request=1;
int show_approvement=1;
int show_rt_request=1;
int show_exit=1;

int created_number;
pid_t *created_processes;

int request_number;
pid_t *requested_processes;

int approvment_number;
pid_t *approvement_processes;

int rt_request_number;
pid_t *rt_requested_processes;

int exited_number;
pid_t *exited_processes;

int a=0;
int request_approved;

//Function to handle
void ch_sig_usr2(int signum){
    if(show_approvement==1)
        printf("Child %d: received parent's approval\n", getpid());

    request_approved=1;            
}

void ch_sig_quit(int signum){
    exit(1);
}


void set_child_handlers(){
    struct sigaction s_act1;
    sigfillset(&s_act1.sa_mask);
    s_act1.sa_handler=ch_sig_usr2;
    if(sigaction(to_child_signal, &s_act1, NULL)==-1){
        perror("Can not set SIGUSR2 handler for child process\n");
        exit(1);
    }

    struct sigaction s_act2;
    sigfillset(&s_act2.sa_mask);
    s_act2.sa_handler=ch_sig_quit;
    if(sigaction(SIGQUIT, &s_act2, NULL)==-1){
        perror("Can not set SIGQUIT handler for child process\n");
        exit(1);
    }    
}

void create_child(){
    pid_t id=fork();
    if(id<0){
        perror("Fork failed\n");
        exit(1);
    }
    else if (id==0){
        request_approved=0;

        set_child_handlers();

        srand(getpid());
        int sleep_time=rand()%11;
        sleep(sleep_time);

        
        union sigval value;
        
        sigset_t tmp_mask, old_mask;
        sigfillset(&tmp_mask);

        if(sigprocmask(SIG_BLOCK, &tmp_mask, &old_mask)==-1){
            perror("Can not set sigmask in child process\n");
            exit(1);
        }

        if(sigqueue(getppid(), to_parent_signal, value)==-1){
            perror("Can not send request signal to parent\n");
            exit(1);
        }


        if(show_request==1)
            printf("Child %d: sent request to parent process \n", getpid());

        while(request_approved==0)
            sigsuspend(&old_mask);


        int random_signal = SIGRTMIN + rand()%((SIGRTMAX-1)-SIGRTMIN);

        if(sigqueue(getppid(), random_signal, value)==-1){
            printf("Can not send SIGRT to parent process \n");
            exit(1);
        }

        if(show_rt_request==1)
            printf("Child %d: sent SIGRT %d to parent process %d\n", getpid(), rand_sig, getppid());    

        exit(1);
    }

    created_processes[created_number]=id;
    created_number++;

    if(show_create==1)
        printf("Parent: creating child process %d\n", id);
}

//Function to handle
void sig_int(int signum){
    printf("Received signal SIGINT\n");
    for(int i=0; i<created_number; i++){
        int local_flag=0;
        for(int j=0; j<exited_number; i++){
            if(exited_processes[j]==created_processes[i])
                local_flag=1;
        }

        if(local_flag==0)
            kill(created_processes[i], SIGQUIT);
    }
    children_finished_ok=2;
}

void sig_chld(int signum){
    int status;
    pid_t id;

    while((id=waitpid(-1, &status, WNOHANG))>0){
        int w_status=WEXITSTATUS(status);
        exited_processes[exited_number]=id;
        exited_number++;

        if(show_exit==1)
            printf("Parent: child process %d terminated with exit code %d\n", id, w_status);
    }

    if(exited_number==children_number)
        children_finished_ok=1;

}

void sig_usr1(int signum, siginfo_t *info, void *ucontext){
    requested_processes[request_number]=info->si_pid;
    if(show_request==1){
        printf("Parent: received request from child process %d\n", info->si_pid);
    }
    request_number++;

    if(request_number==request_limit){
        for(int i=0; i<request_number; i++){
            kill(requested_processes[i], to_child_signal);
            approvement_processes[approvment_number]=requested_processes[i];
            approvment_number++;

            if(show_approvement==1)
                printf("Parent: send approvement to child process %d\n", requested_processes[i]);
        }

    }
    else if(request_number>request_limit){
        kill(requested_processes[request_number-1], to_child_signal);
        approvement_processes[approvment_number]=requested_processes[request_number-1];
        approvment_number++;

        if(show_approvement==1)
        printf("Parent: send approvement to child process %d\n", requested_processes[request_number-1]);
    }

}

void sig_rt(int signum, siginfo_t *info, void *ucontext){
    rt_requested_processes[rt_request_number]=info->si_pid;
    rt_request_number++;

    if(show_rt_request==1)
        printf("Parent: received rt signal %d from child process %d\n", signum, info->si_pid);
}


//Handlers
void set_parent_handlers(){

    //SIGINT HANDLER
    struct sigaction act_int;
    act_int.sa_handler=sig_int;
    sigfillset(&act_int.sa_mask);
    if(sigaction(SIGINT, &act_int, NULL)==-1){
        perror("Can not set SIGINT handler\n");
        exit(1);
    }

    //SIGCHLD HANDLER
    struct sigaction act_chld;
    act_chld.sa_handler=sig_chld;
    sigfillset(&act_chld.sa_mask);
    act_chld.sa_flags=SA_NOCLDSTOP;
    if(sigaction(SIGCHLD, &act_chld, NULL)==-1){
        perror("Can not set SIGCHLD handler\n");
        exit(1);
    }

    //SIGUSR1/CURRENT SIGNAL HANDLER
    struct sigaction act_usr1;
    act_usr1.sa_sigaction=sig_usr1;
    sigfillset(&act_usr1.sa_mask);
    act_usr1.sa_flags=SA_SIGINFO;

    if(sigaction(to_parent_signal, &act_usr1, NULL)==-1){
        perror("Can not set SIGUSR1 handler\n");
        exit(1);
    }

    //REAL TIME HANDLER
    struct sigaction act_rt;
    act_rt.sa_sigaction=sig_rt;
    sigfillset(&act_rt.sa_mask);
    act_rt.sa_flags=SA_SIGINFO;

    for(int i=SIGRTMIN; i<SIGRTMAX-1; i++){
        if(sigaction(i, &act_rt, NULL)==-1){
            perror("Can not set SIGRT handler\n");
            exit(1);
        }
    }
}

void var_initialization(){
    
    children_finished_ok=0;


    created_processes=malloc(sizeof(pid_t)*children_number);
    created_number=0;

    requested_processes=malloc(sizeof(pid_t)*children_number);
    request_number=0;

    approvement_processes=malloc(sizeof(pid_t)*children_number);
    approvment_number=0;

    rt_requested_processes=malloc(sizeof(pid_t)*children_number);
    rt_request_number=0;

    exited_processes=malloc(sizeof(pid_t)*children_number);
    exited_number=0;
}


int main(int argc, char** argv){
    if(argc !=3){
        printf("Wrong number of args");
        exit(1);
    }

    children_number=atoi(argv[1]);
    request_limit=atoi(argv[2]);

    if(request_limit>children_number){
        printf("Request limit can not be higher than children number\n");
        exit(1);
    }

    var_initialization();
    set_parent_handlers();
    
    sigset_t new_mask, old_mask;
    sigemptyset(&new_mask);
    sigprocmask(SIG_BLOCK, &new_mask, &old_mask);

    for(int i=0; i<children_number; i++)
        create_child();

    while(children_finished_ok==0){
        sigsuspend(&old_mask);
    }
    

    printf("Finish!\n");


}