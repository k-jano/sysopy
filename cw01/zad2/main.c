#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include "array.h"
#include <string.h>


static int ticks_per_sec;

static clock_t begin;
static clock_t end;
static struct tms start;
static struct tms stop;

void startClock() {
  begin = times(&start);
}

void stopClock() {
  end = times(&stop);

  printf("real: %f ", (double) (end-begin)/ticks_per_sec); 
  printf("user: %f ", (double) (stop.tms_utime-start.tms_utime)/ticks_per_sec);
  printf("system: %f \n", (double)(stop.tms_stime-start.tms_stime)/ticks_per_sec);
}

int randomize( int count){
    srand(time(NULL));
    int x;
    return x=rand()%count;
}


int main(int argc, char **argv){
    #ifdef _DYNAMIC

    void *handle=dlopen("./lib_array.so", RTLD_LAZY);
    if(!handle){
        fprintf(stderr, "%s\n", dlerror());
        return 1;
    }

    MainArray* (*createMainArr)(int);
    createMainArr = dlsym(handle, "createMainArr");

    void (*freeMainArr)(MainArray *);
    freeMainArr = dlsym(handle, "freeMainArr");

    int (*addBlock)(MainArray *, char* , int, int);
    addBlock = dlsym(handle, "addBlock");

    void (*freeBlock)(MainArray *, int );
    freeBlock=dlsym(handle, "freeBlock");

    Block* (*lowestDifference)(MainArray* , int);
    lowestDifference=dlsym(handle, "lowestDifference");

    void (*staticCreateMainArr)(StaticArray *);
    staticCreateMainArr=dlsym(handle, "staticCreateMainArr");

    void (*staticAddBlock)(StaticArray*, char*, int , int );
    staticAddBlock=dlsym(handle, "staticAddBlock");

    void (*staticFreeBlock)(StaticArray *, int);
    staticFreeBlock=dlsym(handle, "staticFreeBlock");

    char* (*staticLowestDifference)(StaticArray *, int);
    staticLowestDifference=dlsym(handle, "staticLowestDifference");
    #endif


    ticks_per_sec = sysconf(_SC_CLK_TCK);
    int range=100;
    int value_count = 7;
    int value_len[7] = {8, 4, 11, 5, 10, 9, 20};
    char values[7][20] = {
    "przyklad", 
    "test", 
    "abrakadabra", 
    "eowoe",
    "assemblery",
    "123456789",
    "aaaaaaaaaaaaaaaaaaaa"};
    
    int i,j, r;
    randomize(value_count);
    randomize(range);


    //DYNAMIC

    printf("\nDynamic\n");

    startClock();
    MainArray* array= createMainArr(staticMainArraySize);
    stopClock();


    startClock();
    for(i=0; i<staticMainArraySize; i++){
        addBlock(array, values[j], value_len[j], i);
        j=randomize(value_count);
    }
    stopClock();


    startClock();
    for(i=0; i<range; i++){
        lowestDifference(array, r);
        r=randomize(range);
    }    
    stopClock();

    startClock();
    for(i=0; i<staticMainArraySize; i++) {
        freeBlock(array, i);
    }
    for(i=0; i<staticMainArraySize; i++) {
        addBlock(array, values[j], value_len[j], i);
        j=randomize(value_count);
    }
    stopClock();


    startClock();
    for(i=0; i<staticMainArraySize; i++){
        freeBlock(array, i);
        addBlock(array, values[j], value_len[j], i);
        j=randomize(value_count);
    }
    stopClock();

    freeMainArr(array);

    //STATIC

    printf("\nStatic\n");

    startClock();
    StaticArray static_array;
    staticCreateMainArr(&static_array);

    stopClock();

    startClock();
    for(i=0; i<staticMainArraySize; i++){
        staticAddBlock(&static_array, values[j], value_len[j], i);
        j=randomize(value_count);
    }

    stopClock();
    

    startClock();
    for(i=0; i<range; i++){
        staticLowestDifference(&static_array, r);
        r=randomize(range);
    }    
    stopClock();

    startClock();
    for(i=0; i<staticMainArraySize; i++){
        staticFreeBlock(&static_array, i);
    }
    for(i=0; i<staticMainArraySize; i++){
        staticAddBlock(&static_array, values[j], value_len[j], i);
        j=randomize(value_count);
    }
    stopClock();

    startClock();
    for(i=0; i<staticMainArraySize; i++){
        staticFreeBlock(&static_array, i);
        staticAddBlock(&static_array, values[j], value_len[j], i);
        j=randomize(value_count);
    }
    stopClock();




    //ARG

    int mainArraySize;
    //int blockSize;
    char* way=(char*)malloc(10*sizeof(char));
    char** operation=(char**)malloc(3*sizeof(char**));
    for(i=0; i<3; i++){
        operation[i]=(char*)malloc(20*sizeof(char));
    }
    int* number=(int*)malloc(3*sizeof(int));

    if(argc!=10){
        printf("Wrong numbers of arguments\n");
        exit(1);
    }
    else{
        mainArraySize=atoi(argv[1]);
        //blockSize=atoi(argv[2]);
        if((strcmp(argv[3], "dynamic"))==0){
            printf("Heloo dynamic\n");
            way="dynamic";
            MainArray* array= createMainArr(mainArraySize);
            for(i=0; i<mainArraySize; i++){
                addBlock(array, values[j], value_len[j], i);
                j=randomize(value_count);
            }    
        }
        else if((strcmp(argv[3], "static"))==0){
            printf("Hello static\n");
            way="static";
            staticCreateMainArr(&static_array);
            for(i=0; i<mainArraySize; i++){
                staticAddBlock(&static_array, values[j], value_len[j], i);
                j=randomize(value_count);
            }
        }
        else{
            printf("Choose correct way\n");
            exit(1);
        }

        //Operations args
        for(i=4; i<=8; i+=2){
            if((strcmp(argv[i], "search_element"))==0){
                operation[i/2-2]="search_element";                
            }
            else if((strcmp(argv[i], "remove"))==0){
                operation[i/2-2]="remove";                
            }
            else if((strcmp(argv[i], "add"))==0){
                operation[i/2-2]="add";                
            }
            else if((strcmp(argv[i], "remove_and_add"))==0){
                operation[i/2-2]="remove_and_add";                
            }
            else{
                printf("Operation not found\n");
                exit(1);
            }
            number[i/2-2]=atoi(argv[i+1]);
        }
    }

    //Operations
    for(i=0; i<3; i++){
        //Dynamic
        if((strcmp(way, "dynamic"))==0){
            if((strcmp(operation[i], "search_element"))==0){
                lowestDifference(array, number[i]);
            }
            else if((strcmp(operation[i], "remove"))==0){
                for(i=0; i<number[i]; i++){
                    freeBlock(array, i);
                }
            }
            else if((strcmp(operation[i], "add"))==0){
                for(i=0; i<number[i]; i++){
                    addBlock(array, values[j], value_len[j], i);
                    j=randomize(value_count);
                }
            }
            else if((strcmp(operation[i], "remove_and_add"))==0){
                for(i=0; i<number[i]; i++){
                    freeBlock(array, i);
                    addBlock(array, values[j], value_len[j], i);
                    j=randomize(value_count);
                }
            }
        }
        //Static
        else{
            if((strcmp(operation[i], "search_element"))==0){
                staticLowestDifference(&static_array, number[i]);
            }
            else if((strcmp(operation[i], "remove"))==0){
                for(i=0; i<number[i]; i++){
                    staticFreeBlock(&static_array, i);
                }
            }
            else if((strcmp(operation[i], "add"))==0){
                for(i=0; i<number[i]; i++){
                    staticAddBlock(&static_array, values[j], value_len[j], i);
                    j=randomize(value_count);
                }
            }
            else if((strcmp(operation[i], "remove_and_add"))==0){
                for(i=0; i<number[i]; i++){
                    staticFreeBlock(&static_array, i);
                    staticAddBlock(&static_array, values[j], value_len[j], i);
                    j=randomize(value_count);
                }
            }
        }
    }
    


}
