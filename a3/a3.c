#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

char *readString(int fd, char *buf)
{

    char *c = (char *)malloc(sizeof(char));
    int i = 0;
    while (*c != '!')
    {
        read(fd, c, 1);
        buf[i] = *c;
        i++;
    }
    buf[i] = '\0';
    free(c);
    return buf;
}
unsigned int *readNumber(int fd)
{
    unsigned int *number = (unsigned int *)malloc(sizeof(unsigned int));
    int bytesRead = read(fd, number, sizeof(unsigned int));

    if (bytesRead < 0)
    {
        perror("Error reading from file");
        free(number);
        return NULL;
    }
    return number;
}

int main()
{
    int fd1, fd2;
    int sizeFile;
    unsigned int *number;
    char *string_shared;
    char *file_sh;
    char *buff1;
    unsigned int *offset;
    unsigned int *nrBytes;
    unsigned int *section_no;
    unsigned int *offset1;
    unsigned int *nrBytes1;
    unsigned int *logicalOffset;
    unsigned int *nrBytes2;
    // unlink("RESP_PIPE_67730");
    if (mkfifo("RESP_PIPE_67730", 0644) < 0)
    {
        printf("ERROR\n");
        printf("cannot create the response pipe\n");
    }
    char *buf = (char *)malloc(250 * sizeof(char));

    if ((fd1 = open("REQ_PIPE_67730", O_RDONLY)) < 0)
    {
        printf("ERROR1");
        exit(1);
    }
    if ((fd2 = open("RESP_PIPE_67730", O_WRONLY)) < 0)
    {
        printf("ERROR");
        printf("cannot open the request pipe");
        exit(1);
    }

    // f1 read
    // f2 write
    if (write(fd2, "CONNECT!", 8) < 0)
    {
        printf("ERROR3");
        exit(1);
    }
    printf("SUCCESS");

    while (1)
    {
        buf = readString(fd1, buf);
        //  printf("buf = %s\n", buf);
        /// 1 ping ponmg

        if (strncmp(buf, "PING", 4) == 0)
        {
            if (write(fd2, "PING!", 5) < 0)
            {
                exit(1);
            }

            unsigned int response = 67730;
            if (write(fd2, &response, sizeof(unsigned int)) < 0)
            {
                exit(2);
            }

            if (write(fd2, "PONG!", 5) < 0)
            {
                exit(1);
            }
            buf = readString(fd1, buf);
            // printf("buf = %s\n", buf);
        }

        /// 2.4 create shm

        if (strncmp(buf, "CREATE_SHM", 10) == 0)
        {
            number = readNumber(fd1);
            //  printf("number = %u\n", *number);
            write(fd2, "CREATE_SHM!", 11);

            int shm_fd = shm_open("/l2YWZ6tF", O_CREAT | O_RDWR, 0664);

            if (shm_fd < 0)
            {
                write(fd2, "ERROR!", 6);
                perror("shm_fd error");
                exit(1);
            }

            if (ftruncate(shm_fd, *number) == -1)
            {
                perror("nu trunct");
                write(fd2, "ERROR!", 6);
                close(shm_fd);
                exit(2);
            }

            string_shared = (char *)mmap(0, *number, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            close(shm_fd);
            if (string_shared == MAP_FAILED)
            {
                write(fd2, "ERROR!", 6);

                exit(4);
            }
            else
            {
                write(fd2, "SUCCESS!", 8);
            }
        }

        /// 2.5 write_ shm

        if (strncmp(buf, "WRITE_TO_SHM", 12) == 0)
        {

            write(fd2, "WRITE_TO_SHM!", 13);
            unsigned int *offset = readNumber(fd1);
            unsigned int *value = readNumber(fd1);
            printf("offset = %u, value = %u\n", *offset, *value);
            if (0 < *offset && *offset + 4 < *number)
            {
                *((unsigned int *)(string_shared + *offset)) = *value;
                write(fd2, "SUCCESS!", 8);
            }
            else
            {
                write(fd2, "ERROR!", 6);
            }
        }
        /// 2.6 MAP FILE

        if (strncmp(buf, "MAP_FILE!", 9) == 0)
        {
            buf = readString(fd1, buf);
            buf[strlen(buf) - 1] = '\0';

            write(fd2, "MAP_FILE!", 9);
            int fd_fileMap;
            if ((fd_fileMap = open(buf, O_RDONLY)) < 0)
            {
                write(fd2, "ERROR!", 6);
            }
            lseek(fd_fileMap, 0, SEEK_SET);
            sizeFile = lseek(fd_fileMap, 0, SEEK_END);
            lseek(fd_fileMap, 0, SEEK_SET);
            //  printf("fd = %d SIZE = %d \n", fd_fileMap, sizeFile);

            file_sh = (char *)mmap(0, sizeFile, PROT_READ, MAP_SHARED, fd_fileMap, 0);
            if (file_sh == MAP_FAILED)
            {
                write(fd2, "ERROR!", 6);
                exit(4);
            }
            else
            {
                write(fd2, "SUCCESS!", 8);
            }
        }

        if (strncmp(buf, "READ_FROM_FILE_OFFSET!", 22) == 0)
        {
            offset = readNumber(fd1);
            nrBytes = readNumber(fd1);
            write(fd2, "READ_FROM_FILE_OFFSET!", 22);
            // printf("offset = %u, nrBytes = %u\n", *offset, *nrBytes);
            buff1 = (char *)malloc(*nrBytes);
            int j = 0;
            int flagNull = 0;
            for (unsigned int i = *offset; i < (*offset + *nrBytes) && (flagNull == 0); i++)
            {
                if (file_sh[i] == '\0')
                {
                    flagNull = 1;
                }
                buff1[j] = file_sh[i];
                j++;
            }
            if (flagNull == 1)
            {
                write(fd2, "ERROR!", 6);
            }
            else
            {

                for (int k = 0; k < j; k++)
                {
                    string_shared[k] = buff1[k];
                }
                write(fd2, "SUCCESS!", 8);
            }
            free(offset);
            free(nrBytes);
        }
        /// 2.8 Read From File Section Request

        if (strncmp(buf, "READ_FROM_FILE_SECTION", 22) == 0)
        {

            section_no = readNumber(fd1);
            offset1 = readNumber(fd1);
            nrBytes1 = readNumber(fd1);
            printf("offset1 =  %u  , nrBytes1 =  %u\n", *offset1, *nrBytes1);
            unsigned short headerSize = (unsigned char)file_sh[sizeFile - 2] * 256 + (unsigned char)file_sh[sizeFile - 3];
            unsigned int nrSectionHeader = (unsigned int)sizeFile - (unsigned int)headerSize + 1;
            printf(" asdd  = %u , section  = %u \n", file_sh[nrSectionHeader], *section_no);
            if (*section_no < 1 || *section_no > (unsigned int)file_sh[nrSectionHeader])
            {
                write(fd2, "READ_FROM_FILE_SECTION!", 23);
                write(fd2, "ERROR!", 6);
            }
            else
            {
                unsigned int sectionOff = sizeFile;
                sectionOff -= 3;
                printf(" size 2 %hhX\n", file_sh[sectionOff]);
                sectionOff = sectionOff - (( (unsigned int)file_sh[nrSectionHeader] - *section_no ) * 27) - 5;
                printf("offHexa %hhx %hhx %hhx %hhx \n", file_sh[sectionOff], file_sh[sectionOff - 1], file_sh[sectionOff - 2], file_sh[sectionOff - 3]);

                unsigned int sectionOff_nr = ((unsigned char)file_sh[sectionOff] << 24) +
                                             ((unsigned char)file_sh[sectionOff - 1] << 16) +
                                             ((unsigned char)file_sh[sectionOff - 2] << 8) +
                                             (unsigned char)file_sh[sectionOff - 3];

                printf("section offset %u \n", sectionOff_nr);

                for (int i = 0; i < *nrBytes1; i++)
                {
                    string_shared[i] = file_sh[sectionOff_nr + *offset1 + i];
                    printf("%c", file_sh[sectionOff_nr + *offset1 + i]);
                }

                printf("\n");
                for (int i = 0; i < *nrBytes1; i++)
                {
                    printf("%c", string_shared[i]);
                }
                printf("\n");
                write(fd2, "READ_FROM_FILE_SECTION!", 23);
                write(fd2, "SUCCESS!", 8);
                free(section_no);
                free(offset1);
                free(nrBytes1);
            }
        }
        /// 2.9
        if (strncmp(buf, "READ_FROM_LOGICAL_SPACE_OFFSET", 30) == 0)
        {
            logicalOffset = readNumber(fd1);
            nrBytes2 = readNumber(fd1);
            *logicalOffset = *nrBytes2;
            *nrBytes2 = *logicalOffset;
            write(fd2, "READ_FROM_LOGICAL_SPACE_OFFSET!", 31);
            write(fd2, "SUCCESS!", 8);
            free(nrBytes2);
            free(logicalOffset);
        }

        /// EXIT
        if (strncmp(buf, "EXIT", 4) == 0)
        {
            if (string_shared != MAP_FAILED)
            {
                munmap(string_shared, *number);
                printf("unmapped");
            }
            if (file_sh != MAP_FAILED)
            {
                munmap(file_sh, sizeFile);
                printf("unmapped");
            }
            free(number);
            close(fd1);
            close(fd2);
            unlink("RESP_PIPE_67730");

            break;
        }
    }

    return 0;
}