#include <stdio.h>
#include <stdlib.h>

#define FILENAME_LEN 32
#define CLUSTER_SIZE 512
#define MAX_BLOCKS 10000
#define FAT_EOC -1
#define MAX_ENTRIES (CLUSTER_SIZE - sizeof(int)) / sizeof(FSEntry)
#define MAX_PATH_COMPONENTS 7


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
void close_fs();
void _mkdir(const char* path);
void _rm(const char* path);
int _cd(const char* path);
void _ls(const char* path);
void _touch(const char* path);
int insert_entry_in_directory(FSEntry entry);
int remove_entry_from_directory(const char* name);
int allocate_new_cluster(int last_cluster);
void free_cluster_chain(int cluster);

