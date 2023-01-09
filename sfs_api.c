// Alex Zhou
// 260970063

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <ucontext.h>
#include <fcntl.h>
#include <unistd.h>

#include "disk_emu.h"

#include "types/inode_table.h"
#include "types/fd_table.h"
#include "types/directory.h"

#include "sfs_api.h"

//***********************GLOBAL DS***********************
// in-memory
fd_table_t fd_table;
dir_table_t dir_table;

// on-disk
superblock_t sb;
inode_table_t inode_table;
bitmap_t bm;

int err;

//***********************INIT FUNC***********************

void superblock_init(superblock_t *sb)
{
    sb->magic = 0;
    sb->block_size = BLOCK_SIZE;
    sb->fs_size = NUM_BLOCKS;
    sb->inode_length = sizeof(inode_table_t);
    sb->rd = 0;
}

void bitmap_init(bitmap_t *bm)
{
    for (int i = 0; i < BM_BYTE_SIZE; i++)
    {
        bm->data[i] = 0;
    }
}

//***********************HELPER***********************

// sets a bit at index to be used
void bitmap_set_true(int index)
{
    bm.data[index] = 1;
}

// sets a bit at index to be free
void bitmap_set_false(int index)
{
    bm.data[index] = 0;
}

// finds a free bit in bitmap and sets it used.
int bitmap_find()
{
    for (int i = 0; i < BM_BYTE_SIZE; i++)
    {
        if (bm.data[i] == 0)
        {
            bitmap_set_true(i);
            return i + DATA_LOCATION;
        }
    }
    return -1;
}

// get the max of two numbers
int max(int x, int y)
{
    if (x > y)
    {
        return x;
    }
    return y;
}

// get the min of two numbers
int min(int x, int y)
{
    if (x > y)
    {
        return y;
    }
    return x;
}

// sets a given block to be empty. DOES NOT ERROR CHECK
void clear_block(int start_address)
{
    char emptyblock[BLOCK_SIZE];
    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        emptyblock[i] = 0;
    }
    write_blocks(start_address, 1, emptyblock);
}

// writes in-memory data structures onto disk
void save_to_disk()
{
    write_blocks(INODE_LOCATION, INODE_TBL_SIZE, &inode_table);
    write_blocks(RD_LOCATION, DIR_TABLE_SIZE, &dir_table);
    write_blocks(BM_LOCATION, BM_SIZE, &bm);
}

//***********************FUNCS************************

/*
Sets up on-disk data structures for the file system
args:
int fresh: flag to create a new fs or opened from disk
returns: NA
*/
void mksfs(int fresh)
{
    // memset in-memory data structures
    memset(&inode_table, 0, sizeof(inode_table_t));
    memset(&fd_table, 0, sizeof(fd_table_t));
    memset(&dir_table, 0, sizeof(dir_table_t));

    if (fresh)
    {
        // create init superblock
        err = init_fresh_disk(FS_NAME, BLOCK_SIZE, NUM_BLOCKS);
        if (err != 0)
        {
            return;
        }
        // init data structures
        superblock_init(&sb);
        inode_table_init(&inode_table);
        dir_table_init(&dir_table);
        bitmap_init(&bm);

        fdt_init(&fd_table);

        // write data structures to disk
        write_blocks(SB_LOCATION, SB_SIZE, &sb);
        save_to_disk();
    }
    else
    {
        err = init_disk(FS_NAME, BLOCK_SIZE, NUM_BLOCKS);
        if (err != 0)
        {
            return;
        }

        // temporary buffers before copying to data structures.
        char temp_sb[SB_SIZE * BLOCK_SIZE];
        char temp_inodet[INODE_TBL_SIZE * BLOCK_SIZE];
        char temp_rd[DIR_TABLE_SIZE * BLOCK_SIZE];
        char temp_bm[BM_SIZE * BLOCK_SIZE];

        read_blocks(SB_LOCATION, SB_SIZE, &temp_sb);
        read_blocks(INODE_LOCATION, INODE_TBL_SIZE, &temp_inodet);
        read_blocks(RD_LOCATION, DIR_TABLE_SIZE, &temp_rd);
        read_blocks(BM_LOCATION, BM_SIZE, &temp_bm);

        memcpy(&sb, &temp_sb, sizeof(superblock_t));
        memcpy(&inode_table, &temp_inodet, sizeof(inode_table_t));
        memcpy(&dir_table, &temp_rd, sizeof(dir_table_t));
        memcpy(&bm, &temp_bm, sizeof(bitmap_t));

        fdt_init(&fd_table);
    }
}

