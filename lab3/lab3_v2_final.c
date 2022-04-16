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

typedef struct pathstruct {
    char* path;
    ulong size;
} pathstruct;

pathstruct *targetPaths;
int targetsCount =0;
int targetsSize = 1;

pathstruct *sourcePaths;
int sourceCount =0;
int sourceSize = 1;

#define buffer_size 0x1000000
char buffer1[buffer_size];
char buffer2[buffer_size];

char *prog_name;

int compare_files(pathstruct file1, pathstruct file2, ulong* bytes_size){
    if(file1.size!=file2.size) return 0;
    ulong cod1, cod2;
    int is_same=1;
    *bytes_size = 0;
    FILE* fileptr1 = fopen(file1.path, "rb");
    if(fileptr1==NULL){ 
        fprintf(stderr,"%d %s: can't open %s\n", getpid(), prog_name, file1);
        return 0;
    }

    FILE* fileptr2 = fopen(file2.path, "rb");
    if(fileptr2==NULL){
        fprintf(stderr,"%d %s: can't open %s\n", getpid(), prog_name, file2);
        fclose(fileptr1);
        return 0;
    } 
    do{
        cod1 = fread(buffer1, sizeof(char),buffer_size,fileptr1);
        cod2 = fread(buffer2, sizeof(char),buffer_size,fileptr2);
        *bytes_size = *bytes_size + cod1;

        if(cod1!=cod2){
            is_same = 0;
            break;
        }

        for(int j = 0; j<cod1;j++){
            if(buffer1[j]!=buffer2[j]){
                is_same = 0;
                break;
            }
        }
        if(!is_same) break;
    } while(cod1 != 0 && cod2 != 0);

    
    if(fclose(fileptr1)) fprintf(stderr,"%d %s: %s %s\n", getpid(), prog_name, strerror(errno), file1);
    if(fclose(fileptr2)) fprintf(stderr,"%d %s: %s %s\n", getpid(), prog_name, strerror(errno), file1);
    return is_same;
}
int str_len = PATH_MAX;


pathstruct *resize_array(pathstruct* array, int *size){
    *size = *size<<1;
    pathstruct* tmp = (pathstruct*)malloc(*size*sizeof(pathstruct));
    for (int i = 0; i < *size>>1; i++)
    {
        tmp[i]=array[i];
    }
    free(array);
    return tmp;
}

pathstruct *add_to_array(pathstruct* array, char* elm, int*ind, int*size,const ulong file_size){
    pathstruct tmp;
    tmp.size = file_size;
    tmp.path = (char*)calloc(str_len,sizeof(char));
    strcpy(tmp.path, elm);

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

void walk(DIR *dir, const char *path, pathstruct** array_of_paths, int* ind, int* size) {
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
                *array_of_paths = add_to_array(*array_of_paths, fullpath, ind, size, pstatus.st_size);
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

    prog_name=basename(argv[0]);
    if(argc < 4){
        fprintf(stderr, "%d %s: Not enogth arguments\n", getpid(), prog_name);
        exit(-1);
    }
    targetPaths = (pathstruct*)malloc(sizeof(pathstruct));
    sourcePaths = (pathstruct*)malloc(sizeof(pathstruct));

    char* targetDirName = malloc(strlen(argv[2]));
    strcpy(targetDirName, argv[2]);
    targetDirName = realpath(targetDirName, NULL);

    char* sourceDirName = malloc(strlen(argv[1]));
    strcpy(sourceDirName, argv[1]);
    sourceDirName = realpath(sourceDirName, NULL);

    mxCount = atoi(argv[3]);
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

    dir = _opendir(sourceDirName);
    if (dir == NULL) {
        exit(errno);
    } else{
        walk(dir, sourceDirName, &sourcePaths, &sourceCount, &sourceSize);
    }

    for(int i=0;i<targetsCount;i++){
        for(int j=0;j<sourceCount;j++){
            if(counter >= mxCount){
                wait(0);
                counter--;
            }

            pid_t pid = fork();
            if (pid == -1) {
                fprintf(stderr, "%d %s: fork: %s\n", getpid(), prog_name, strerror(errno));
                exit(-1);
            } else if (pid == 0) {
                ulong total_bytes = 0;
                if(compare_files(sourcePaths[j], targetPaths[i], &total_bytes)){
                    printf("%d: %s %ld %s == %s\n", getpid(), sourcePaths[j].path, total_bytes, sourcePaths[j].path, targetPaths[i].path);
                } else {
        	        printf("%d: %s %ld %s != %s\n", getpid(), sourcePaths[j].path, total_bytes, sourcePaths[j].path, targetPaths[i].path);
		        }
                exit(0);
            } else {
                counter++;
            }
        }
    }

    while (wait(0) != -1) {}

    return 0;
}
