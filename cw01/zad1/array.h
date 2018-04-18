#define staticMainArraySize 100000
#define staticBlockSize 50

typedef struct Block{
    char* charArray;
    int size;
}Block;

typedef struct MainArray{
    Block ** blocks;
    int size;
}MainArray;

typedef struct StaticArray{
    char array[staticMainArraySize][staticBlockSize];
    int lengths[staticMainArraySize];
}StaticArray;

//DYNAMIC
MainArray* createMainArr(int size);
void freeMainArr(MainArray *array);
int addBlock(MainArray *array, char* values, int size, int number);
void freeBlock(MainArray *array, int number);
Block* lowestDifference(MainArray* array, int number);

//STATIC
void staticCreateMainArr(StaticArray * array);
void staticAddBlock(StaticArray* array, char*value, int size, int number);
void staticFreeBlock(StaticArray *array, int number);
char* staticLowestDifference(StaticArray * array, int number);
