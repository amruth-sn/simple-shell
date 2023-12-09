****Simple Custom Shell****

This project was implemented by Amruth Niranjan at Boston University

Tools used:
TutorialsPoint (C library functions and explanations with examples)
YouTube (Extended guides on forking, exec, wait, pipe, signal handlers and errno)
Linux Man-Pages

Notes:
- I passed in arguments to main (argc and argv), and depending on whether a user executed ./myshell with the argument "-n" or not, the program prints out "my_shell$". This was for the purpose of automated testing.

- I used fgets() to read user input from the standard input. The parameters I passed in were 1) the place where user input would be stored, 2), the size of the mentioned user input storage, and 3) the file that I read from (standard input).

- I checked if fgets() returned a value of NULL as the detector clause for whenever the user wants to exit the shell. According to man, fgets() returns the string s on success, and NULL on error or when E.O.F. occurs while no characters have been read.  According to sources on the Internet (and my own testing) the corresponding E.O.F. indicator for stdin happens to be CTRL-D. Thus, the shell exits whenever the user prompts CTRL-D, while continuing to keep the main shell open (if they run the program from the command line).

- At this point, the program begins to parse the command input by the user, whether valid or not. I used the strtok() function to tokenize the input character array as a series of words, delimited by spaces. First, though, I pre-parsed the command to add spaces before and after all pipe characters (|) and redirect characters (< and >). From this point, a simple while loop checks if the current token has reached the end of the input array.

- Each token is added to a 2-dimensional array of strings (3-dimensional array of characters). If a pipe character is encountered, the next token is appended to the next line. If a redirect character is encountered, that character is appended to the next line, with the corresponding subsequent token (a file name) appended to the token immediately after. Each line ended with a NULL and the last row was a row that began with NULL.

- I check for background processes. If an ampersand (&) exists, then a variable background is switched to true.

- If the tokens array only contains one row, this logically means there is only one command to execute. I copied my structure from the mini-deadline to thus execute the first command using execvp, feeding in the entire row for the aarguments, and then checking for error.

- Otherwise, if more than one row in the tokens array exists, I first check if input or output is being reidrected, and if either or both of these are true, the names of these files are stored on the stack. Input and output file descriptors are also stored. Now, we begin to loop through the array, because the existence of more than one row implies that something is being redirected somewhere.

- A two-dimensional file descriptor array is dynamically allocated for every pipe. The number of pipes is equal to the number of separate commands to be executed minus one. I instantialized pipeCount to represent each "command" to be executed (a redirect counts as one command). The file descriptor array has two columns (for read-end and write-end of the pipes).

- For each row in the tokens array, a fork is created. For the first pipe, STDOUT is reassociated to the write-end using dup2. The read-end is not being used, so it is closed. Afterward, the old write-end is closed, too. For the last pipe, STDIN is reassociated to the read-end of the corresponding row of the file descriptor array, and the unused previous read and write ends are closed after use. Thus, for every other pipe, a combination of both of these occurs. STDOUT's file descriptor is associated with the duplicated write-end of the corresponding row of the file descriptor array, and STDIN's file descriptor is associated with the duplicated read-end of the corresponding row of the file descriptor array. Unused ends are closed for both.

- If files are to be redirected, no explicit piping occurs, rather the file descriptor corresponding with the file to read from or write to replaces the file descriptor corresponding with the read-end or write-end (respectively) of the corresponding row of the file descriptor array. All of this occurs in the child process. Thus, for each child, ALL OTHER read and write ends are closed.

- Lastly, in the parent process, all pipe read and write ends are closed. After, dynamically allocated file descriptor arrays are freed, and the tokens array, along with the buffers for both user_input and the pre-parsed new_input, are cleaned. If the aforementioned background variable is not true, the parent process waits for each child process to finish. If it is not, the parent process does not wait! And so, the loop begins again.

- Child processes whose exit statuses are not collected due to the presence of the background variable are terminated by the signal handler instantiated at the beginning of the main() function. It uses a infinitely repeating function pointer that constantly checks for zombie processes and terminates them itself.

Please let me know if there are any bugs!

Thank you,

Amruth Niranjan
