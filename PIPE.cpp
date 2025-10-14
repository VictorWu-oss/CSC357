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
using namespace std;

int fd[2]; // Declare fd as a global variable
// fd[0] = read end (output)
// fd[1] = write end (input)

void mysignalhandler(int s) {
    dup2(fd[0], STDIN_FILENO);                      // Redirect stdin to read end of pipe
    printf("Signal received: Overwrite STDIN\n");
}

int main() {
    signal(SIGUSR1, mysignalhandler);           
    pipe (fd);

    int parentPID = getpid();
    char text[100];

    if (fork() == 0){
        sleep(2);                               // Let parent set up signal handler
        close(fd[0]);                           // Close unused read end
        kill(parentPID, SIGUSR1);               // Send signal to parent
        write(fd[1], "Hello from child", 17);   // Write to pipe
        close(fd[1]);                           // Reader will see EOF
        return 0;
    }
    else {                                      
        close(fd[1]);                           // Close unused write end
        int save_stdin = dup(STDIN_FILENO);     // Save original stdin
        signal(SIGUSR1, mysignalhandler);       // Re-register signal handler
        read(STDIN_FILENO, text, 100);          // This will be interrupted by signal
        printf("Read from stdin: %s\n", text);      

        printf("Restore STDIN\n");              // Now restore stdin
        dup2(save_stdin, STDIN_FILENO);         // Restore original stdin
        text[0] = 0;                            // Clear buffer
        read(STDIN_FILENO, text, 100);          // Read from original stdin
        printf("Read from stdin: %s\n", text);  
        wait(0);                                // Wait for child to finish      
    }
    close(fd[0]);
    close(fd[1]);

    return 0;
}
