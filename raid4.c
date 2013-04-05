#include "raid4.h"
//Assume the final disk, eg. NUM_DISKS - 1, has the parity.
//When calculating our disk/local block address, we act as though we have 1 less disk than we do.
//#define DEBUG
//#define DEBUG_XOR
//#define DEBUG2 1
int r4_read(int block, int size, char * data)
{
    char * loc = data;
    int disk = (block/info.blocksPerStrip) % (NUM_DISKS - 1);
    int local_b = localAdd_r4(block);
    int i = 0;
    
    //each loop is one block
    for (i = 0; i < size; i++)
    {
            #ifdef DEBUG
                fprintf(stderr, "Read disk: %i\n", disk);
            #endif 
        if (disk_array_read(da, disk, local_b, loc) == -1)
        {
            //Need to deal with parity
            char par_temp[BLOCK_SIZE]; //Holds a block of data read from the disk
            char par_res[BLOCK_SIZE]; //Holds current result of parity calculation
            int failed_read = 0; //Set to true if we can't read a disk
            memset(par_res, 0, BLOCK_SIZE);
            for (int j = 0; j < NUM_DISKS; j++)
            {
                if (j != disk)
                {
                    if (disk_array_read(da, j, local_b, par_temp) == -1)
                    {
                        printf("ERROR ");
                        failed_read = 1;
                        j+= NUM_DISKS; //Break out of the for loop
                    }
                    else
                    {
                        for (int byte = 0; byte < BLOCK_SIZE; byte++)
                        {
                            par_res[byte] = par_temp[byte] ^ par_res[byte];
                        }
                    }
                }
            }
            //TODO: We could abstract the above code into a function for getting parity in general.
            if (!failed_read)
            {
                memcpy(loc, par_res, BLOCK_SIZE); // Copy the result of our parity calculation into the read buffer
            }
        }
        loc += BLOCK_SIZE;
        block++;
        disk = (block/info.blocksPerStrip) % (NUM_DISKS - 1);
        local_b = localAdd_r4(block);
    }
    printf("Read up to block %i\n", i);
    return 1;
}

//DEBUG: Print out contents of parity buffer.
void print_par_buf(char buf[NUM_DISKS][BLOCK_SIZE * BPS])
{
printf("---------------------------------------------------------\n");
    
    for (int j = 0; j < BLOCK_SIZE * BPS; j++)
    {
        for (int i = 0; i < NUM_DISKS; i++)
        {
            printf("[%x]", buf[i][j]);
        }
        printf("\n");
    }
printf("---------------------------------------------------------\n");
}

