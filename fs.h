#include <stdio.h>
#include <stdlib.h>

#define FILENAME_LEN 32
#define CLUSTER_SIZE 512
#define FAT_EOC -1
#define MAX_ENTRIES (CLUSTER_SIZE - sizeof(int)) / sizeof(FSEntry)
#define MAX_DEPTH 100

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
int open_fs(const char* fs_filename);
void close_fs();
void _mkdir(const char* name);
void _rm(const char* name);
void _cd(const char* name);
void _ls(const char* name);
void _touch(const char* name);
void _cat(const char* name);
void _append(const char* name, const char* text);
int insert_entry_in_directory(FSEntry entry);
int remove_entry_from_directory(const char* name);
int allocate_new_cluster(int last_cluster);
void free_cluster_chain(int cluster);
void read_file(int start_cluster, int size);
void write_file(int start_cluster, int size, const char* text);
void print_path();
