/****************
LE2: Introduction to Unnamed Pipes
****************/
#include <unistd.h> // pipe, fork, dup2, execvp, close
#include <iostream>
#include <sys/wait.h>
#include <cstdlib>
using namespace std;

int main () {
    // lists all the files in the root directory in the long format
    char* cmd1[] = {(char*) "ls", (char*) "-al", (char*) "/", nullptr};
    // translates all input from lowercase to uppercase
    char* cmd2[] = {(char*) "tr", (char*) "a-z", (char*) "A-Z", nullptr};

    // TODO: add functionality
    // Create pipe
    int fd[2];
    if (pipe(fd) == -1){
      perror("pipe failed");
      return 1;
    }
    int saved_stdin = dup(STDIN_FILENO);
    int saved_stdout = dup(STDOUT_FILENO);
    if (saved_stdin == -1 || saved_stdout == -1) {
      perror("dup failed");
      exit(1);
    }
    // Create child to run first command
    // In child, redirect output to write end of pipe
    // Close the read end of the pipe on the child side.
    // In child, execute the command
    pid_t child1 = fork();
    if (child1 == 0) {
      dup2(fd[1], STDOUT_FILENO);
      close(fd[0]);
      close(fd[1]);
      char* args[] = {const_cast<char*>("ls"), const_cast<char*>("-al"), const_cast<char*>("/"), NULL};
      execvp("ls", args);
      perror("execvp failed (ls)");
      exit(1);
    }
    // Create another child to run second command
    // In child, redirect input to the read end of the pipe
    // Close the write end of the pipe on the child side.
    // Execute the second command.
    pid_t child2 = fork();
    if (child2 == 0) {
      dup2(fd[0], STDIN_FILENO);
      close(fd[0]);
      close(fd[1]);
      char* args[] = {const_cast<char*>("tr"), const_cast<char*>("[:lower:]"), const_cast<char*>("[:upper:]"), NULL};
      execvp("tr", args);
      perror("execvp failed (tr)");
      exit(1);
    }
    // Reset the input and output file descriptors of the parent.
    close(fd[0]);
    close(fd[1]);
    int status;
    waitpid(child1,&status,0);
    waitpid(child2,&status,0);
    close(saved_stdin);
    close(saved_stdout);
    return 0;
}
