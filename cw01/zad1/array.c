#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "array.h"


MainArray* createMainArr(int size) {
    int i;
    MainArray *array=malloc(sizeof(MainArray));    
    if(array == NULL){
        printf("Can't alloc array \n");
        return NULL;
    }

    array->size=size;
    array->blocks=malloc(size*sizeof(Block*));
    if(array->blocks==NULL){
        printf("Can't alloc array \n");
        free(array);
        return NULL;
    }
    for(i=0; i<size; i++){
        array->blocks[i]=NULL;
    }
    return array;
    
}

void freeMainArr(MainArray *array){
    int i;
    for(i=0; i<array->size; i++){
        if(array->blocks[i]!=NULL){
            free(array->blocks[i]);
        }
    }
    free(array);
}

int addBlock(MainArray *array, char* values, int size, int number){
    int j;
    
    
    if(array->blocks[number]==NULL){
        array->blocks[number]=malloc(sizeof(Block));
        array->blocks[number]->charArray=malloc(size*sizeof(char));
        for(j=0; j<size; j++){
            array->blocks[number]->charArray[j]=values[j];
        }
        array->blocks[number]->size=size;
        
    }    
    
    return 0;


}

void freeBlock(MainArray *array, int number){
    if(array->blocks[number]!=NULL){
        if(array->blocks[number]->charArray!=NULL){
            free(array->blocks[number]->charArray);
            free(array->blocks[number]);
            array->blocks[number]=NULL;
        }
    }

    
}

Block* lowestDifference(MainArray* array, int number){
    int i,j;
    Block* blockTest= NULL;
    int differences[array->size];
    for(i=0; i<array->size; i++){
        blockTest=array->blocks[i];
        if(blockTest==NULL){
            differences[i]=-1;
        }
        else{
            differences[i]=0;
            for(j=0;j<blockTest->size; j++){
                differences[i]+=blockTest->charArray[j];
            }
            differences[i]=abs(differences[i]-number);
        }
    }
    int minTmp=-1;
    i=0;
    while(minTmp==-1 && i<array->size){
        if(differences[i]==-1){
            i=i+1;
        }
        else{
            minTmp=differences[i];
            blockTest=array->blocks[i];
        }
    }
    if(minTmp==-1){
        return NULL;
    }

    else{
        for(i=0; i<array->size; i++){
            if(differences[i]!=-1 && differences[i]<minTmp){
                minTmp=differences[i];
                blockTest=array->blocks[i];
            }
        }
    }
    return blockTest;
}

void staticCreateMainArr(StaticArray * array){
    int i,j;

    for(i=0; i<staticMainArraySize; i++){
        for(j=0; j<staticBlockSize; j++){
            array->array[i][j]=0;
        }
        array->lengths[i]=0;
    }
    
}

void staticFreeBlock(StaticArray *array, int number){
    if(number<staticMainArraySize){
        int i;
        for(i=0; i<array->lengths[number]; i++){
            array->array[number][i]=0;
        }
        array->lengths[number]=0;
    }
}

void staticAddBlock(StaticArray* array, char*value, int size, int number){
    int i;
    if(number<staticMainArraySize){
        for(i=0; i<size; i++){
            array->array[number][i]=value[i];
        }
        array->lengths[i]=size;
    }
}

char* staticLowestDifference(StaticArray * array, int number){
    int i, j;
    int differences[staticMainArraySize];
    char* blockTest=(char*)malloc(staticBlockSize*sizeof(char));
    int tmp;
    for(i=0; i<staticMainArraySize; i++){
        if((array->lengths[i]=0)){
            differences[i]=-1;
        }
        else{
            differences[i]=0;
            for(j=0; j<staticBlockSize; j++){
                differences[i]+=array->array[i][j];
            }
            differences[i]= abs(differences[i]-number);
        }    
        
    }

    
    int minTmp=-1;
    i=0;
    while(minTmp==-1 && i<staticMainArraySize){
        if(differences[i]==-1){
            i+=1;
        }
        else{
            minTmp=differences[i];
            tmp=i;
        }
    }
    if(minTmp==-1){
        return NULL;
    }

    else{
        for(i=0; i<staticMainArraySize; i++){
            if(differences[i]!=-1 && differences[i]<minTmp){
                minTmp=differences[i];
                tmp=i;
            }
        }
    }
    for(i=0; i<array->lengths[tmp]; i++){
        blockTest[i]=array->array[tmp][i];
    }
    return blockTest;
}