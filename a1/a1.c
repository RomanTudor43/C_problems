#include <stdio.h>
#include <string.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>

#define BUFSIZE 2048

mode_t permFromString(const char *perm)
{
    mode_t mode = 0;
    if (perm[0] == 'r')
        mode |= S_IRUSR;
    if (perm[6] == 'r')
        mode |= S_IROTH;
    if (perm[3] == 'r')
        mode |= S_IRGRP;
    if (perm[1] == 'w')
        mode |= S_IWUSR;
    if (perm[4] == 'w')
        mode |= S_IWGRP;
    if (perm[7] == 'w')
        mode |= S_IWOTH;
    if (perm[2] == 'x')
        mode |= S_IXUSR;
    if (perm[5] == 'x')
        mode |= S_IXGRP;
    if (perm[8] == 'x')
        mode |= S_IXOTH;
    return mode;
}

int listDir(const char *dir_name, int recursive, const char *filtering_option, int *success)
{
    struct dirent *directoryEntry;
    struct stat entry_stat;
    DIR *dir;
    char path[1024];
    dir = opendir(dir_name);
    int size_filt = -1;
    if (filtering_option[0] >= '0' && filtering_option[0] <= '9')
    {
        size_filt = atoi(filtering_option);
    }
    if (dir == NULL)
    {
        printf("invalid directory path");
        return -1;
    }
    if (*success == 0)
    {
        printf("SUCCESS\n");
        *success = 1;
    }

    while ((directoryEntry = readdir(dir)) != 0)
    {
        snprintf(path, sizeof(path), "%s/%s", dir_name, directoryEntry->d_name);
        if (lstat(path, &entry_stat) == -1)
        {
            printf("Error getting file stats");
            closedir(dir);
            return -1;
        }
        if (S_ISDIR(entry_stat.st_mode))
        {

            if ((strcmp(directoryEntry->d_name, ".") == 0) || (strcmp(directoryEntry->d_name, "..") == 0))
            {
                continue;
            }
            if (recursive)
            {
                listDir(path, 1, filtering_option, success);
            }
        }
        if (S_ISREG(entry_stat.st_mode) && size_filt > 0)
        {
            if (entry_stat.st_size > size_filt)
            {
                continue;
            }
        }
        if (S_ISREG(entry_stat.st_mode) || S_ISDIR(entry_stat.st_mode))
        {
            if (strcmp(filtering_option, "~") == 0 || size_filt > 0)
            {
                printf("%s\n", path);
            }
            else
            {
                mode_t required_perm = permFromString(filtering_option);
                if ((entry_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) == required_perm)
                {
                    printf("%s\n", path);
                }
            }
        }
    }
    closedir(dir);
    return 0;
}
int SFCheck(char *path)
{
    char buf[BUFSIZE];
    int fileDescriptor;
    if ((fileDescriptor = open(path, O_RDONLY)) < 0)
    {
        printf("erorr open");
        close(fileDescriptor);
        return -1;
    }

    if (lseek(fileDescriptor, -3, SEEK_END) < 0)
    {
        printf("error seeking file\n");
        close(fileDescriptor);
        return -1;
    }

    if (read(fileDescriptor, buf, 3) != 3)
    {
        printf("ERROR\n");
        close(fileDescriptor);
        return -1;
    }
    unsigned short size = (unsigned char)buf[0] + 256 * (unsigned char)buf[1];
    char magic = buf[2];
    if (magic != 'a')
    {
        printf("ERROR\n");
        printf("wrong magic");
        close(fileDescriptor);
        return -1;
    }

    if (lseek(fileDescriptor, -size, SEEK_END) < 0)
    {
        printf("error seeking file\n");
        close(fileDescriptor);
        return -1;
    }
    if (read(fileDescriptor, buf, size) != size)
    {
        printf("error reading file\n");
        close(fileDescriptor);
        return -1;
    }
    unsigned int version = (unsigned char)buf[0]; // i used unsigend int !!
    if (version < 75 || version > 169)
    {
        printf("ERROR\n");
        printf("wrong version");
        close(fileDescriptor);
        return -1;
    }
    /// section nr buf[1] - add tests
    unsigned int noSections = (unsigned char)buf[1];
    // printf("no %u", noSections);
    if (noSections != 2 && (noSections < 5 || noSections > 11))
    {
        printf("ERROR\n");
        printf("wrong sect_nr");
        close(fileDescriptor);
        return -1;
    }

    int i;

    /// for the buff and check tge other cond - tyoe
    unsigned int headerSize = size;
    unsigned int sizeSect[noSections];
    unsigned int typeSect[noSections];
    char nameSect[noSections][20];
    headerSize -= 3;
    int j;
    for (i = 0; i < noSections; i++)
    {
        sizeSect[i] = ((unsigned char)buf[headerSize - 1] << 24) + ((unsigned char)buf[headerSize - 2] << 16) + ((unsigned char)buf[headerSize - 3] << 8) +
                      (unsigned char)buf[headerSize - 4];
        headerSize -= 9;
        typeSect[i] = (unsigned char)buf[headerSize];
        if (typeSect[i] != 77 && typeSect[i] != 71 && typeSect[i] != 74 && typeSect[i] != 10 && typeSect[i] != 45 && typeSect[i] != 44)
        {
            printf("ERROR\n");
            printf("wrong sect_types");
            close(fileDescriptor);
            return -1;
        }
        for (j = 17; j >= 0; j--)
        {
            nameSect[i][j] = buf[headerSize - 1];
            headerSize--;
        }
        nameSect[i][18] = '\0';
        // headerSize -= 18;///search for name
    }
    printf("SUCCESS\n"); //// to add name
    printf("version=%u\n", version);
    printf("nr_sections=%u\n", noSections);
    j = 1;
    for (i = noSections - 1; i >= 0; i--)
    {
        printf("section%d: %s %u %u\n", j, nameSect[i], typeSect[i], sizeSect[i]);
        j++;
    }
    close(fileDescriptor);
    return 0;
}
int extractLine(char *path, int section_nr, int line_nr)
{
    char buf[BUFSIZE]; //// ??? se presupune ca e format bun ? sau tre sa verific ??
    int fd;
    if ((fd = open(path, O_RDONLY)) < 0)
    {
        printf("ERROR\n");
        printf("invalid file");
        return -1;
    }
    if (lseek(fd, -3, SEEK_END) < 0)
    {
        printf("ERROR\n");
        close(fd);
        return -1;
    }

    if (read(fd, buf, 3) != 3)
    {
        printf("ERROR\n");
        return -1;
    }
    unsigned short size = (unsigned char)buf[0] + 256 * (unsigned char)buf[1];
    char magic = buf[2];
    if (magic != 'a')
    {
        printf("ERROR\n");
        printf("invalid file");
        close(fd);
        return -1;
    } ///
    if (size < (27 * section_nr) + 5)
    {
        printf("ERROR\n");
        printf("section");
        close(fd);
        return -1;
    }

    if (lseek(fd, -size + (27 * (section_nr - 1)) + 2, SEEK_END) < 0)
    {
        printf("ERROR\n");
        close(fd);
        return -1;
    }
    if (read(fd, buf, 27) != 27)
    {
        printf("ERROR\n");
        close(fd);
        return -1;
    }
    unsigned int sizeSection = ((unsigned char)buf[26] << 24) + ((unsigned char)buf[25] << 16) + ((unsigned char)buf[24] << 8) +
                               (unsigned char)buf[23];
    unsigned int offset = ((unsigned char)buf[22] << 24) + ((unsigned char)buf[21] << 16) + ((unsigned char)buf[20] << 8) +
                          (unsigned char)buf[19];
    if (lseek(fd, offset, SEEK_SET) < 0) /// begin of section
    {
        printf("ERROR\n");
        close(fd);
        return -1;
    }
    /// alloc dynamical for the buf
    char *buffer = (char *)malloc(sizeSection * sizeof(char));
    if (read(fd, buffer, sizeSection) != sizeSection) /// read the section into buffer
    {
        printf("ERROR\n");
        close(fd);
        free(buffer);
        return -1;
    }
    int countLine = 1;
    int i;
    bool end = false;
    bool found = false;
    for (i = sizeSection - 1; i >= 0 && !end; i--)
    {
        if (line_nr == countLine)
        {
            printf("%c", buffer[i]);
        }

        if (buffer[i] == '\n')
        {
            countLine++;
            if (line_nr == countLine)
            {
                found = true;
                printf("SUCCESS\n");
            }
            if (countLine > line_nr)
            {
                end = 1;
            }
        }
    }
    if (!found)
    {
        printf("ERROR\n");
        printf("line");
        close(fd);
        free(buffer);
        return -1;
    }
    close(fd);
    free(buffer);
    return 0;
}
int SFCheck2(char *path)
{
    char buf[BUFSIZE];
    int fileDescriptor;
    if ((fileDescriptor = open(path, O_RDONLY)) < 0)
    {
        printf("ERROR\n");
        printf("invalid directory path");
        close(fileDescriptor);
        return -1;
    }

    if (lseek(fileDescriptor, -3, SEEK_END) < 0)
    {
        // printf("ERROR\n");
        // printf("invalid directory path");
        close(fileDescriptor);
        return -1;
    }

    if (read(fileDescriptor, buf, 3) != 3)
    {
        // printf("ERROR\n");
        close(fileDescriptor);
        return -1;
    }
    unsigned short size = (unsigned char)buf[0] + 256 * (unsigned char)buf[1];
    char magic = buf[2];
    if (magic != 'a')
    {
        //  printf("ERROR\n");
        // printf("invalid directory path");
        close(fileDescriptor);
        return -1;
    }

    if (lseek(fileDescriptor, -size, SEEK_END) < 0)
    {
        // printf("ERROR\n");
        //  printf("invalid directory path");
        close(fileDescriptor);
        return -1;
    }
    if (read(fileDescriptor, buf, size) != size)
    {
        //  printf("ERROR\n");
        //   printf("invalid directory path");
        close(fileDescriptor);
        return -1;
    }
    unsigned int version = (unsigned char)buf[0]; // i used unsigend int !!
    if (version < 75 || version > 169)
    {
        //  printf("ERROR\n");
        //  printf("invalid directory path");
        close(fileDescriptor);
        return -1;
    }
    /// section nr buf[1] - add tests
    unsigned int noSections = (unsigned char)buf[1];
    // printf("no %u", noSections);
    if (noSections != 2 && (noSections < 5 || noSections > 11))
    {
        // printf("ERROR\n");
        // printf("invalid directory path");
        close(fileDescriptor);
        return -1;
    }

    int i;

    /// for the buff and check tge other cond - tyoe
    unsigned int headerSize = size;
    unsigned int typeSect[noSections];
    headerSize -= 3;
    int countType = 0;
    for (i = 0; i < noSections; i++)
    {
        
        headerSize -= 9;
        typeSect[i] = (unsigned char)buf[headerSize];
        if (typeSect[i] != 77 && typeSect[i] != 71 && typeSect[i] != 74 && typeSect[i] != 10 && typeSect[i] != 45 && typeSect[i] != 44)
        {
            // printf("ERROR\n");
            // printf("wrong sect_types");
            close(fileDescriptor);
            return -1;
        }
        if (typeSect[i] == 77)
        {
            countType++;
        }
        headerSize -= 18;
    }
    if (countType == 0)
    {
        close(fileDescriptor);
        return -1;
    }

    close(fileDescriptor);
    return 0;
}

