#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <ftw.h>

struct tm arg_date;
char operator;

char date_comparison(struct tm* first, struct tm* second){
    if((first->tm_year< second -> tm_year) || (first->tm_year==second->tm_year && first->tm_mon < second -> tm_mon) ||
        (first->tm_year==second->tm_year && first->tm_mon == second -> tm_mon && first-> tm_mday < second->tm_mday))
        return '<';
    else if((first->tm_year> second -> tm_year) || (first->tm_year==second->tm_year && first->tm_mon > second -> tm_mon) ||
        (first->tm_year==second->tm_year && first->tm_mon == second -> tm_mon && first-> tm_mday > second->tm_mday))
        return '>';
    else 
        return '=';    
}

void print_rights(const struct stat* stat_buffer){
    printf((stat_buffer -> st_mode & S_IRUSR) ? "r" : "-");
    printf((stat_buffer -> st_mode & S_IWUSR) ? "w" : "-");
    printf((stat_buffer -> st_mode & S_IXUSR) ? "x" : "-");
    printf((stat_buffer -> st_mode & S_IRGRP) ? "r" : "-");
    printf((stat_buffer -> st_mode & S_IWGRP) ? "w" : "-");
    printf((stat_buffer -> st_mode & S_IXGRP) ? "x" : "-");
    printf((stat_buffer -> st_mode & S_IROTH) ? "r" : "-");
    printf((stat_buffer -> st_mode & S_IWOTH) ? "w" : "-");
    printf((stat_buffer -> st_mode & S_IXOTH) ? "x" : "-");
    printf("\n");
}

void process(const char* path, const struct stat* stat_buffer){
    
    struct tm* local_date;
    local_date= localtime(&(stat_buffer -> st_mtime));


    if(date_comparison(local_date, &arg_date)== operator){
        printf("%s\n", path);
        char date_to_print[100];
        strftime(date_to_print, 100, "%b %d %Y", localtime(&(stat_buffer -> st_mtime)));
        printf("%s\n", date_to_print);
        print_rights(stat_buffer);
        printf("%ld\n\n", stat_buffer -> st_size);
    }
    
}

void search_dir(char* path){
    
    strcat(path, "/");
    int path_end_index = strlen(path);

    
    path[path_end_index] = '\0';

    DIR* current_dir;
    current_dir=opendir(path);
    if(current_dir==NULL){
        printf("Can not open directory\n");
        exit(1);
    }

    struct dirent* dir_temp;
    struct stat stat_buf;
    while((dir_temp= readdir(current_dir))!=NULL){

        if(strcmp(dir_temp -> d_name, ".")!=0 && strcmp(dir_temp -> d_name, "..") != 0){

            strcat(path, dir_temp -> d_name);

            lstat(path, &stat_buf);
            if(S_ISREG(stat_buf.st_mode)){
                process(path, &stat_buf);
            }

            else if(S_ISDIR(stat_buf.st_mode)){
                search_dir(path);
            }
            
           
            path[path_end_index] = '\0'; 

        }        
    }
}


int nftw_func(const char* path, const struct stat* stat_buf, int x, struct FTW* ftw){
    if(x==FTW_F)
        process(path, stat_buf);
    return 0;
}

int main(int argc, char** argv){
    if(argc!=5){
        printf("Wrong number of arguments\n");
        exit(1);
    }

    char* path = malloc(1000*sizeof(char));

    if(argv[1][0]=='/'){
        strcpy(path, argv[1]);
    }

    else{
        getcwd(path, 1000);
        strcat(path, "/");
        strcat(path, argv[1]);
    }

    printf("Path: %s\n", path);

    if(strcmp(argv[2], "<")==0){
        operator='<';
    }
    else if(strcmp(argv[2], "=")==0){
        operator='=';
    }
    else if(strcmp(argv[2], ">")==0){
        operator='>';
    }
    else{
        printf("Wrong operator\n");
        exit(1);
    }

    printf("Operator: %c\n", operator);
    strptime(argv[3], "%b %d %Y", &arg_date);

    if(strcmp(argv[4], "standard") == 0) {
        printf("searching with standard function:\n\n");
        search_dir(path);
    }

    else if(strcmp(argv[4], "nftw")==0){
        printf("searching with nftw function\n\n");
        nftw(path, nftw_func, 10, FTW_PHYS);

    }
    else{
        printf("choose standard or nftw function\n\n");
        exit(1);
    }

}