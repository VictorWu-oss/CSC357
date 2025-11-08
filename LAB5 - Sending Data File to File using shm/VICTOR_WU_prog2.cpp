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

int main(){
    // 2nd program links to shared memory w/o O_CREAT
    int fd_link = shm_open("/myshm", O_RDWR, 0777);
    if(fd_link == -1)
    {
        perror("Failed Linking to shared memory");
        exit(0);
    }

    char *sh_mem = (char*)mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd_link, 0);
    if (sh_mem == MAP_FAILED)
    {
        perror("Error! Mapping Failed during linking");
        exit(0);
    }

    // Regularly check shared memory and wait for data to be available.
    // Once buffer not empty, read and print the sentence and end prog 2

    while(sh_mem[0] == '\0')
    {

    }

    printf("Received: %s\n", sh_mem);

    sh_mem[0] = '\0'; // Clear so future checks see empty buffer
    munmap(sh_mem, 1024);
    close(fd_link);
    return 0;
}   