/*
Gets the next filename.
args:
char *fname: buffer of where to place the next filename.
returns: 1 on success, 0 on fail
 */
int sfs_getnextfilename(char *fname)
{
    for (int i = dir_table.index; i < DIR_TABLE_BYTE_SIZE; i++)
    {
        if (dir_table.data[i].free)
        {
            memcpy(fname, dir_table.data[i].filename, MAX_FILENAME);
            dir_table.index++;
            write_blocks(RD_LOCATION, DIR_TABLE_SIZE, &dir_table);
            return 1;
        }
        dir_table.index++;
    }
    dir_table.index = 0;
    write_blocks(RD_LOCATION, DIR_TABLE_SIZE, &dir_table);
    return 0;
}

/*
Finds file size at specified path
args:
const char *path: specified path
returns: size of path on successful search, -1 when no file found
*/
int sfs_getfilesize(const char *path)
{
    for (int i = 0; i < DIR_TABLE_BYTE_SIZE; i++)
    {
        if (strlen(path) != strlen(dir_table.data[i].filename))
        {
            continue;
        }
        else if (memcmp(dir_table.data[i].filename, path, strlen(path)) == 0)
        {
            return inode_table.data[dir_table.data[i].inode].size;
        }
    }
    return -1;
}

/*
Opens file with given name
args:
char *name: name of file to open
returns: index of file in fd table on success, -1 on fail
*/
int sfs_fopen(char *name)
{
    if (strlen(name) > MAX_FILENAME - 1)
    {
        return -1;
    }
    // lookup name in directory table
    dir_entry_t *entry = dir_lookup_fname(&dir_table, name);

    if (entry == NULL)
    {
        // can't find in dir_table, create new entry
        inode_t inode;
        inode_init(&inode, 0);
        // get slot in inode table
        int inode_index = inode_table_insert(&inode_table, inode);
        if (inode_index == -1)
        {
            return -1;
        }

        // get slot in dir table
        dir_entry_t dir_entry;
        dir_entry_init(&dir_entry, name, inode_index, 1);
        int dir_index = dir_table_insert(&dir_table, dir_entry);
        if (dir_index == -1)
        {
            inode_init(&(inode_table.data[inode_index]), -1);
            return -1;
        }

        // get slot in fd table
        fd_entry_t fd_entry;
        fde_init(&fd_entry, inode_index, 0, 0);
        int fd_index = fdt_insert(&fd_table, fd_entry);
        if (fd_index == -1)
        {
            inode_init(&(inode_table.data[inode_index]), -1);
            dir_entry_init(&(dir_table.data[dir_index]), "", 0, 0);
            return -1;
        }

        save_to_disk();
        return fd_index;
    }
    else
    {
        // entry found
        int fd_index = fdt_lookup(&fd_table, entry->inode);
        if (fd_index != -1)
        {
            // entry already open, return index in fd table
            return fd_index;
        }
        else
        {
            // open entry in fd table
            fd_entry_t fd_entry;
            fde_init(&fd_entry, entry->inode, 0, inode_table.data[entry->inode].size);
            fd_index = fdt_insert(&fd_table, fd_entry);
            return fd_index;
        }
    }
    return -1;
}

/*
Closes a file by removing file from fd table
args:
int fileID: index in fd_table to close
returns: 0 on success, -1 on fail
*/
int sfs_fclose(int fileID)
{
    return fdt_remove(&fd_table, fileID);
}

