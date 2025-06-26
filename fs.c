#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "fs.h"

void* fs_data = NULL;              // only used for mmapping, useless later
FileSystem* fs = NULL;
int fs_fd = -1;
int* fat = NULL;                   // FAT array
char* data = NULL;                 // data buffer
FSEntry* current_dir = NULL;       // pointer 
int current_cluster;        // index of current cluster
int current_entry_count;    // number of entries in current directory

// Creates file system named <fs_filename> of <size> bytes
void format(const char* fs_filename, int size){
    int cluster_count = size/CLUSTER_SIZE;
    int fat_bytes = cluster_count * sizeof(int);
    int fat_clusters = (fat_bytes + CLUSTER_SIZE - 1)/CLUSTER_SIZE;     // how many clusters do I need to store all FAT bytes?
    int fat_start = 1;                                                  // entry 0 of FAT is usually reserved for Boot Sector
    int data_start = fat_start + fat_clusters;

    fs_fd = open(fs_filename, O_CREAT | O_RDWR, 0600);
    assert(fs_fd > 0 && "file open failed");

    assert(!ftruncate(fs_fd, size) && "ftruncate failed");

    fs_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);
    assert(fs_data != MAP_FAILED && "mmap failed");

    fs = (FileSystem*)fs_data;
    fs->total_cluster = cluster_count;
    fs->fat_start = fat_start;
    fs->data_start = data_start;

    fat = (int*)(fs_data + CLUSTER_SIZE * fat_start);                // FAT clusters are stored after Boot Sector cluster
    data = fs_data + CLUSTER_SIZE * data_start;                      // data clusters start after Boot Sector + FAT clusters

    // Initialize FAT (0 means free cluster, -1 means EOC)
    for(int i=0; i<cluster_count; i++)
        fat[i] = 0;

    fs->root_cluster = data_start;                                   // root cluster is the first of data clusters
    fat[fs->root_cluster] = FAT_EOC;

    // Initialize root dir with 0
    *(int*)(data + CLUSTER_SIZE * fs->root_cluster) = 0;
    
    assert(!munmap(fs_data, size) && "munmap failed");
    assert(!close(fs_fd) && "file close failed");
}

// Opens <fs_filename>
void open_fs(const char* fs_filename) {
    fs_fd = open(fs_filename, O_RDWR);
    assert(fs_fd > 0 && "open failed");

    struct stat st;
    assert(fstat(fs_fd, &st) == 0 && "fstat failed");
    int size = st.st_size;

    fs_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);
    assert(fs_data != MAP_FAILED && "mmap failed");

    // Retrieves all FS parameters
    fs = (FileSystem*)fs_data;

    fat = (int*)(fs_data + CLUSTER_SIZE * fs->fat_start);
    data = (char*)(fs_data + CLUSTER_SIZE * fs->data_start);

    current_cluster = fs->root_cluster;
    current_dir = (FSEntry*)(data + CLUSTER_SIZE * (current_cluster - fs->data_start));
    current_entry_count = *(int*)(data + CLUSTER_SIZE * (current_cluster - fs->data_start));
}

void close_fs(int size) {
    assert(!munmap(fs_data, size) && "munmap failed");
    assert(!close(fs_fd) && "file close failed");
    fs = NULL;
    fs_fd = -1;
    fs_data = NULL;
    fat = NULL;
    data = NULL;
    current_dir = NULL;
}

