// Alex Zhou
// 260970063

#define DIR_TABLE_SIZE 64
#define DIR_TABLE_BYTE_SIZE 1000
#define MAX_FILENAME 64 - 2 * sizeof(int)

typedef struct _dir_entry_t
{
    char filename[MAX_FILENAME];
    int free; // 0 = free, 1 = used
    int inode;
} dir_entry_t;

typedef struct _dir_table_t
{
    dir_entry_t data[DIR_TABLE_BYTE_SIZE];
    int index;
} dir_table_t;

void dir_entry_init(dir_entry_t *entry, char filename[], int inode, int free)
{
    memset(entry, 0, sizeof(dir_entry_t));
    memcpy(entry->filename, filename, strlen(filename));
    entry->inode = inode;
    entry->free = free;
}

void dir_table_init(dir_table_t *table)
{
    table->index = 0;

    // init all entries to free;
    for (int i = 0; i < DIR_TABLE_BYTE_SIZE; i++)
    {
        dir_entry_t entry;
        dir_entry_init(&entry, "", 0, 0);
        table->data[i] = entry;
    }
}

// inserts file into directory. Returns index in table on success, -1 on fail.
int dir_table_insert(dir_table_t *table, dir_entry_t entry)
{
    for (int i = 0; i < DIR_TABLE_BYTE_SIZE; i++)
    {
        if (table->data[i].free == 0)
        {
            table->data[i] = entry;
            return i;
        }
    }
    return -1;
}

// looks for file with filename in dir_table. Returns entry on success, NULL on fail.
dir_entry_t *dir_lookup_fname(dir_table_t *table, char *filename)
{

    for (int i = 0; i < DIR_TABLE_BYTE_SIZE; i++)
    {
        if (strlen(filename) != strlen(table->data[i].filename))
        {
            continue;
        }
        else if (memcmp(table->data[i].filename, filename, strlen(filename)) == 0)
        {
            return &(table->data[i]);
        }
    }
    return NULL;
}