/*
Writes a single block to a given location
args:
int start_address: block to write to
char *buffer: buffer containing what to write
int offset: starting point of writing in block
int length: length of buffer to write
returns: length written
*/
int write_single_block(int start_address, char *buffer, int offset, int length)
{
    // find number of bytes to write
    // fills the block if length > BLOCK_SIZE - offset
    // writes the full length length < BLOCK_SIZE - offset
    int written = min(length, BLOCK_SIZE - offset);

    // allocate temporary buffer in case block being written to already contains something
    char *new_buf = malloc(BLOCK_SIZE);
    read_blocks(start_address, 1, new_buf);
    memcpy(new_buf + offset, buffer, written);
    write_blocks(start_address, 1, new_buf);
    free(new_buf);
    return written;
}

/*
Writes length of characters from buffer to data blocks of file.
args:
int fileID: index of fd table of file to write to
char *buf: buffer containing what to write
int length: length of buffer to write
returns: length written
*/
int sfs_fwrite(int fileID, char *buf, int length)
{
    char *buffer = buf;
    int left_to_write = length;
    int written = 0;

    // check fileID
    if (fileID < 0 || fileID >= FD_TABLE_SIZE)
    {
        return -1;
    }
    fd_entry_t fd = fd_table.data[fileID];

    // check if entry is actually used
    if (fd.inode == -1)
    {
        return -1;
    }
    inode_t inode = inode_table.data[fd.inode];

    // check if indirect ptrs are going to be used
    int ind_block[IND_SIZE];
    int ind_check = 0;
    if (fd.write_ptr + length > BLOCK_SIZE * INODE_NUM_PTRS)
    {
        // check if ind block is allocated
        if (inode.ind_ptr == -1)
        {
            inode.ind_ptr = bitmap_find();
            if (inode.ind_ptr == -1)
            {
                // bitmap full
                return written;
            }
            write_blocks(BM_LOCATION, BM_SIZE, &bm);
        }
        read_blocks(inode.ind_ptr, 1, ind_block);
        ind_check = 1;
    }

    while (left_to_write > 0)
    {
        int block_index = fd.write_ptr / BLOCK_SIZE;
        int block_offset = fd.write_ptr % BLOCK_SIZE;
        int block;

        // use direct pointers
        if (block_index < INODE_NUM_PTRS)
        {
            // check if block is allocated
            block = inode.ptrs[block_index];
            if (block == -1)
            {
                block = bitmap_find();
                if (block == -1)
                {
                    // bitmap full
                    return written;
                }
                write_blocks(BM_LOCATION, BM_SIZE, &bm);
                inode.ptrs[block_index] = block;
            }
        }
        // use indirect ptrs
        else
        {
            if (block_index - INODE_NUM_PTRS >= IND_SIZE)
            {
                // max file size reached
                return written;
            }
            block = ind_block[block_index - INODE_NUM_PTRS];
            if (block == 0)
            {
                block = bitmap_find();
                if (block == -1)
                {
                    // bitmap full
                    return written;
                }

                write_blocks(BM_LOCATION, 2, &bm);
                ind_block[block_index - INODE_NUM_PTRS] = block;
                write_blocks(inode.ind_ptr, 1, &ind_block);
            }
        }

        // write to selected block
        int bytes = write_single_block(block, buffer, block_offset, left_to_write);

        left_to_write -= bytes;
        written += bytes;

        buffer += bytes;
        fd.write_ptr += bytes;
    }

    // clean up inode and fd table
    if (ind_check == 1)
    {
        write_blocks(inode.ind_ptr, 1, ind_block);
    }
    fd_table.data[fileID] = fd;
    inode.size = max(fd.write_ptr, inode.size);
    inode_table.data[fd.inode] = inode;
    write_blocks(INODE_LOCATION, INODE_TBL_SIZE, &inode_table);

    return written;
}

/*
Reads a single block from location
args:
int start_address: block to read from
char *buffer: buffer to read to
int offset: starting point of reading in block
int length: length to read
returns: length read
*/
int read_single_block(int start_address, char *buffer, int offset, int length)
{
    // find number of bytes to write
    int read = min(length, BLOCK_SIZE - offset);

    // allocate temporary buffer in case we're not reading the full block
    char *new_buf = malloc(BLOCK_SIZE);
    read_blocks(start_address, 1, new_buf);
    memcpy(buffer, new_buf + offset, read);
    free(new_buf);
    return read;
}

