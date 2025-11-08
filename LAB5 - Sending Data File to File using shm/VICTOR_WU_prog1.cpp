#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <cctype>
#include <algorithm>
#include <cmath>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <chrono>
#include <sys/wait.h>
#include <signal.h>
using namespace std;

typedef unsigned char BYTE;

size_t SIZE = 1024;

int main(){
    // One program creates a shared memory character array
    // Other programs link to this
    // Expect fd # of 3.
    int fd = shm_open("/myshm", O_CREAT | O_RDWR, 0777);
    if(fd == -1)
    {
        perror("Failed creating shared memory");
        exit(0);
    }

    // Overrides original size of files in bytes, Sets and assigns size of shared mem object (fd). 
    ftruncate(fd, SIZE);    

    // Map shared memory to current process address space, SHARED makes sure changes are visible to other processes mapping the same object.
    // Treat as a byte array or struct
    char *sh_mem = (char*) mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sh_mem == MAP_FAILED)
    {
        perror("Mapping Failed during creation");
        exit(0);
    }

    // Waits for user input at read line
    // Read whole line (stdin) w/ spaces of max 1024 bytes into a local buffer sh_mem
    std::string text;
    getline(cin, text);

    // End program if user types quit
    if (text == "quit") 
    {
        printf("Quit Program\n");
        munmap(sh_mem, SIZE);
        close(fd);
        shm_unlink("/myshm");
        exit(0);
    }

    else{
        size_t len = min(text.size(), SIZE -1); // Len is actual size of bytes, min picks smaller value, SIZE -1 leave 1 byte for null terminator
        memcpy(sh_mem, text.data(), len);       // Copies size(len) bytes from buffer(text) to shared memory (sh_mem). Data() gets pointer to underlying array
        sh_mem[len] = '\0';                     // Null terminate the string in buffer
    }

    munmap(sh_mem, SIZE);
    close(fd);
    shm_unlink("/myshm");
    return 0;
}