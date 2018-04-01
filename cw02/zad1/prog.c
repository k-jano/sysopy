#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <string.h>
#include <time.h>
#include <sys/times.h>

static clock_t begin;
static clock_t end;
static struct tms start;
static struct tms stop;
static int ticks_per_sec;

void startClock() {
  begin = times(&start);
}

void stopClock() {
  end = times(&stop);

  printf("real: %f ", (double) (end-begin)/ticks_per_sec); 
  printf("user: %f ", (double) (stop.tms_utime-start.tms_utime)/ticks_per_sec);
  printf("system: %f \n", (double)(stop.tms_stime-start.tms_stime)/ticks_per_sec);
}


void generate(char name[], int records_count, int records_size ){
    srand(time(NULL));
    int wy;
    char c;
    int i;   
    wy=open(name ,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR);
    for(i=0; i<records_count*records_size; i++){
        c=65+(rand()%26);
        write(wy, &c, 1);
    }
    close(wy); 
}


void sort(char name[], int records_count, int records_size){
    int i, j, wy;
    char* arr=(char*)malloc(records_size*sizeof(char));
    char* to_swap=(char*)malloc(records_size*sizeof(char));
    wy=open(name ,O_RDWR); 
    
    for(i=0; i<records_count; i++){
        for(j=i+1; j<records_count; j++){
            int record_position= records_size*i;
            lseek(wy, record_position * sizeof(char), SEEK_SET);
            read(wy, arr, records_size * sizeof(char));
            int swap_record_position=records_size*j;
            lseek(wy, swap_record_position * sizeof(char), SEEK_SET);
            read(wy, to_swap, records_size * sizeof(char));

            if(arr[0]>to_swap[0]){
                lseek(wy, record_position*sizeof(char), SEEK_SET);
                write(wy, to_swap, records_size*sizeof(char));
                lseek(wy, swap_record_position*sizeof(char), SEEK_SET);
                write(wy, arr, records_size*sizeof(char));
            }
        }
        
    }
}

void sortLib(char name[], int records_count, int records_size){
    int i, j;
    char* arr=(char*)malloc(records_size*sizeof(char));
    char* to_swap=(char*)malloc(records_size*sizeof(char));
    FILE *plik=fopen(name, "r+");

    for(i=0; i<records_count; i++){
        printf("%s\n", arr);
        for(j=i+1; j<records_count; j++){
            int record_position= records_size*i;
            fseek(plik, record_position * sizeof(char), SEEK_SET);
            fread(arr, sizeof(char), records_size, plik);
            
            int swap_record_position=records_size*j;
            fseek(plik, swap_record_position * sizeof(char), SEEK_SET);
            fread(to_swap, sizeof(char), records_size, plik);
            printf("%s", to_swap);
                
            if(arr[0]>to_swap[0]){
                fseek(plik, record_position*sizeof(char), SEEK_SET);
                fwrite(to_swap, 1, records_size, plik);
                fseek(plik, swap_record_position*sizeof(char), SEEK_SET);
                fwrite(arr, 1, records_size, plik);
            }
            
            
        }
        printf("\n");
        
    }
}

void copy(char name1[], char name2[], int records_count, int records_size){
    char block[records_size];
    int we, wy;
    int liczyt, i;
    we=open(name1, O_RDONLY);
    wy=open(name2, O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
    for(i=0; i<records_count; i++){
        liczyt=read(we, block, sizeof(block));
        write(wy, block, liczyt);
    }
    close(we);
    close(wy);
}

void copyLib(char name1[], char name2[], int records_count, int records_size){
    char block[records_size];
    int i;
    FILE *plik=fopen(name1, "r");
    FILE *plik2=fopen(name2, "w");
    for(i=0; i<records_count; i++){
        fread(block, 1, records_size, plik);
        fwrite(block, 1, records_size, plik2);
    }
    fclose(plik);
    fclose(plik2);
}

int main(int argc, char **argv) {
    ticks_per_sec = sysconf(_SC_CLK_TCK);
    char* operation=(char*)malloc(10*sizeof(char));
    char* way=(char*)malloc(3*sizeof(char));
    char* nameFile1=(char*)malloc(100*sizeof(char));
    char* nameFile2=(char*)malloc(100*sizeof(char));
    int records_count;
    int records_size;

    //Arguments
    if(argc <5 || argc>7){
        printf("Wrong number of arguments\n");
        exit(1);
    }
    if((strcmp(argv[1],"generate"))==0){
        if(argc!=5){
            printf("To generate enter 4 args\n");
            exit(1);
        }
        else{
            nameFile1=argv[2];
            records_count=atoi(argv[3]);
            records_size=atoi(argv[4]);
            operation="generate";            
        }
        
    }
    else if((strcmp(argv[1], "sort"))==0){
        if(argc!=6){
            printf("To sort enter 5 args\n");
            exit(1);
        }
        else{
            nameFile1=argv[2];
            records_count=atoi(argv[3]);
            records_size=atoi(argv[4]);
            operation="sort";
            if((strcmp(argv[5], "sys"))==0){
                way="sys";
            }    
            else if((strcmp(argv[5], "lib"))==0){
                way="lib";
            }
            else{
                printf("Choose way: sys or lib \n");
                exit(1);
            }    
        }
    }
    else if((strcmp(argv[1], "copy"))==0){
        if(argc!=7){
            printf("To copy enter 6 args\n");
            exit(1);
        }
        else{
            nameFile1=argv[2];
            nameFile2=argv[3];
            records_count=atoi(argv[4]);
            records_size=atoi(argv[5]);
            operation="copy";
            if((strcmp(argv[6], "sys"))==0){
                way="sys";
            }    
            else if((strcmp(argv[6], "lib"))==0){
                way="lib";
            }
            else{
                printf("Choose way: sys or lib \n");
                exit(1);
            }    
        }
    }
    else{
        printf("Wrong command");
        exit(1);
    }



    //Operations
    if((strcmp(operation,"generate"))==0){
        startClock();
        generate(nameFile1, records_count, records_size);
        stopClock(); 
    }
    else if((strcmp(operation,"sort"))==0){ 
        if((strcmp(way,"sys"))==0){
            startClock();
            sort(nameFile1, records_count, records_size);
            stopClock();
        }
        else if((strcmp(way,"lib"))==0){
            startClock();
            sortLib(nameFile1, records_count, records_size);
            stopClock();
        }
    }
    else if((strcmp(operation,"copy"))==0){
        if((strcmp(way,"sys"))==0){
            startClock();
            copy(nameFile1, nameFile2, records_count, records_size);
            stopClock();
        }
        else if ((strcmp(way,"lib"))==0){
            startClock();
            copyLib(nameFile1, nameFile2, records_count, records_size);
            stopClock();
        }
    }
} 