int findall(const char *path1, int *success)
{
    struct dirent *directoryEntry;
    struct stat entry_stat;
    DIR *dir;
    char path[1024];
    dir = opendir(path1);
    if (dir == NULL)
    {
        printf("invalid directory path");
        return -1;
    }
    if (*success == 0)
    {
        printf("SUCCESS\n");
        *success = 1;
    }

    while ((directoryEntry = readdir(dir)) != 0)
    {
        snprintf(path, sizeof(path), "%s/%s", path1, directoryEntry->d_name);
        if (lstat(path, &entry_stat) == -1)
        {
            printf("Error getting file stats");
            closedir(dir);
            return -1;
        }
        if (S_ISDIR(entry_stat.st_mode))
        {

            if ((strcmp(directoryEntry->d_name, ".") == 0) || (strcmp(directoryEntry->d_name, "..") == 0))
            {
                continue;
            }
            findall(path, success);
        }
        if (S_ISREG(entry_stat.st_mode) || S_ISDIR(entry_stat.st_mode))
        {
            if (SFCheck2(path) == 0)
            {
                printf("%s\n", path);
            }
        }
    }
    closedir(dir);
    return 0;
}

int main(int argc, char **argv)
{
    int recursive = 0;
    char *dir_path = NULL;
    char *filtering_option = "~";
    char *section = NULL;
    char *path = NULL;
    if (argc >= 2)
    {

        if (strcmp(argv[1], "variant") == 0)
        {
            printf("67730\n");
            return 0;
        }
        if (strcmp(argv[1], "list") == 0)
        {

            for (int i = 2; i < argc; i++)
            {
                if (strcmp(argv[i], "recursive") == 0)
                {
                    recursive = 1;
                }
                if (strncmp(argv[i], "path=", 5) == 0)
                {
                    dir_path = argv[i] + 5;
                }
                if (strncmp(argv[i], "size_smaller=", 13) == 0)
                {
                    filtering_option = argv[i] + 13;
                }
                else if (strncmp(argv[i], "permissions=", 12) == 0)
                {
                    filtering_option = argv[i] + 12;
                }
            }
            int succes = 0;
            listDir(dir_path, recursive, filtering_option, &succes);
        }

        ///
        if (strcmp(argv[1], "parse") == 0)
        {
            path = argv[2] + 5;
            // printf("path =%s", path);
            SFCheck(path);
        }

        ///
        if (strcmp(argv[1], "extract") == 0)
        {
            path = argv[2] + 5;
            section = argv[3] + 8;
            dir_path = argv[4] + 5;
            int sectionNr = atoi(section);
            int lineNr = atoi(dir_path);
            // printf( " path = %s, section = %d, dir_path = %d ", path, sectionNr,lineNr );
            extractLine(path, sectionNr, lineNr);
        }
        if (strcmp(argv[1], "findall") == 0)
        {
            path = argv[2] + 5;
            int success = 0;
            findall(path, &success);
        }
    }

    return 0;
}
