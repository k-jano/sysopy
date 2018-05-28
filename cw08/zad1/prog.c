#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>

int ** image_to_open;
int ** image_to_save;

int image_width;
int image_heigth;

double ** filter;
int filter_width;

int pthread_number;

int ticks_per_sec;

clock_t begin;
clock_t end;
struct tms start;
struct tms stop;

void start_clock() {
  begin=times(&start);
}


void stop_clock() {
  end=times(&stop);

  printf("real: %.2f \n",(double)(end-begin)/100); 
}

int my_round(double x){
    int lower, upper;
    lower=(int)x;
    upper=(int)x+1;

    if((double)upper-x<=x-(double)lower)
        return upper;
    else 
        return lower;
}

int my_ceil(double x){
    int upper; 
    int lower;
    lower=(int)x;
    upper=(int)x+1;
    if(x-(double)lower==0.0)
        return lower;
    return upper;
}

int my_max(int x, int y){
    if(x>=y)
        return x;
    else
        return y;
}

int my_min(int x, int y){
    if(x<=y)
        return x;
    else
        return y;   
}

void scanning_files(char* image_path, char* filter_path){
    //Opening
    FILE * image_file = fopen(image_path, "r");
    FILE* filter_file = fopen(filter_path, "r");

    char buffer[100];
    fscanf(image_file, "%s", buffer);
    fscanf(image_file, "%d", &image_width);
    fscanf(image_file, "%d", &image_heigth);
    //printf("Szerokosc %d i wysokosc %d\n", image_width, image_heigth);

    fscanf(image_file, "%s", buffer);

    fscanf(filter_file, "%d", &filter_width);
    //printf("Wymiar filtru %d\n", filter_width);

    //Allocating
    image_to_open=malloc(image_heigth* sizeof(int*));
    for(int i=0; i<image_heigth; i++){
        image_to_open[i]=malloc(image_width*sizeof(int));
    }

    image_to_save=malloc(image_heigth* sizeof(int*));
    for(int i=0; i<image_heigth; i++){
        image_to_save[i]=malloc(image_width*sizeof(int));
    }

    filter = malloc(filter_width*sizeof(double*));
    for(int i=0; i<filter_width; i++){
        filter[i]=malloc(filter_width*sizeof(double));
    }

    for(int i=0; i<image_heigth; i++){
        for(int j=0; j<image_width; j++){
            fscanf(image_file, "%d", &image_to_open[i][j]);
        }
    }

    for(int i=0; i<filter_width; i++){
        for(int j=0; j<filter_width; j++){
            fscanf(filter_file, "%lf", &filter[i][j]);
        }
    }

    
    fclose(image_file);
    fclose(filter_file);
}

void calculating_output(int tmp_heigth, int n){
    for(int i=n*(image_heigth/pthread_number); i<tmp_heigth+n*(image_heigth/pthread_number); i++){
        for(int j=0; j<image_width; j++){
            int sum=0;
            for(int k=0; k<filter_width; k++){
                for(int l=0; l<filter_width; l++){
                    
                    int parameter1=my_min(image_heigth-1,my_max(0, i- my_ceil((double)filter_width/2.0)+k));
                    int parameter2=my_min(image_width-1, my_max(0, j-my_ceil((double)filter_width/2.0)+l));
                    sum+= image_to_open[parameter1][parameter2]*filter[k][l];
                }
            }

            image_to_save[i][j]=my_round(sum);
        }
    }
}


void saving_image(char* output_image_path){
    FILE * output= fopen(output_image_path, "w+");

    fprintf(output, "P2\n");
    fprintf(output, "%d %d\n", image_width, image_heigth);
    fprintf(output, "255\n");

    for(int i=0; i<image_heigth; i++){
        for(int j=0; j<image_width; j++)
            fprintf(output, "%d ", image_to_save[i][j]);
        
        fprintf(output, "\n");
    }

    fclose(output);
}

void* pthread_task(void* arg){
    int* n=arg;
    int tmp_heigth;
    if(*n==pthread_number-1)
        tmp_heigth=image_heigth/pthread_number+image_heigth%pthread_number;
    else
        tmp_heigth=image_heigth/pthread_number;
    calculating_output(tmp_heigth, *n);
    return NULL;
}

int main(int argc, char** argv){

    if(argc !=5){
        perror("Wrong nr of args\n");
        exit(1);
    }

    pthread_number=atoi(argv[1]);
    char* image_path= argv[2];
    char* filter_path= argv[3];
    char* output_image_path=argv[4];

    scanning_files(image_path, filter_path);

    start_clock();

    int index[pthread_number];
    for(int i=0; i<pthread_number; i++){
        index[i]=i;
    }
    pthread_t array[pthread_number];
    for(int i=0; i<pthread_number; i++){
        pthread_create(&array[i], NULL, pthread_task, &index[i]);
    }
    for(int i=0; i<pthread_number; i++){
        pthread_join(array[i], NULL);
    }

    stop_clock();

    saving_image(output_image_path);

}