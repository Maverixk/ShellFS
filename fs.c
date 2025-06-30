#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "fs.h"

void *fs_data = NULL; // only used for mmapping, useless later
FileSystem *fs = NULL;
int fs_fd = -1;
int fs_size = -1;
int *fat = NULL;             // FAT array
char *data = NULL;           // data buffer
FSEntry *current_dir = NULL; // pointer
int current_cluster;         // index of current cluster
int current_entry_count;     // number of entries in current directory

// Creates file system named <fs_filename> of <size> bytes
void format(const char *fs_filename, int size){
    int cluster_count = size / CLUSTER_SIZE;
    int fat_bytes = cluster_count * sizeof(int);
    int fat_clusters = (fat_bytes + CLUSTER_SIZE - 1) / CLUSTER_SIZE; // how many clusters do I need to store all FAT bytes?
    int fat_start = 1;                                                // entry 0 of FAT is usually reserved for Boot Sector
    int data_start = fat_start + fat_clusters;

    fs_fd = open(fs_filename, O_CREAT | O_RDWR, 0600);
    assert(fs_fd > 0 && "file open failed");

    assert(!ftruncate(fs_fd, size) && "ftruncate failed");

    fs_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);
    assert(fs_data != MAP_FAILED && "mmap failed");

    fs = (FileSystem *)fs_data;
    fs->total_cluster = cluster_count;
    fs->fat_start = fat_start;
    fs->data_start = data_start;

    fat = (int *)(fs_data + CLUSTER_SIZE * fat_start); // FAT clusters are stored after Boot Sector cluster
    data = fs_data + CLUSTER_SIZE * data_start;        // data clusters start after Boot Sector + FAT clusters

    // Initialize FAT (0 means free cluster, -1 means EOC)
    for (int i = 0; i < cluster_count; i++)
        fat[i] = 0;

    fs->root_cluster = data_start; // root cluster is the first of data clusters
    fat[fs->root_cluster] = FAT_EOC;

    // Initialize root dir with 0
    *(int *)(data + CLUSTER_SIZE * fs->root_cluster) = 0;

    assert(!munmap(fs_data, size) && "munmap failed");
    assert(!close(fs_fd) && "file close failed");
}

// Opens <fs_filename>
void open_fs(const char *fs_filename){
    fs_fd = open(fs_filename, O_RDWR, 0600);
    if(fs_fd < 0){
        printf("open: file system does not exist\n");
        return;
    }

    struct stat st;
    assert(fstat(fs_fd, &st) == 0 && "fstat failed");
    fs_size = st.st_size;

    fs_data = mmap(NULL, fs_size, PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);
    assert(fs_data != MAP_FAILED && "mmap failed");

    // Retrieves all FS parameters
    fs = (FileSystem *)fs_data;
    assert(fs != NULL && "FS address error");

    fat = (int *)(fs_data + CLUSTER_SIZE * fs->fat_start);
    data = (char *)(fs_data + CLUSTER_SIZE * fs->data_start);

    // We start from root
    current_cluster = fs->root_cluster;
    assert(current_cluster >= fs->data_start && current_cluster < fs->total_cluster && "current cluster out of bounds");
    current_dir = (FSEntry *)(data + CLUSTER_SIZE * (current_cluster - fs->data_start) + sizeof(int));
    current_entry_count = *(int *)(data + CLUSTER_SIZE * (current_cluster - fs->data_start));

}

// Closes currently open FS
void close_fs(){
    assert(!munmap(fs_data, fs_size) && "munmap failed");
    assert(!close(fs_fd) && "file close failed");
    fs = NULL;
    fs_fd = -1;
    fs_data = NULL;
    fat = NULL;
    data = NULL;
    current_dir = NULL;
}

