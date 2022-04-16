#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

int min;
int max;
long int counter;
char *prog_name;

FILE *out;

DIR *_opendir(const char *__name) {
    DIR *dir;
    dir = opendir(__name);
    if (dir == NULL) {
        fprintf(stderr, "%s: opendir(%s): %s\n", prog_name + 2, __name, strerror(errno));
    }
    return dir;
}

void showdir(DIR *dir, const char *path) {
    char fullpath[PATH_MAX + 1];
    struct stat pstatus;
    struct dirent *dir_entry;

    dir_entry = readdir(dir);
    if (!dir_entry) {
        fprintf(stderr, "%s: readdir:%s\n", prog_name + 2, strerror(errno));
        exit(errno);
    }
    while (dir_entry) {
        if ((strncmp(dir_entry->d_name, ".", PATH_MAX) == 0) || (strncmp(dir_entry->d_name, "..", PATH_MAX) == 0)) {
            dir_entry = readdir(dir);
            continue;
        }

        strncpy(fullpath, path, PATH_MAX);

        if (fullpath[strlen(fullpath) - 1] != '/')
            strncat(fullpath, "/", PATH_MAX);
        strncat(fullpath, dir_entry->d_name, PATH_MAX);
     //   printf("%s\n",dir_entry->d_name);

        if (lstat(fullpath, &pstatus) == 0) {
            if (S_ISREG(pstatus.st_mode) && pstatus.st_size > min && pstatus.st_size < max) {
                fprintf(out, "%s %ld\n", fullpath, pstatus.st_size);
              //  printf("%lul\n",pstatus.st_dev);
                counter++;
            }

            if (S_ISDIR(pstatus.st_mode)) {
                DIR *nextdir = _opendir(fullpath);
                if (nextdir != NULL)
                    showdir(nextdir, fullpath);
            }
        }
        dir_entry = readdir(dir);
    }
    closedir(dir);
}

int main(int argc, char *argv[], char *envp[]) {
    if (argc != 5) {
        fprintf(stderr, "%s: expected 4 arguments\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    prog_name = argv[0];
    char* dirName = malloc(strlen(argv[1]));
    strcpy(dirName, argv[1]);
    min = atoi(argv[2]);
    max = atoi(argv[3]);
    char* destFile = malloc(strlen(argv[4]));
    strcpy(destFile, argv[4]);

    if (min < 0 || max < 0) {
        fprintf(stderr, "%s: size cannot be negative\n", prog_name+2);
        exit(EXIT_FAILURE);
    }


    out = fopen(destFile, "w");

    DIR* dir = _opendir(dirName);
    if (dir == NULL) {
        exit(errno);
    }

    showdir(dir, dirName);

    printf("scanned %ld files\n", counter);

    free(dirName);
    free(destFile);

    return 0;
}
