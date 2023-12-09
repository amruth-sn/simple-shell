#ifndef MY_SHELL_H
#define MY_SHELL_H
void parse_commands(char* input_start, char* tokens[512][512], int* num_tokens_ptr, int row_count);
void collector(int signal_number);
#endif