// Creates a directory starting from a simple dir name
void _mkdir(const char *name){
    // Check that dir name isn't longer than FILENAME_LEN bytes
    if (strlen(name) >= FILENAME_LEN){
        printf("mkdir: name too long\n");
        return;
    }

    // Can't create dir with no name or .(current), ..(parent) name, I'll cry
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        printf("mkdir: invalid directory name\n");
        return;
    }

    // Check if name has already been used
    int temp_cluster = current_cluster;
    while (temp_cluster != FAT_EOC){
        void *cluster_data = data + CLUSTER_SIZE * (temp_cluster - fs->data_start);
        FSEntry *entries = (FSEntry *)(cluster_data + sizeof(int));
        int cluster_entry_count = *(int *)(cluster_data);
        for (int i = 0; i < cluster_entry_count; i++){
            if (strcmp(entries[i].name, name) == 0){
                printf("mkdir: directory '%s' is already existing\n", name);
                return;
            }
        }
        temp_cluster = fat[temp_cluster];
    }

    // Find a free cluster on FAT
    int new_cluster = -1;
    for (int i = fs->data_start; i < fs->total_cluster; i++){
        if (fat[i] == 0){
            fat[i] = FAT_EOC;
            new_cluster = i;
            break;
        }
    }

    if (new_cluster == -1){
        printf("mkdir: no empty space\n");
        return;
    }

    // Add entry to current directory
    FSEntry entry;
    strncpy(entry.name, name, FILENAME_LEN);
    entry.name[FILENAME_LEN - 1] = '\0';
    entry.is_dir = 1;
    entry.start_cluster = new_cluster;
    entry.size = 0;

    if (insert_entry_in_directory(entry)){
        printf("mkdir: not enough space to insert entry\n");
        return;
    }

    // Initialize new cluster entry count to 2
    FSEntry *new_dir_entries = (FSEntry *)(data + CLUSTER_SIZE * (new_cluster - fs->data_start) + sizeof(int));
    *(int *)(data + CLUSTER_SIZE * (new_cluster - fs->data_start)) = 2;

    // To make cd command code easier I want to map self and parent dir in the new dir entries array
    strcpy(new_dir_entries[0].name, ".");
    new_dir_entries[0].is_dir = 1;
    new_dir_entries[0].start_cluster = new_cluster;

    strcpy(new_dir_entries[1].name, "..");
    new_dir_entries[1].is_dir = 1;
    new_dir_entries[1].start_cluster = current_cluster;  
}

void _cd(const char *name){
    // Check that dir name isn't longer than FILENAME_LEN bytes
    if (strlen(name) >= FILENAME_LEN){
        printf("mkdir: name too long\n");
        return;
    }

    // I have to consider the possibility that user may want to go back to root directory
    if (strcmp(name, "/") == 0){
        current_cluster = fs->root_cluster;
        current_dir = (FSEntry *)(data + CLUSTER_SIZE * (current_cluster - fs->data_start) + sizeof(int));
        current_entry_count = *(int *)(data + CLUSTER_SIZE * (current_cluster - fs->data_start));
        return;
    }

    int cluster = current_cluster;

    if (strcmp(name, ".") == 0){
        // Don't you dare moving
    }
    else if (strcmp(name, "..") == 0){
        void *cluster_ptr = data + CLUSTER_SIZE * (cluster - fs->data_start);
        FSEntry *entries = (FSEntry *)(cluster_ptr + sizeof(int));
        int entry_count = *(int *)cluster_ptr;

        // If user wants to go up the tree, I look for parent directory in the dir entries array
        int found = 0;
        for (int i = 0; i < entry_count; i++){
            if (entries[i].is_dir && strcmp(entries[i].name, "..") == 0){
                cluster = entries[i].start_cluster;
                found = 1;
                break;
            }
        }

        if (!found){
            printf("cd: no parent directory\n");
            return;
        }
    }
    else{
        int found = 0;
        int temp_cluster = cluster;

        // Scan through all dir clusters until I found the subdir I'm looking for
        while (temp_cluster != FAT_EOC){
            void *cluster_ptr = data + CLUSTER_SIZE * (temp_cluster - fs->data_start);
            int entry_count = *(int *)cluster_ptr;
            FSEntry *entries = (FSEntry *)(cluster_ptr + sizeof(int));

            for (int i = 0; i < entry_count; i++){
                if (entries[i].is_dir && strcmp(entries[i].name, name) == 0){
                    cluster = entries[i].start_cluster;
                    found = 1;
                    break;
                }
            }

            if (found) break;
            temp_cluster = fat[temp_cluster];
        }

        if (!found){
            printf("cd: directory '%s' not found\n", name);
            return;
        }
    }

    // Update cluster information
    current_cluster = cluster;
    current_dir = (FSEntry *)(data + CLUSTER_SIZE * (cluster - fs->data_start) + sizeof(int));
    current_entry_count = *(int *)(data + CLUSTER_SIZE * (cluster - fs->data_start));
}