int r4_write(int block, int size, const char * data)
{
    char * loc = (char * ) data;
    int disk = (block/info.blocksPerStrip) % (NUM_DISKS - 1);
    int local_b = localAdd_r4(block);
    char par_buf[NUM_DISKS][BLOCK_SIZE * BPS]; 
    int rows[BPS];
    char par_val[BLOCK_SIZE];
    char par_temp[BLOCK_SIZE];
    
    memset(par_buf, -1, NUM_DISKS * BLOCK_SIZE * BPS);
    memset(rows, -1, sizeof(int) * BPS);
    memset(par_val, 0, BLOCK_SIZE);
    memset(par_temp, 0, BLOCK_SIZE);
    
    int s_disk = -1;
    int s_block = -1;
    int s_lba = -1;
    int s_index = -1; // Starting index we're modifying in our stripe
    
    int write_err = 0;
    int first_write = 1;
    
    for (int i = 0; i < size; i++)
    {
    #ifdef DEBUG
        printf("LBA: %i, local block: %i, disk: %i\n", block, local_b, disk);
    #endif
    
        if ((disk == 0 && (local_b % BPS == 0)) || first_write) 
        { 
            first_write = 0;
        
            // First block of a new stripe
            memset(par_buf, -1, NUM_DISKS * BLOCK_SIZE * BPS);
            memset(rows, -1, sizeof(int) * BPS);
            memset(par_val, 0, BLOCK_SIZE);
            memset(par_temp, 0, BLOCK_SIZE);
            
            s_disk = disk;
            s_block = local_b;
            s_lba = block;
            s_index = (disk * BPS) + (local_b % BPS);
        }
        
        rows[local_b % BPS] = 1;
        
        if ( ( (disk == (NUM_DISKS - 2)) && (local_b % BPS == (BPS - 1)) )
        || i == (size - 1))
        {
        #ifdef DEBUG
            printf("End of stripe - LBA: %i, local block: %i, disk: %i\n", block, local_b, disk);
        #endif
            // End of stripe or end of write request - time to write out data and parity
            
            int e_index = (disk * BPS) + (local_b % BPS); // Last index in the stripe we write to
            int phys_offset = local_b - (local_b % BPS); // Gets the base physical offset within the disk for this stripe
            
            // Copy data from given write buffer into our parity buffer
            int copy_size = BLOCK_SIZE * (block - s_lba + 1);
            memcpy(&par_buf[s_disk][(s_block % BPS) * BLOCK_SIZE], 
                   loc, 
                   copy_size);
            int curr_disk = 0;
            int curr_row = 0;
            for (curr_row = 0; curr_row < BPS; curr_row++)
            {
                memset(par_val, 0, BLOCK_SIZE);
                // Calculate parity and write for each 'row' - eg. one block from each disk
                if (rows[curr_row] == 1)
                {
                    // Something in this row was altered; need to calc parity/write
                    for (curr_disk = 0; curr_disk < NUM_DISKS - 1; curr_disk++)
                    {
                        int curr_index = (curr_disk * BPS) + curr_row;
                        // Attempt additive parity.
                        // Check if we need to load from disk
                        if ( (curr_index < s_index) || (curr_index > e_index) )
                        {
                            // We do; try loading
                            if (disk_array_read(da, curr_disk, phys_offset + curr_row, par_temp) == -1)
                            {
                                // Failed a read, attempt subtractive parity
                                #ifdef DEBUG_XOR
                                    printf("Failed read on disk %i, attempting subtractive parity\n", curr_disk);
                                #endif
                                int bad_disk = curr_disk;
                                memset(par_val, 0, BLOCK_SIZE);
                                // Loop through our disks and read where we plan to modify
                                for (curr_disk = 0; curr_disk < NUM_DISKS - 1; curr_disk++)
                                {
                                    // Check that we aren't trying to read off the bad disk
                                    if (!(curr_disk == bad_disk))
                                    {
                                        curr_index = (curr_disk * BPS) + curr_row;
                                        // Read from disk for our blocks we'll modify and XOR with memory
                                        if ( (curr_disk > s_index) &&  (curr_disk < e_index) )
                                        {
                                            // We'll modify this value, read its old value from disk for parity
                                            if (disk_array_read(da, curr_disk, phys_offset + curr_row, par_temp) == -1)
                                            {
                                                // TODO: We have two or more failed disks; abort?
                                                write_err = 1;
                                            }
                                            else
                                            {
                                                #ifdef DEBUG_XOR
                                                    printf("par_temp: %x, par_buf: %x", *par_temp, par_buf[curr_disk][curr_row * BLOCK_SIZE]);
                                                #endif
                                                
                                                // XOR with memory
                                                block_xor(par_temp, &par_buf[curr_disk][curr_row * BLOCK_SIZE]);
                                                
                                                #ifdef DEBUG_XOR
                                                    printf(", par_temp 2: %x, par_val: %x", *par_temp, *par_val);
                                                #endif
                                                
                                                // XOR with result thus far
                                                block_xor(par_val, par_temp);
                                                
                                                #ifdef DEBUG_XOR
                                                    printf(", par_val 2: %x\n", *par_val);
                                                #endif
                                                //memset(par_temp, 0, BLOCK_SIZE);
                                            }
                                        }
                                    }
                                }
                                // Read in old parity value and XOR with that
                                if (disk_array_read(da, NUM_DISKS - 1, phys_offset + curr_row, par_temp) == -1)
                                {
                                    // TODO: We have two or more failed disks; abort?
                                    write_err = 1;
                                }
                                else
                                {
                                    #ifdef DEBUG_XOR
                                        printf("curr_disk: %i, curr_row: %i, par_temp: %x, par_val: %x", curr_disk, curr_row, *par_temp, *par_val);
                                    #endif
                                    
                                    block_xor(par_val, par_temp);
                                    
                                    #ifdef DEBUG_XOR
                                        printf(", par_val 2: %x\n", *par_val);
                                    #endif
                                }                                
                                curr_disk += NUM_DISKS; // Break out of the outer disk loop
                            }
                            else
                            {
                                // Successfully loaded from disk, XOR with result so far
                        
                                #ifdef DEBUG_XOR
                                    printf("curr_disk: %i, curr_row: %i, par_temp: %x, par_val: %x", curr_disk, curr_row, *par_temp, *par_val);
                                #endif
                            
                                block_xor(par_val, par_temp);      
                                                      
                            #ifdef DEBUG_XOR
                                printf(", par_val 2: %x\n", *par_val);
                            #endif
                            }
                        }
                        else
                        {
                            // We can read our par_temp from memory.
                            memcpy(par_temp, &par_buf[curr_disk][curr_row * BLOCK_SIZE], BLOCK_SIZE);
                            #ifdef DEBUG_XOR
                                printf("par_temp: %x, par_val: %x", *par_temp, *par_val);
                            #endif
                            
                            //XOR with our result so far
                            block_xor(par_val, par_temp);
                            
                            #ifdef DEBUG_XOR
                                printf(", par_val 2: %x\n", *par_val);
                            #endif
                        }
                    }
                    // We now have the parity value we want in par_val.  Write it out to disk.
                    // If we reach here, it's guaranteed we have one or zero failed disks, 
                    // OR that we already marked a write error.
                    if (disk_array_write(da, NUM_DISKS - 1, phys_offset + curr_row, par_val) == -1)
                    {
                        write_err = 1;
                    }
                    #ifdef DEBUG
                        printf("Writing parity %x out to disk %i\n", *par_val, NUM_DISKS - 1);
                    #endif
                }              
                // Write modified data out to disk
                for (curr_disk = 0; curr_disk < NUM_DISKS - 1; curr_disk++)
                {
                    int curr_index = (curr_disk * BPS) + curr_row;
                    #ifdef DEBUG
                        printf("curr_disk: %i, curr_index: %i\n", curr_disk, curr_index);
                    #endif
                    // Check if we're going to write out this block
                    if ( (curr_index >= s_index) &&  (curr_index <= e_index) )
                    {
                        // Write out the block to disk
                        disk_array_write(da, curr_disk, phys_offset + curr_row, &par_buf[curr_disk][curr_row * BLOCK_SIZE]);
                    #ifdef DEBUG
                        printf("Writing value beginning with %x out to disk %i, block %i, ", par_buf[curr_disk][curr_row * BLOCK_SIZE], curr_disk, phys_offset + curr_row);
                        printf("curr_row: %i, index %i, s_index %i, e_index %i\n", curr_row, curr_index, s_index, e_index);
                    #endif
                    }
                }
            }
            
            #ifdef DEBUG2
            print_par_buf(par_buf);
            #endif
            
            // Advance pointer in our data     
            loc += (block - s_lba + 1) * (BLOCK_SIZE);
        }
        block++;
        disk = (block/info.blocksPerStrip) % (NUM_DISKS - 1);
        local_b = localAdd_r4(block);
    }
    if (write_err) { printf("ERROR "); }
    return 0;
}

