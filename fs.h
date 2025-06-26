#include <stdio.h>
#include <stdlib.h>

#define FILENAME_LEN 32
#define CLUSTER_SIZE 512
#define MAX_BLOCKS 10000
#define FAT_EOC -1


// Data structures
typedef struct FSEntry{
    char name[FILENAME_LEN];
    int is_dir;     // 0=file, 1=directory
    int start_cluster;
    int size;   // in bytes
} FSEntry;

typedef struct FileSystem{
    int total_cluster;
    int root_cluster;
    int fat_start;
    int data_start;
} FileSystem;

// FS functions
void format(const char* fs_filename, int size);
void open_fs(const char* fs_filename);
void close_fs(int size);
