#include "raid10.h"

//NOTE: Assume disks 0 and 1, 2 and 3, 4 and 5 etc. are paired.

int r10_read(int block, int size, char * data)
{
    char * loc = data;
    int disk = ((block/info.blocksPerStrip) * 2) % info.numDisks;
    int local_b = localAdd_r10(block);
    int i = 0;
    
    //each loop is one block
    for (i = 0; i < size; i++)
    {
        if(disk_array_read(da, disk, local_b, loc) == -1)
        {
            if (disk_array_read(da, disk + 1, local_b, loc) == -1) { printf("ERROR "); }
            else { printf("%i ", * (int *)loc); }
        }
        else { printf("%i ", * (int *)loc); }
        loc += BLOCK_SIZE;
        block++;
        disk = ((block/info.blocksPerStrip) * 2) % info.numDisks;
        local_b = localAdd_r10(block);
    }
    #ifdef DEBUG
    printf("Read up to block %i\n", i);
    #endif
    return 1;
}

int r10_write(int block, int size, const char * data)
{
    int write_err = 0;
    char * loc = (char * ) data;
    int disk = ((block/info.blocksPerStrip) * 2) % info.numDisks;
    int local_b = localAdd_r10(block);
    int i = 0;
    for (i = 0; i < size; i++)
    {
        if(disk_array_write(da, disk, local_b, loc) == -1) 
        {  
            if (disk_array_write(da, disk + 1, local_b, loc) == -1)
            {
                write_err = 1; 
            }
        } 
        else
        {
            disk_array_write(da, disk + 1, local_b, loc); //We don't care about failure of the second disk if the first worked.
        }
        loc += BLOCK_SIZE;
        block++;
        disk = ((block/info.blocksPerStrip) * 2) % info.numDisks;
        local_b = localAdd_r10(block);
    }
    #ifdef DEBUG
    printf("Wrote up to block %i\n", i);
    #endif
    if (write_err) { printf("ERROR "); }
    return 1;
}

int r10_recover(int disk)
{
    int copy_disk = 0;
    if (disk % 2 == 1) { copy_disk = disk - 1; }
    else { copy_disk =  disk + 1; }
    char buffer[BLOCK_SIZE];
    
    for (int i = 0; i < info.blocksPerDisk; i++)
    {
        if (disk_array_read(da, copy_disk, i, buffer) == -1 ) { printf("ERROR\n"); return -1; } 
        if (disk_array_write(da, disk, i, buffer) == -1 ) { printf("ERROR\n"); } //TODO: Do we print out an error in this case? It shouldn't ever happen, to be fair.
    }
    return 1;
}
