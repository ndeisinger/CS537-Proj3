#include "raid0.h"

int r0_read(int block, int size, char * data)
{
    char * loc = data;
    int disk = (block/info.blocksPerStrip) % info.numDisks;
    int local_b = localAdd_r0(block);
    int i = 0;
    
    //each loop is one block
    for (i = 0; i < size; i++)
    {
        if(disk_array_read(da, disk, local_b, loc) == -1) { printf("ERROR "); } //couldn't read
        else { printf("%i ", * (int *)loc); }
        loc += BLOCK_SIZE;
        block++;
        disk = (block/info.blocksPerStrip) % info.numDisks;
        local_b = localAdd_r0(block);
    }
    #ifdef DEBUG
    printf("Read up to block %i\n", i);
    #endif
    return 1;
}

int r0_write(int block, int size, const char * data)
{
    int write_err = 0;
    char * loc = (char * ) data;
    int disk = (block/info.blocksPerStrip) % info.numDisks;
    int local_b = localAdd_r0(block);
    int i = 0;
    for (i = 0; i < size; i++)
    {
        if(disk_array_write(da, disk, local_b, loc) == -1) { write_err = 1; } //couldn't write
        loc += BLOCK_SIZE;
        block++;
        disk = (block/info.blocksPerStrip) % info.numDisks;
        local_b = localAdd_r0(block);
    }
    #ifdef DEBUG
    printf("Wrote up to block %i\n", i);
    #endif
    if (write_err) { printf("ERROR "); }
    return 1;
}

int r0_recover(int disk)
{
    return -1;
}
