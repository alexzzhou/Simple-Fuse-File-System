// Alex Zhou
// 260970063

#define FD_TABLE_SIZE 1000

typedef struct _fd_entry_t
{
    int inode; //-1 when not used
    int read_ptr;
    int write_ptr;
} fd_entry_t;

typedef struct _fd_table_t
{
    fd_entry_t data[FD_TABLE_SIZE];
} fd_table_t;

void fde_init(fd_entry_t *entry, int i_ptr, int r_ptr, int w_ptr)
{
    entry->inode = i_ptr;
    entry->read_ptr = r_ptr;
    entry->write_ptr = w_ptr;
}

void fdt_init(fd_table_t *table)
{
    fd_entry_t fd_entry;
    // all inodes are initialized to -1
    fde_init(&fd_entry, -1, 0, 0);
    for (int i = 0; i < FD_TABLE_SIZE; i++)
    {
        table->data[i] = fd_entry;
    }
}

int fdt_insert(fd_table_t *table, fd_entry_t entry)
{
    for (int i = 0; i < FD_TABLE_SIZE; i++)
    {
        if (table->data[i].inode == -1)
        {
            table->data[i] = entry;
            return i;
        }
    }
    return -1;
}

int fdt_remove(fd_table_t *table, int fileID)
{
    if (fileID < 0 || fileID >= FD_TABLE_SIZE)
    {
        return -1;
    }
    if (table->data[fileID].inode == -1)
    {
        return -1;
    }
    fd_entry_t fd_entry;
    fde_init(&fd_entry, -1, 0, 0);
    table->data[fileID] = fd_entry;
    return 0;
}

int fdt_lookup(fd_table_t *table, int inode)
{
    for (int i = 0; i < FD_TABLE_SIZE; i++)
    {
        if (table->data[i].inode == inode)
        {
            return i;
        }
    }
    return -1;
}