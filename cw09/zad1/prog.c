#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

#define CYAN "\x1b[36m"
#define MAGENTA "\x1b[35m"
#define RED "\x1b[91m"
#define GREEN "\x1b[32m"

size_t max_line_size=1024;

//Settings
int producer_number;
int customer_number;
int buffer_size;
char* source_path; 
int compare_value;
char* comparator;
char* output_mode;
int nk_value;

//Input
FILE* input;

//Buffer
char** main_array;
int* array_lengths;
int producer_actual_number;
int customer_actual_number;
int elements_to_read;

//Mutex
pthread_mutex_t number_mutex;
pthread_mutex_t waiting_mutex;

//Cond
pthread_cond_t full_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t main_cond = PTHREAD_COND_INITIALIZER;

//Pthreads
pthread_t* producers;
pthread_t* customers;

//Main flag
int main_flag;

void read_settings(char* settings){
    FILE* setting_file=fopen(settings, "r");
    if(setting_file==NULL){
        perror("Can not open setting file\n");
        exit(1);
    }

    fscanf(setting_file, "%d", &producer_number);
    if(producer_number<0){
        perror("Wrong nr of producers\n");
        exit(1);
    }

    fscanf(setting_file, "%d", &customer_number);
    if(customer_number<0){
        perror("Wrong nr of consumers\n");
        exit(1);
    }

    fscanf(setting_file, "%d", &buffer_size);
    if(buffer_size<0){
        perror("Wrong nr of buff_size\n");
        exit(1);
    }

    source_path=(char*)malloc(max_line_size*sizeof(char));
    fscanf(setting_file, "%s", source_path);

    input=fopen(source_path, "r");
    if(input==NULL){
        perror("Can not open file to read\n");
        exit(1);
    }

    fscanf(setting_file, "%d", &compare_value);

    comparator=(char*)malloc(max_line_size*sizeof(char));
    fscanf(setting_file, "%s", comparator);
    if(strcmp(comparator, "<")!=0 && strcmp(comparator, "=")!=0 && strcmp(comparator, ">")!=0){
        printf("Wrong comparator\n");
        exit(1);
    }

    output_mode=(char*)malloc(max_line_size*sizeof(char));
    fscanf(setting_file, "%s", output_mode);
    if(strcmp(output_mode, "extended")!=0 && strcmp(output_mode, "simple")!=0){
        printf("Wrong output_mode\n");
        exit(1);
    }

    fscanf(setting_file, "%d", &nk_value);
    if(nk_value<0){
        printf("Wrong nk value\n");
        exit(1);
    }

    fclose(setting_file);
}

void buffer_initialization(){
    main_array=(char**)malloc(buffer_size*sizeof(char*));
    array_lengths=(int*)malloc(buffer_size*sizeof(int));
    producer_actual_number=0;
    customer_actual_number=0;
    elements_to_read=0;
    for(int i=0; i<buffer_size; i++){
        array_lengths[i]=0;
    }
}

void sig_int(int signum){
    printf("SIGINT signal received\n");
    exit(1);
}

void printing(char* sentence, int sentence_length){
    printf(CYAN"Customer: %s o dlugosci %d\n", sentence, sentence_length);
}

void customer_operation(int my_number){
    char tmp[max_line_size];
    strcpy(tmp, main_array[my_number]);
    int tmp_length=array_lengths[my_number];
    free(main_array[my_number]);
    array_lengths[my_number]=0;
    elements_to_read--;

    if(strcmp("<", comparator)==0){
        if(tmp_length<compare_value)
            printing(tmp, tmp_length);
    }
    else if(strcmp("=", comparator)==0){
        if(tmp_length==compare_value)
            printing(tmp, tmp_length);
    }
    else if(strcmp(">", comparator)==0){
        if(tmp_length>compare_value)
            printing(tmp, tmp_length);
    }

}

void* producer_task(){

    char* tmp=(char*)malloc(max_line_size*sizeof(char));

    int producer_counter=0;
    while(1){
        pthread_mutex_lock(&number_mutex);
        producer_counter++;
        while(elements_to_read>=buffer_size){
            pthread_cond_wait(&full_cond, &number_mutex);
        }

        while(array_lengths[producer_actual_number]!=0)
            producer_actual_number=(producer_actual_number+1)%buffer_size;

        int my_number=producer_actual_number;
        producer_actual_number=(producer_actual_number+1)%buffer_size;

        if(getline(&tmp, &max_line_size, input)==-1){
            if(strcmp(output_mode, "extended")==0)
                printf(RED"Koniec pliku\n");
            pthread_mutex_unlock(&number_mutex);
            main_flag=1;
            pthread_cond_signal(&main_cond);
            break;
        }
        main_array[my_number]=(char*)malloc(max_line_size*sizeof(char));
        strcpy(main_array[my_number], tmp);
        array_lengths[my_number]=strlen(main_array[my_number]);
        elements_to_read++;
        if(strcmp(output_mode, "extended")==0)
            printf(MAGENTA"Producer: Wpisalem: %s o dlugosci %d w %d numer bufora\n", main_array[my_number], array_lengths[my_number], my_number);

        pthread_cond_signal(&empty_cond);
        pthread_mutex_unlock(&number_mutex);

    }
    return NULL;
}


void* customer_task(){
    while(1){
        pthread_mutex_lock(&number_mutex);

        while(elements_to_read<=0)
            pthread_cond_wait(&empty_cond, &number_mutex);

        while(array_lengths[customer_actual_number]==0)
            customer_actual_number=(customer_actual_number+1)%buffer_size;

        int my_number=customer_actual_number;
        customer_actual_number=(customer_actual_number+1)%buffer_size;

        customer_operation(my_number);

        pthread_cond_signal(&full_cond);
        pthread_mutex_unlock(&number_mutex);
    }
}

void exit_fun(){
    if(main_array)
        free(main_array);
    if(input)
        fclose(input);
    pthread_mutex_destroy(&number_mutex);
    pthread_mutex_destroy(&waiting_mutex);
    pthread_cond_destroy(&full_cond);
    pthread_cond_destroy(&empty_cond);
    pthread_cond_destroy(&main_cond);    
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

    if(argc!=2){
        perror("Wrong nr of args\n");
        exit(1);
    }

    main_flag=0;

    pthread_mutex_init(&number_mutex, NULL);
    pthread_mutex_init(&waiting_mutex, NULL);


    char* settings=argv[1];
    read_settings(settings);
    printf("%d %d %d %s %d %s %s %d\n", producer_number, customer_number, buffer_size, source_path, compare_value, comparator, output_mode, nk_value);

    
    buffer_initialization();
    
    producers=(pthread_t*)malloc(producer_number*sizeof(pthread_t));
    customers=(pthread_t*)malloc(customer_number*sizeof(pthread_t));


    for(int i=0; i<producer_number; i++){
        pthread_create(&producers[i], NULL, producer_task, NULL);
    }

    for(int i=0; i<customer_number; i++){
        pthread_create(&customers[i], NULL, customer_task, NULL);
    }
    
    if(nk_value>0){
        sleep(nk_value);
        printf(GREEN"Succes after sleeping!!\n");
        exit(1);
    }

    if(main_flag==0){
        pthread_cond_wait(&main_cond, &waiting_mutex);
    }


    while(1){
        pthread_mutex_lock(&number_mutex);
        if(elements_to_read<=0)
            break;
        pthread_mutex_unlock(&number_mutex);
    }

    printf(GREEN"Success!!\n");
}