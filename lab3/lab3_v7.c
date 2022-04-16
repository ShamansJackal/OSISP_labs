#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

char **targetPaths;
int targetsCount =0;
int targetsSize = 1;

#define buffer_size 0x1000000
u_char buffer[buffer_size];

u_char lookup_table[256];

void init_table(){
    for(int i = 0; i<256; i++){
        lookup_table[i] = (i&1)+lookup_table[i/2];
    }
}

char *prog_name;

void task(char* file){
    ulong cod;
    ulong bytes_size = 0;
    ulong ones = 0;

    FILE* fileptr = fopen(file, "rb");
    if(fileptr==NULL){
        fprintf(stderr,"%d %s: can't open %s\n", getpid(), prog_name, file);
        exit(-1);
    }


    do{
        cod = fread(buffer, sizeof(char),buffer_size,fileptr);
        bytes_size += cod;

        for(int i = 0; i<cod;i++){
            ones += lookup_table[buffer[i]];
        }
    } while(cod != 0);

    printf("%d: %s %ld %ld %ld\n", getpid(), file, bytes_size, bytes_size*8-ones, ones);

    if(fclose(fileptr)){
        fprintf(stderr, "%d: %s %d: closefile:%s\n", getpid(), prog_name, strerror(errno));
        exit(-1);
    }
}

int str_len = PATH_MAX;

char **resize_array(char** array, int *size){
    *size = *size<<1;
    char** tmp = (char**)malloc(*size*sizeof(char*));
    for (int i = 0; i < *size>>1; i++)
    {
        tmp[i]=array[i];
    }
    free(array);
    return tmp;
}

char **add_to_array(char** array, char* elm, int*ind, int*size){
    char *tmp = (char*)calloc(str_len,sizeof(char));
    strcpy(tmp, elm);

    if(*ind>=*size){
        array = resize_array(array, size);
    }
    array[*ind] = tmp;
    *ind = *ind+1;

    return array;
}

DIR *_opendir(const char *__name) {
    DIR *dir;
    dir = opendir(__name);
    if (dir == NULL) {
        fprintf(stderr, "%d: %s: opendir(%s): %s\n", getpid(), prog_name , __name, strerror(errno));
        errno = 0;
    }
    return dir;
}

void walk(DIR *dir, const char *path, char*** array_of_paths, int* ind, int* size) {
    char* fullpath = (char*)calloc(str_len, sizeof(char));
    struct stat pstatus;
    struct dirent *dir_entry;

    do
    {
        dir_entry = readdir(dir);
        if (!dir_entry && errno)
        {
            fprintf(stderr, "%d: %s: readdir:%s\n", getpid(), prog_name, strerror(errno));
            errno = 0;
            break;
        } else if (!dir_entry)
        {
            break;
        }

        if ((strcmp(dir_entry->d_name, ".") == 0) || (strcmp(dir_entry->d_name, "..") == 0))
        {
            continue;
        }

        strcpy(fullpath, path);

        if (fullpath[strlen(fullpath) - 1] != '/')
            strcat(fullpath, "/");
        strcat(fullpath, dir_entry->d_name);

        if (lstat(fullpath, &pstatus) == 0)
        {
            if (S_ISREG(pstatus.st_mode) && ((pstatus.st_mode & 0444) > 0))
            {
                *array_of_paths = add_to_array(*array_of_paths, fullpath, ind, size);
            }else if (S_ISDIR(pstatus.st_mode))
            {
                DIR *dir2 = _opendir(fullpath);
                if (dir2 == NULL) continue;
                walk(dir2, fullpath, array_of_paths, ind, size);
            }
        }
        else
        {
            fprintf(stderr, "%d: %s %d: lstat:%s\n", getpid(), prog_name, strerror(errno));
            errno = 0;
            continue;
        }
    } while (dir_entry);

    if(closedir(dir))
        fprintf(stderr, "%d: %s %d: closedir:%s\n", getpid(), prog_name, strerror(errno));
}

int main(int argc, char *argv[]){
    int counter = 1;
    int mxCount = 1;

    init_table();

    prog_name=basename(argv[0]);
    if(argc < 3){
        fprintf(stderr, "%d %s: Not enogth arguments\n", getpid(), prog_name);
        exit(-1);
    }
    targetPaths = (char**)malloc(sizeof(char*));

    char* targetDirName = malloc(strlen(argv[1]));
    strcpy(targetDirName, argv[1]);
    targetDirName = realpath(targetDirName, NULL);

    mxCount = atoi(argv[2]);
    if (mxCount < 2) {
        fprintf(stderr, "%d %s: 3st argument is incorrect\n",getpid(), prog_name);
        exit(-1);
    }

    DIR* dir = _opendir(targetDirName);
    if (dir == NULL) {
        exit(errno);
    } else{
        walk(dir, targetDirName, &targetPaths, &targetsCount, &targetsSize);
    }

    for(int i=0;i<targetsCount;i++){
        if(counter >= mxCount){
            wait(0);
            counter--;
        }

        pid_t pid = fork();
        if (pid == -1) {
            fprintf(stderr, "%d %s: fork: %s\n", getpid(), prog_name, strerror(errno));
            exit(-1);
        } else if (pid == 0) {
            task(targetPaths[i]);
            return 0;
        } else {
            counter++;
        }
    }

    while (wait(0) != -1) {}

    return 0;
}
