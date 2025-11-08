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
#include <time.h>
#include <unistd.h>
#include <chrono>
#include <sys/wait.h>
#include <signal.h>
using namespace std;

// // Redirection!
// //Have a command line program that reads from a text file and writes to wherever you direct it
// to/:
// ./redirect test.txt > terminal //prints the text from test.txt to the terminal
// ./redirect test.txt > file.txt //prints the text from test.txt to a new file file.txt
// Use redirection!
// Read from a file with fread.
// Make a pipe.
// Redirect the output of the pipe either to stdout or the new file.
// Write to the entry of this pipe whatever you read from the file with fread.
// Exercise 2 (BONUS +50%):
// Fork into a parent and a child.
// Do the reading of the file of exercise 1 in a child process and use the pipe to communicate
// over pipe 1 with the parent. Parent is using another pipe as in exercise 1.

int fd[2];

int main(int argc, char *argv[]){
    pipe(fd);
    const char *input = argv[1];

    char text[1000];

    int parent = getpid();

    if (fork() ==0) {
        FILE *file = fopen(input, "r");
        int bytes = fread(text, 1, sizeof(text), file);
        text[bytes] = 0;

        //cout << bytes << endl;
        write(fd[1], text, 23);
        
        fclose(file);
        close(fd[1]);
        close(fd[0]);
        return 0;
    }
    else {
        char text2[1000];

        int by2 = read(fd[0], text2, 1000);
        text2[by2] = 0;
        printf("%s", text2);
       
        close(fd[1]);
        close(fd[0]);
    }
    return 0;
}