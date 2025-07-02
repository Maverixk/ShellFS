#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"

#define MAX_LINE 1024

// Used to make sure user can't open or format a FS while another FS is already open 
int fs_open = 0;
char filename[FILENAME_LEN] = "";

// Print help menù
void print_help() {
    printf("Available commands:\n");
    printf("\t- format <file_system> <size>\n");
    printf("\t- open   <file_system>\n");
    printf("\t- mkdir  <dir>\n");
    printf("\t- cd     <dir | / | .. | .>\n");
    printf("\t- touch  <file>\n");
    printf("\t- cat    <file>\n");
    printf("\t- ls     <dir>\n");
    printf("\t- append <file> <text>\n");
    printf("\t- rm     <dir/file>\n");
    printf("\t- close\n");
    printf("\t- clear\n");
    printf("\t- help\n");
    printf("\t- quit\n");
}

// Check if we got the right number of token for a specific function
int check_arity(const char* cmd, int got, int expected) {
    if (got != expected) {
        printf("%s: wrong number of arguments (expected %d, actual %d)\n",
               cmd, expected - 1, got - 1);
        return -1;
    }
    return 0;
}

// Shell loop
int main(void) {
    char line[MAX_LINE];

    printf("Mini‑shell FAT – type 'help' to list commands, 'quit' to shutdown.\n");

    while (1) {
        if(!fs_open) printf("fs> ");
        else{
            printf("fs@%s:",filename);
            print_path();
        }

        if (!fgets(line, sizeof(line), stdin)) {        // EOF (Ctrl‑D)
            putchar('\n');
            break;
        }
        // Removes final newline
        line[strcspn(line, "\n")] = 0;

        // Ignores empty lines or lines just made of spaces
        if (strspn(line, " \t") == strlen(line)) continue;

        // 1st token = command
        char* cmd = strtok(line, " ");
        if (!cmd) continue;

        // Quit and help are always available
        if (strcmp(cmd, "quit") == 0) {
            break;
        } else if (strcmp(cmd, "help") == 0) {
            print_help();
        }else if(strcmp(cmd, "clear") == 0) {
            system("clear");
        }

        // Format
        else if (strcmp(cmd, "format") == 0) {
            if (fs_open) {
                printf("format: another file system is already open\n");
                continue;
            }
            char* a = strtok(NULL, " ");
            char* b = strtok(NULL, " ");
            if (check_arity("format", a && b ? 3 : (a ? 2 : 1), 3) == -1) continue;
            int size = atoi(b);
            if (size <= 0) { printf("format: <size> must be a positive integer\n"); continue; }
            format(a, size);
        }

        // Open
        else if (strcmp(cmd, "open") == 0) {
            if (fs_open) { 
                printf("open: a file system is already open\n"); 
                continue; 
            }
            char* file = strtok(NULL, " ");
            if (check_arity("open", file ? 2 : 1, 2) == -1) continue;
            if(open_fs(file) == -1){
                fs_open = 0;
                printf("open: file system does not exist\n");
                continue;
            }
            fs_open = 1;
            strncpy(filename, file, FILENAME_LEN);
            filename[FILENAME_LEN - 1] = '\0';
        }

        // Close
        else if (strcmp(cmd, "close") == 0) {
            if (!fs_open) { 
                printf("close: no file system is currently open\n"); 
                continue; 
            }
            if (check_arity("close", 1, 1) == -1) continue;     // 1=1 is always true, we want to skip the arity check
            close_fs();
            fs_open = 0;
            filename[0] = '\0';     // very bad way to empty filename array once the FS is close
        }

        // Command listed in the else below require an open file_system
        else {

            if (!fs_open) {
                printf("You must first open a file system with command 'open'.\n");
                continue;
            }

            // mkdir
            if (strcmp(cmd, "mkdir") == 0) {
                char* n = strtok(NULL, " ");
                if (check_arity("mkdir", n ? 2 : 1, 2) == -1) continue;
                _mkdir(n);
            }
            // cd
            else if (strcmp(cmd, "cd") == 0) {
                char* n = strtok(NULL, " ");
                if (check_arity("cd", n ? 2 : 1, 2) == -1) continue;
                _cd(n);
            }
            // touch
            else if (strcmp(cmd, "touch") == 0) {
                char* n = strtok(NULL, " ");
                if (check_arity("touch", n ? 2 : 1, 2) == -1) continue;
                _touch(n);
            }
            // cat
            else if (strcmp(cmd, "cat") == 0) {
                char* n = strtok(NULL, " ");
                if (check_arity("cat", n ? 2 : 1, 2) == -1) continue;
                _cat(n);
            }
            // ls
            else if (strcmp(cmd, "ls") == 0) {
                char* n = strtok(NULL, " ");
                if (check_arity("ls", n ? 2 : 1, 2) == -1) continue;
                _ls(n);
            }
            // rm
            else if (strcmp(cmd, "rm") == 0) {
                char* n = strtok(NULL, " ");
                if (check_arity("rm", n ? 2 : 1, 2) == -1) continue;
                _rm(n);
            }
            // append
            else if (strcmp(cmd, "append") == 0) {
                char* file = strtok(NULL, " ");
                char* text = strtok(NULL, "");      // whatever is left is text
                int provided = file ? (text ? 3 : 2) : 1;
                if (check_arity("append", provided, 3) == -1) continue;
                _append(file, text);
            }

            // If the command is unknown
            else printf("Command not recognised, type 'help' for command list.\n");
        }
    }

    // Clean FS closing
    if (fs_open) 
        close_fs();

    printf("Bye!\n");
    return 0;
}