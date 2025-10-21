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

int fd[2]; // Declare fd as a global variable
// fd[0] = read end (output)
// fd[1] = write end (input)

// Flag to know if signal arrived (reading from pipe)
int from_pipe = 0;

// Redirect stdin to read end of pipe
void mysignalhandler(int s) {
    from_pipe = 1;
    dup2(fd[0], STDIN_FILENO);                      
}

int main() 
{
    pipe(fd);
    int parentPID = getpid();

    bool *activity = (bool*)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);  

    // Shared Buffer 
    char *text = (char*)malloc(100);
    pid_t pid = fork();

    if (pid == 0) {
        // In child process: wait for 10 seconds, if no activity from parent, then overwrite stdin and print message. 
        // Restore stdin afterwards to let parent read normally from keyboard again
        close(fd[0]);          
        for (;;)
        {
            sleep(5);
            // If parent has written something, activity has incremented, skip this iteration of the for loop -> start 10 sec again
            if (*activity == true) {
                *activity = false;  // Reset for next check
                continue;
            }

            else {
                // Send signal to parent FIRST to redirect stdin
                kill(parentPID, SIGUSR1);
                
                // Small delay 
                usleep(100000);  
                
                // Then write to pipe 
                write(fd[1], "Inactivity-Detected!\n", strlen("Inactivity-Detected!\n"));
            }           
        }
    }
    else {         
        // In parent process: continuously read from stdin (terminal), write to pipe, and print what is read    
        close(fd[1]); 
        int save_stdin = dup(STDIN_FILENO);     // Save original stdin (THE TERMINAL)
        signal(SIGUSR1, mysignalhandler);       // Initialize signal handler -> read from pipe when signal received from child (kill it)

        for (;;){
            scanf("%s", text);
            
            // Check if this input came from the pipe 
            if (from_pipe) {
                printf("%s\n", text);  // Print the inactivity message 
                dup2(save_stdin, STDIN_FILENO); // Restore stdin to terminal
                from_pipe = 0;  // Reset flag
                continue;  // Loop back maksa it so next scanf will read from terminal again
            }
            
            // Normal user input print with exclamation marks
            printf("!%s!\n", text);

            // Flag for activity, tell child that parent is active
            *activity = true;

            // Entering quit will terminate processes
            if (strcmp(text, "quit") == 0)
            {
                kill(0, SIGTERM);               
                break;
            }
        }
        wait(0);                                
    }
    
    munmap(activity, 4096);
    return 0;
}