void _rm(const char* name){
    // Check that dir name isn't longer than FILENAME_LEN bytes
    if (strlen(name) >= FILENAME_LEN){
        printf("mkdir: name too long\n");
        return;
    }

    int found = 0;
    int cluster = current_cluster;

    while(!found && cluster != FAT_EOC){
        void* cluster_ptr = data + CLUSTER_SIZE * (cluster - fs->data_start);
        FSEntry* entries = (FSEntry*)(cluster_ptr + sizeof(int));
        int entry_count = *(int*)cluster_ptr;

        for(int i = 0; i < entry_count; i++){
            if(strcmp(entries[i].name, name) == 0){
                found = 1;

                // If the entry is a directory and it's not empty we can't remove it (same as we can't create directories recursively)
                if(entries[i].is_dir){
                    int dir_cluster = entries[i].start_cluster;
                    int entry_count = *(int*)(data + CLUSTER_SIZE * (dir_cluster - fs->data_start));

                    // There are other entries apart from . and ..
                    if(entry_count > 2){
                        printf("rm: directory not empty\n");
                        return;
                    }
                }

                free_cluster_chain(entries[i].start_cluster);

                if(remove_entry_from_directory(entries[i].name) == -1)
                    printf("rm: error removing entry\n");

                return;
            }
        }
        cluster = fat[cluster];
    }

    if(!found){
        printf("rm: '%s' not found\n", name);
        return;
    }
}

void _ls(const char* name){
    // Check that dir name isn't longer than FILENAME_LEN bytes
    if (strlen(name) >= FILENAME_LEN){
        printf("mkdir: name too long\n");
        return;
    }

    // We look for the directory in the current directory
    int found = 0;
    for(int i = 0; i < current_entry_count; i++){
        if(strcmp(name, current_dir[i].name) == 0){
            found = 1;
            if(current_dir[i].is_dir){
                int cluster = current_dir[i].start_cluster;
                void* cluster_ptr = data + CLUSTER_SIZE * (cluster - fs->data_start);
                FSEntry* entries = (FSEntry*)(cluster_ptr + sizeof(int));
                int entry_count = *(int*)cluster_ptr;

                printf("ls: ");
                for(int i = 0; i < entry_count; i++){
                    printf("%s", entries[i].name);
                    if(i != entry_count - 1) printf(" | ");
                    else printf("\n");
                }
            }
            else{
                printf("ls: '%s' not a directory\n", name);
                return;
            }
        }
    }

    if(!found){
        printf("ls: '%s' not found\n", name);
        return;
    }
}

void _touch(const char* name){
    // We want the filename to stay within FILENAME_LEN bytes
    if(strlen(name) >= FILENAME_LEN){
        printf("touch: name is too long\n");
        return;
    }

    // Can't create file with no name or .(current), ..(parent) name, I'll cry
    if (strlen(name) == 0 || strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        printf("touch: invalid file name\n");
        return;
    }

    // Check if name has already been used in current directory
    int temp_cluster = current_cluster;
    while(temp_cluster != FAT_EOC){
        void* cluster_data = data + CLUSTER_SIZE * (temp_cluster - fs->data_start);
        int entry_count = *(int*)cluster_data;
        FSEntry* entries = (FSEntry*)(cluster_data + sizeof(int));
        for(int i = 0; i < entry_count; i++){
            if(strcmp(entries[i].name, name) == 0){
                printf("touch: file '%s' is already existing\n", name);
                return;
            }
        }
        temp_cluster = fat[temp_cluster];
    }

    // Find a free cluster on FAT
    int new_cluster = -1;
    for(int i = fs->data_start; i < fs->total_cluster; i++){
        if(fat[i] == 0){
            fat[i] = FAT_EOC;
            new_cluster = i;
            break;
        }
    }

    if(new_cluster == -1){
        printf("touch: no empty space\n");
        return;
    }

    // Add file to current directory
    FSEntry entry;
    strncpy(entry.name, name, FILENAME_LEN);
    entry.name[FILENAME_LEN - 1] = '\0';
    entry.is_dir = 0;
    entry.size = 0;
    entry.start_cluster = new_cluster;

    if(insert_entry_in_directory(entry) == -1){
        printf("touch: not enough space to insert entry\n");
        return;
    }
}

