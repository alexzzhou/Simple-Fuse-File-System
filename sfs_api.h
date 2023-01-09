// Alex Zhou
// 260970063

#ifndef SFS_API_H
#define SFS_API_H 

#define BLOCK_SIZE      1024
#define NUM_BLOCKS      2000
#define NUM_DATA_BLOCKS 1923
#define CACHE_SIZE      500
#define BM_SIZE         2
#define BM_BYTE_SIZE         NUM_DATA_BLOCKS
#define IND_SIZE        BLOCK_SIZE/sizeof(int)

#define MAX_FILE_SIZE   1024
#define MAXFILENAME     60

#define FS_NAME "alex_fs_disk"

#define SB_SIZE         1

//Disk Indexes
#define SB_LOCATION     0       //superblock is 1 block
#define INODE_LOCATION  1       //i-node table is 10 blocks
#define RD_LOCATION     11      //root directory is 64 blocks
#define DATA_LOCATION   75      //data blocks are 2000 - (1 + 10 + 64 + 2) = 1921
#define BM_LOCATION     1997    //bitmap is NUM_BLOCKS/BLOCK_SIZE = 2

typedef struct _superblock_t {
    int magic;
    int block_size;
    int fs_size;
    int inode_length;
    int rd;
} superblock_t;

typedef struct _bitmap_t{
    int data[BM_BYTE_SIZE];
} bitmap_t;

void mksfs(int);

int sfs_getnextfilename(char*);

int sfs_getfilesize(const char*);

int sfs_fopen(char*);

int sfs_fclose(int);

int sfs_fwrite(int, char*, int);

int sfs_fread(int, char*, int);

int sfs_fseek(int, int);

int sfs_remove(char*);

#endif
