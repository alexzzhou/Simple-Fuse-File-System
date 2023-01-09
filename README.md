Alex Zhou
260970063

Use make to compile. 

    make
    ./sfs temp

First time compiling with fuse_wrap_new.c causes a warning. sfs should still be 
compiled. 

The file system is split up into two sections:

The implementation, in sfs_api.c and sfs_api.h

and support files, found under types:
    types/directory.h
    types/fd_table.h
    types/inode_table.h

The support files are headers that contain relevant functions for easily setting up 
the data structures of the file system. The functions are relatively simple, only 
really used to initialize, insert, remove, or lookup an entry of each structure.

The types folder should be located in the same folder as sfs_api.c and sfs_api.h. 

No changes were made to:
    fuse_wrap_new.c
    disk_emu.h
    disk_emu.c
    Makefile
