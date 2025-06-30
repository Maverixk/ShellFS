#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"


/*
Per ora la shell è scriptata (anche male peraltro),
quando avrò finito di ragionare sul FS farò anche il tokenizer
e l'interfaccia per la shell.
*/

int main(){
    int d;
    do{
        printf("Insert a command:");
        scanf("%d", &d);

        if(d == 1){
            printf("Lancio \"format file_system.xyz 2048\"\n");
            format("file_system.xyz", 5120);
        }
        else if(d == 2){
            printf("Lancio \"open file_system.xyz\"\n");
            open_fs("file_system.xyz");
        }
        else if(d == 3){
            printf("Lancio \"close file_system.xyz\"\n");
            close_fs(200);
        }
        else if(d == 4){
            printf("Lancio \"mkdir warius\"\n");
            _mkdir("warius");
        }
        else if(d == 5){
            printf("Lancio \"cd warius\"\n");
            _cd("warius");
           // print_path(); la implemento dopo, ora ho riunione a SPV :(
        }

    }while(d != 23);

    _touch("marius.txt");
    _touch("valerius.txt");
    _touch("lorenzo.txt");
    _ls(".");
    _append("lorenzo.txt", "ciao marius!");
    _cat("marius.txt");
    _cat("lorenzo.txt");
    _rm("lorenzo.txt");
    _cat("lorenzo.txt");
}