/*
Reads length of characters from data blocks of file to buffer.
args:
int fileID: index of fd table of file to write to
char *buf: buffer to be read to
int length: length of file to read
returns: length read
*/
int sfs_fread(int fileID, char *buf, int length)
{
    char *buffer = buf;
    int left_to_read = length;
    int read = 0;

    // check fileID
    if (fileID < 0 || fileID >= FD_TABLE_SIZE)
    {
        return -1;
    }
    fd_entry_t fd = fd_table.data[fileID];

    // check if entry is actually used
    if (fd.inode == -1)
    {
        return -1;
    }
    inode_t inode = inode_table.data[fd.inode];

    left_to_read = min(inode.size - fd.read_ptr, length);

    // check if indirect ptrs are going to be used
    int ind_block[BLOCK_SIZE];
    if (fd.read_ptr + length > BLOCK_SIZE * INODE_NUM_PTRS)
    {
        // check if ind block is allocated
        if (inode.ind_ptr == -1)
        {
            // non-allocated ind block trying to be used, return
            return read;
        }
        read_blocks(inode.ind_ptr, 1, ind_block);
    }

    while (left_to_read > 0)
    {
        int block_index = fd.read_ptr / BLOCK_SIZE;
        int block_offset = fd.read_ptr % BLOCK_SIZE;
        int block;

        // use direct pointers
        if (block_index < INODE_NUM_PTRS)
        {
            // check if block is allocated
            block = inode.ptrs[block_index];
            if (block == -1)
            {
                // non-allocated block trying to be read from, return
                return read;
            }
        }
        // use indirect ptrs
        else
        {
            block = ind_block[block_index - INODE_NUM_PTRS];
            if (block == 0)
            {
                // non-allocated block trying to be read from, return
                return read;
            }
        }

        // read from selected block
        int bytes = read_single_block(block, buffer, block_offset, left_to_read);

        left_to_read -= bytes;
        read += bytes;

        buffer += bytes;
        fd.read_ptr += bytes;
    }

    // update read pointer in fd table
    fd_table.data[fileID] = fd;
    return read;
}

/*
Sets read and write pointers of a opened file to set location.
args:
int fileID: index of fd table of file
int loc: value to set read and write pointer to
returns: length written
*/
int sfs_fseek(int fileID, int loc)
{
    // check fileID
    if (fileID < 0 || fileID >= FD_TABLE_SIZE)
    {
        return -1;
    }
    fd_table.data[fileID].read_ptr = loc;
    fd_table.data[fileID].write_ptr = loc;
    return 0;
}

/*
Remove an entire file from the system
args:
char *file: name of file to remove
returns: 0 on success, -1 in failure
*/
int sfs_remove(char *file)
{
    dir_entry_t *entry = dir_lookup_fname(&dir_table, file);
    if (entry == NULL)
    {
        return -1;
    }

    inode_t *inode = &(inode_table.data[entry->inode]);

    // clean data from direct pointers
    for (int i = 0; i < INODE_NUM_PTRS; i++)
    {
        if (inode->ptrs[i] != -1)
        {
            clear_block(inode->ptrs[i]);
            bitmap_set_false(inode->ptrs[i]);
        }
    }

    // clean data from indirect pointers
    if (inode->ind_ptr != -1)
    {
        int ind_block[IND_SIZE];
        read_blocks(inode->ind_ptr, 1, ind_block);

        for (int i = 0; i < IND_SIZE; i++)
        {
            if (ind_block[i] == 0)
            {
                break;
            }
            clear_block(ind_block[i]);
            bitmap_set_false(ind_block[i]);
        }
        clear_block(inode->ind_ptr);
        bitmap_set_false(inode->ind_ptr);
    }

    // clean fd_entry
    fdt_remove(&fd_table, fdt_lookup(&fd_table, entry->inode));

    // clean inode
    inode_init(inode, -1);

    // clean entry
    dir_entry_init(entry, "", 0, 0);

    dir_table.index = 0;
    save_to_disk();
    return 0;
}