#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

char *prog_name;
FILE *out;

DIR *_opendir(const char *__name)
{
    DIR *dir;
    dir = opendir(__name);
    if (dir == NULL)
    {
        fprintf(stderr, "%s: opendir(%s): %s\n", prog_name, __name, strerror(errno));
    }
    return dir;
}

void processdir(DIR *dir, const char *path)
{
    char fullpath[PATH_MAX + 1];
    char max_file_name[PATH_MAX] = "-";
    struct stat pstatus;
    struct dirent *dir_entry;
    long max_size = 0;
    long total_size = 0;
    long count = 0;

    do
    {
        dir_entry = readdir(dir);
        if (!dir_entry && errno)
        {
            fprintf(stderr, "%s: readdir:%s\n", prog_name, strerror(errno));
            break;
        } else if (!dir_entry)
        {
            break;
        }

        if ((strncmp(dir_entry->d_name, ".", PATH_MAX) == 0) || (strncmp(dir_entry->d_name, "..", PATH_MAX) == 0))
        {
            continue;
        }

        strncpy(fullpath, path, PATH_MAX);

        if (fullpath[strlen(fullpath) - 1] != '/')
            strncat(fullpath, "/", PATH_MAX);
        strncat(fullpath, dir_entry->d_name, PATH_MAX);

        if (lstat(fullpath, &pstatus) == 0)
        {
            if (S_ISREG(pstatus.st_mode))
            {
                total_size+=pstatus.st_size;
                count++;
                if(pstatus.st_size>max_size) strncpy(max_file_name, dir_entry->d_name, PATH_MAX);
            }else if (S_ISDIR(pstatus.st_mode))
            {
                DIR *dir2 = _opendir(fullpath);
                if (dir2 == NULL) exit(errno);
                processdir(dir2, fullpath);
            }
        }
        else
        {
            fprintf(stderr, "%s: lstat:%s\n", prog_name, strerror(errno));
            exit(errno);
        }
    } while (dir_entry);
    
    if(closedir(dir))
        fprintf(stderr, "%s: closedir:%s\n", prog_name, strerror(errno));

    printf("%s %ld %ld %s\n", path, count, total_size, max_file_name);
    fprintf(out,"%s %ld %ld %s\n", path, count, total_size, max_file_name);
}

int main(int argc, char *argv[], char *envp[])
{
    if (argc != 3)
    {
        fprintf(stderr, "%s: expected 2 arguments\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    prog_name = basename(argv[0]);
    char *dirName = malloc(strlen(argv[1]));
    strcpy(dirName, argv[1]);
    char *destFile = malloc(strlen(argv[2]));
    strcpy(destFile, argv[2]);

    out = fopen(destFile, "w");

    DIR* dir = _opendir(dirName);
    if (dir == NULL) exit(errno);
    processdir(dir, dirName);

    fclose(out);

    free(dirName);
    free(destFile);

    return 0;
}
