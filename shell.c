#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"


int main(){
    int d;
    do{
        printf("Insert a command:");
        scanf("%d", &d);

        if(d == 1){
            printf("Lancio \"format file_system.xyz 200\"\n");
            format("file_system.xyz", 200);
        }
        else if(d == 2){
            printf("Lancio \"open file_system.xyz\n");
            open_fs("file_system.xyz");
        }
        else if(d == 3){
            printf("Lancio \"close file_system.xyz\n");
            close_fs(200);
        }
    }while(d != 23);
   
}