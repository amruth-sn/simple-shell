#include "myshell.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#define TRUE 1
#define MAX_INPUT 512
void parse_commands(char* input_start, char* tokens[512][512], int* num_tokens_ptr, int row_count){
        char* delim = " \t\n";
        char *token = strtok(input_start, delim);
        int r = 0; int c = 0;
        while(token != NULL){

                //printf("test0\n");
                if(strcmp(token, "<") == 0 || strcmp(token, ">") == 0 || strcmp(token, "|") == 0){
                        if(strcmp(token, "|") == 0){
                                tokens[r++][c] = NULL;
                                c=0;
                                //tokens[r][c] = token; --> wrong, just make NULL and newline
                        }
                        else if(strcmp(token, "<") == 0){
                                tokens[r++][c] = NULL;
                                c=0;
                                tokens[r][c++] = token;

                        }

                        else{ // ">"
                                tokens[r++][c] = NULL;
                                c=0;
                                tokens[r][c++] = token;

                        }
                }
                else{
                        tokens[r][c++] = token;

                }

                (*num_tokens_ptr)++;
                token = strtok(NULL, delim);
        }

        //tokens[row_count][0] = NULL; //redundancy check: make the row_count 'th row's first token NULL
}
void collector(int signal_number){
        int status;
        pid_t child;
        while((child = waitpid(-1, &status, WNOHANG)) > 0) {
                if(WIFEXITED(status)){
                        printf("Process %d has exited with status %d.\n", child, WEXITSTATUS(status));
                }
                else if(WIFSIGNALED(status)){
                        printf("Process %d has terminated due to signal %d.\n", child, WTERMSIG(status));
                }
        }
}
int main(int argc, char* argv[]) {
        struct sigaction sa = {
        .sa_handler = collector,
        .sa_flags = SA_RESTART | SA_NOCLDSTOP
        };
        sigemptyset(&sa.sa_mask);
        sigaction(SIGCHLD, &sa, NULL);
        while(TRUE) {
                if(!(argc > 1 && strcmp(argv[1], "-n") == 0)){
                        printf("my_shell$ ");
                }

                char user_input[MAX_INPUT];
                if(fgets(user_input, sizeof(user_input)/sizeof(user_input[0]), stdin) == NULL){
                        printf("\n");
                        break;
                }

                //int background = 0;
                int n = strlen(user_input);
                user_input[n-1] = '\0';
                int num_tokens = 0;
                char* tokens[512][512]; //2D string array of tokens to be processed/parsed/executed

                //PRE-PARSE
                int i;
                int count = 0;
                int pipeCount =1; //not ACTUALLY pipeCount, rather the "command count" of commands to be executed
                int rows = 1;
                for(i = 0; i < n; i++){
                        if(user_input[i] == '|' || user_input[i] == '<' || user_input[i] == '>'){
                                count+= 2;
                                rows++;
                        }

                        if(user_input[i] == '|'){
                                pipeCount++;
                        }

                }
                //printf("%d\n", pipeCount)
                char new_input[1024];
                int j = 0;
                for(i = 0; i < n + count; i++){
                        if(user_input[i] == '>' || user_input[i] == '<' || user_input[i] == '|'){
                                new_input[j++] = ' ';
                                new_input[j++] = user_input[i];
                                new_input[j++] = ' ';
                        }
                        else{
                                new_input[j++] = user_input[i];
                        }
                }
                new_input[j] = '\0';
                //printf("%s\n", new_input);
                parse_commands(new_input, tokens, &num_tokens, rows);
                int background = 0;
                for(i = 0; tokens[rows-1][i] != NULL; i++){
                        if(strcmp(tokens[rows-1][i], "&") == 0){
                                background = 1;
                                tokens[rows-1][i] = NULL;
                        }
                }

                int** fd = malloc(sizeof(int*) * (pipeCount - 1));
                for(i = 0; i < pipeCount - 1; i++){
                        fd[i] = malloc(sizeof(int) * 2);        //2D file descriptor array
                        pipe(fd[i]); //Create pipeline along the first index
                }
                int status;
                pid_t processID;
                //int command = 0;

                if(rows == 1){
                        fflush(stdout);
                        processID = fork();
                        if(processID == 0){
                                execvp(tokens[0][0], tokens[0]);
                                printf("Something went wrong with your %s, please try again: %s\n", tokens[0][0], strerror(errno));
                                exit(errno);
                        }
                        else if(processID > 0){
                                if(!background) wait(&status);
                        }
                        else{
                                perror("Fork failed!\n");
                                exit(EXIT_FAILURE);
                        }
                }
                else{
                        int output_fd; int input_fd;
                        char* input_file = NULL;
                        char* output_file = NULL;
                        if(strcmp(tokens[1][0], "<") == 0){
                                input_file = tokens[1][1];
                                //printf("%s\n", input_file);
                        }

                        if(strcmp(tokens[rows-1][0], ">") == 0){
                                output_file = tokens[rows-1][1];
                        }

                        for(i = 0; i < pipeCount; i++){ //Children here will spawn and terminate. None shall make it past
                                 //from i = 0 to pipecount-1
                                fflush(stdout);
                        //if(strcmp(tokens[i][0], "<") != 0 && strcmp(tokens[i][0], ">") != 0){
                                processID = fork();
                                if(processID == 0){

                                        if(i != pipeCount-1){ //all but last pipe
                                                dup2(fd[i][1], 1); //redirect STDOUT to write end
                                                close(fd[i][0]);
                                                close(fd[i][1]); //close unused
                                        }

                                        if(i != 0){ //all but first pipe
                                                dup2(fd[i-1][0], 0); //redirect STDIN to read end
                                                close(fd[i-1][0]);
                                                close(fd[i-1][1]); //close unused
                                        }

                         //redirect both STDIN and STDOUT to read and write ends, respectively
                                        //change structure of conditionals.
                                        //middle pipes should still work as intended (?)
                                        if(i == 0 && input_file != NULL){
                                                input_fd = open(input_file, O_RDONLY);
                                                dup2(input_fd, 0);
                                                close(input_fd);
                                        }

                                        if(i == pipeCount-1 && output_file != NULL){
                                                output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666); //Read and write from/to file, create if not already created, and give owner all perms, group read/write perms, and other read/write perms
                                                fflush(stdout);
                                                dup2(output_fd, 1);
                                                close(output_fd);
                                        }

                                        j = 1;
                                        while(j < pipeCount){
                                                if(j != i+1 && j != i){
                                                        close(fd[j-1][0]);
                                                        close(fd[j-1][1]);
                                                }
                                 //close all pipes except for current and last pipe for each iteration
                                                j++;
                                        }

                                        if(strcmp(tokens[i][0], "<") && strcmp(tokens[i][0], ">")){
                                                execvp(tokens[i][0], tokens[i]);
                                                printf("Something went wrong with your %s, please try again: %s\n", tokens[i][0], strerror(errno));
                                                exit(errno);
                                        }
                                }
                        }
                        if(processID > 0){
                                j = 1;
                                while(j < pipeCount){
                                        close(fd[j-1][0]);
                                        close(fd[j-1][1]);
                                        j++;
                                }
                        //close all pipes iteratively, just catch children VVV
                                if(!background){ // &&&&&
                                        for(j = 0; j < pipeCount; j++){
                                                wait(&status);
                                        }
                                }

                                //Now free malloc'ed stuff
                                for(i = 1; i < pipeCount; i++){
                                        free(fd[i-1]);
                                }
                                free(fd);
                        }



                }       //CLEAR THE BUFFER - Courtesy of Chonghua
                i=0; j=0;
                for(i = 0; tokens[i][j] != NULL; i++){
                        for(j = 0; tokens[i][j] != NULL; j++){
                                tokens[i][j] = NULL;
                        }
                }
