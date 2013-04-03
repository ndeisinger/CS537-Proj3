#include "raid4.h"
//Assume the final disk, eg. NUM_DISKS - 1, has the parity.
//When calculating our disk/local block address, we act as though we have 1 less disk than we do.

int r4_read(int block, int size, char * data)
{
    char * loc = data;
    int disk = ((block/info.blocksPerStrip) * 2) % (NUM_DISKS - 1);
    int local_b = localAdd_r4(block);
    int i = 0;
    
    //each loop is one block
    for (i = 0; i < size; i++)
    {
        if (disk == failedDisk)
        {
            //Need to deal with parity
            char par_temp[BLOCK_SIZE]; //Holds a block of data read from the disk
            char par_res[BLOCK_SIZE]; //Holds current result of parity calculation
            memset(par_res, 0, BLOCK_SIZE);
            for (int j = 0; j < NUM_DISKS; j++)
            {
                if (j != failedDisk)
                {
                    disk_array_read(da, j, local_b, par_temp);
                    for (int byte = 0; byte < BLOCK_SIZE; byte++)
                    {
                        par_res[byte] = par_temp[byte] ^ par_res[byte];
                    }
                }
            }
            //TODO: We could abstract the above code into a function for getting parity in general.
            memcpy(loc, par_res, BLOCK_SIZE); // Copy the result of our parity calculation into the read buffer
        }
        else
        {
            if(disk_array_read(da, disk, local_b, loc) == -1)
            {
                if (disk_array_read(da, disk + 1, local_b, loc) == -1)  { strncpy(loc, "ERROR", 5); }
            }
        }
        loc += BLOCK_SIZE;
        block++;
        disk = ((block/info.blocksPerStrip) * 2) % (NUM_DISKS - 1);
        local_b = localAdd_r4(block);
    }
    printf("Read up to block %i\n", i);
    return 1;
}

//NOTE: THE BELOW FUNCTIONS ARE NOT YET RAID4; THEY HAVE BEEN COPIED FROM RAID10
int r4_write(int block, int size, const char * data)
{
    char * loc = (char * ) data;
    int disk = ((block/info.blocksPerStrip) * 2) % info.numDisks;
    int local_b = localAdd_r4(block);
    int i = 0;
    for (i = 0; i < size; i++)
    {
        if(disk_array_write(da, disk, local_b, loc) == -1) 
        {  
            if(disk_array_write(da, disk + 1, local_b, loc) == -1) { printf("ERROR\n"); } 
        } 
        else disk_array_write(da, disk + 1, local_b, loc); //We don't care about failure of the second disk if the first worked.
        loc += BLOCK_SIZE;
        block++;
        disk = ((block/info.blocksPerStrip) * 2) % info.numDisks;
        local_b = localAdd_r4(block);
    }
    printf("Wrote up to block %i\n", i);
    return 1;
}

int r4_recover(int disk)
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