void block_xor(char * dst, char * oper)
{
    // Performs an xor across a block of data.
    for (int byte = 0; byte < BLOCK_SIZE; byte++)
    {
    #ifdef DEBUG
    //printf("dst: %x, oper: %x, res: %x\n", dst[byte], oper[byte], dst[byte] ^ oper[byte]);
    #endif
        dst[byte] = dst[byte] ^ oper[byte];
    }
}

int r4_recover(int disk) {
	int numberDisks = info.numDisks;
	int numberBlocks = info.blocksPerDisk;

	//array to store data/parity to restore failed disk 
	char data[numberDisks-1][BLOCK_SIZE];
	memset(data, 0, ((numberDisks - 1) * BLOCK_SIZE));

	int row;
	//go through each row/physical block in a disk
	for(row = 0; row < numberBlocks; row++) {
		int col;
		//go through and get the data for each disk for the current offset
		int i = 0; //used to place blocks into data[]
		for(col = 0; col < numberDisks; col++) {
			//check if current column/disk is the failed one, if not get data
			if(col != disk) {
				disk_array_read(da, col, row, data[i]);
				#ifdef DEBUG_XOR
				printf("Value at data[%i]: %x\n", i, *data[i]);
				#endif
				i++;
			}
		}
		//now compute restored block
		for(col = 1; col < numberDisks-1; col++) {
			block_xor(data[0], data[col]);
		}
		#ifdef DEBUG_XOR
		printf("Calculated restore val: %x\n", data[0][0]);
		#endif
		disk_array_write(da, disk, row, data[0]);
	}

    	return 1;
}







