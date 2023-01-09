// Alex Zhou
// 260970063

#define INODE_NUM_PTRS 12
#define INODE_TBL_SIZE 10

typedef struct _inode_t
{
    int mode;
    int link_cnt;
    int uid;
    int gid;
    int size;
    int ptrs[INODE_NUM_PTRS];
    int ind_ptr;
} inode_t;

#define NUM_INODES 1024 * INODE_TBL_SIZE / sizeof(inode_t)

typedef struct _inode_table_t
{
    inode_t data[NUM_INODES];
} inode_table_t;

void inode_init(inode_t *inode, int mode)
{
    inode->mode = mode;
    inode->size = 0;
    inode->ind_ptr = -1;
    for (int i = 0; i < INODE_NUM_PTRS; i++)
    {
        inode->ptrs[i] = -1;
    }
}
void inode_table_init(inode_table_t *tbl)
{
    for (int i = 0; i < NUM_INODES; i++)
    {
        inode_t inode;
        inode_init(&inode, -1);
        tbl->data[i] = inode;
    }
}

int inode_table_insert(inode_table_t *tbl, inode_t inode)
{
    for (int i = 0; i < NUM_INODES; i++)
    {
        if (tbl->data[i].mode == -1)
        {
            tbl->data[i] = inode;
            return i;
        }
    }
    return -1;
}