void _cat(const char* name){
    // We want the filename to stay within FILENAME_LEN bytes
    if(strlen(name) >= FILENAME_LEN){
        printf("touch: name is too long\n");
        return;
    }

    int cluster = current_cluster;
    void* cluster_ptr = data + CLUSTER_SIZE * (cluster - fs->data_start);
    FSEntry* entries = (FSEntry*)(cluster_ptr + sizeof(int));
    int entry_count = *(int*)cluster_ptr;

    int found = 0;
    for(int i = 0; i < entry_count; i++){
        if(strcmp(entries[i].name, name) == 0){
            found = 1;
            if(!entries[i].size){
                printf("cat: empty file\n");
                return;
            }
            if(!entries[i].is_dir) read_file(entries[i].start_cluster, entries[i].size);
            else printf("cat: '%s' not a file\n", name);
            break;
        }
    }

    if(!found) printf("cat: '%s' not found\n", name);
}

// Starting from a certain cluster, allocate a new one and mark it as FAT_EOC
int allocate_new_cluster(int last_cluster){
    for (int i = fs->data_start; i < fs->total_cluster; i++){
        if (fat[i] == 0){
            fat[i] = FAT_EOC;
            memset(data + CLUSTER_SIZE * (i - fs->data_start), 0, CLUSTER_SIZE);
            fat[last_cluster] = i;
            return i;
        }
    }
    return -1; // No space available
}

// Starting from a certain cluster, free all the clusters in the chain
void free_cluster_chain(int cluster){
    while(cluster != FAT_EOC){
        int next = fat[cluster];
        fat[cluster] = 0;
        cluster = next;
    }
}

int insert_entry_in_directory(FSEntry entry){
    int cluster = current_cluster;

    while (1){
        void *cluster_ptr = data + CLUSTER_SIZE * (cluster - fs->data_start);
        int *entry_count_ptr = (int *)cluster_ptr;
        FSEntry *entries = (FSEntry *)(cluster_ptr + sizeof(int));

        // How many FSEntries can fit in a cluster? If I haven't exceed that number I can still use this cluster
        if (*entry_count_ptr < MAX_ENTRIES){
            entries[*entry_count_ptr] = entry;
            (*entry_count_ptr)++;
            return 0;
        }

        // If I can't fit any more FSEntry in this cluster, and it's the last cluster available, we allocate a new one
        if (fat[cluster] == FAT_EOC){
            int new_cluster = allocate_new_cluster(cluster);
            if (new_cluster == -1)
                return -1;
            cluster = new_cluster;
        }
        // If it is not the last one we make sure to reach the end of the cluster chain
        else cluster = fat[cluster];
    }

    return 0;       // technically never used cause we're stuck in a while(1) cycle
}

int remove_entry_from_directory(const char* name){
    int cluster = current_cluster;

    while(1){
        void* cluster_ptr = data + CLUSTER_SIZE * (cluster - fs->data_start);
        FSEntry* entries = (FSEntry*)(cluster_ptr + sizeof(int));
        int* entry_count_ptr = (int*)cluster_ptr;

        // If we find the entry, we shift all the following entries backwards one position
        for(int i = 0; i < *entry_count_ptr; i++){
            if(strcmp(entries[i].name, name) == 0){
                for(int j = i; j < *entry_count_ptr; j++)
                    entries[j] = entries[j+1];
            }
            (*entry_count_ptr)--;
            return 0;
        }
        if(fat[cluster] == FAT_EOC) break;
        cluster = fat[cluster];
    }
    return -1;       // Not found
}

void read_file(int start_cluster, int size){
    // Check that cluster is within data bound
    if(start_cluster < fs->data_start || start_cluster >=fs->total_cluster){
        printf("cat: invalid cluster\n");
        return;
    }

    int cluster = start_cluster;
    int remaining = size;

    // For each cluster, read its content and jump onto the next
    while(cluster != FAT_EOC && remaining > 0){
        char* payload = data + CLUSTER_SIZE * (cluster - fs->data_start);
        int chunk = remaining < CLUSTER_SIZE ? remaining : CLUSTER_SIZE;

        fwrite(payload, 1, chunk, stdout);
        remaining -= chunk;
        cluster = fat[cluster];
    }

    if(remaining == 0) printf("\n");
    else printf("cat: couldn't read entire file\n");
}
