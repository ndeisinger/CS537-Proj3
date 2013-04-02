#include "util.h"
#include "raid0.h"

int verbose;
diskInfo info; //command line info
disk_array_t da; //disk array

//pointers to generic functions: set to specific functions depending on raid level
int (*read_f)(int, int, char *);
int (*write_f)(int, int, const char *);
int (*recover_f)(int);


#define DEBUG 1;

int main(int argc, char * argv[])
{
    verbose = 0;
    info.numDisks = -1;
    info.blocksPerDisk = -1;
    info.blocksPerStrip = -1;
    info.numStrips = -1;
    info.level = -1;
    char * tracefile = NULL;

    struct option long_options[] = 
    {
        {"level", required_argument, NULL, 'a'},
        {"strip", required_argument, NULL, 'b'},
        {"disks", required_argument, NULL, 'c'},
        {"size", required_argument, NULL, 'd'},
        {"trace", required_argument, NULL, 'e'},
        {"verbose", no_argument, &verbose, 1},
        {0, 0, 0, 0}
    };
    //parsing command line
    int option_index = 0;
    int opt;
    while ((opt = getopt_long_only(argc, argv, "", long_options, &option_index)) != -1 )
    {
        switch(opt)
        {
        case 0:
            break;
        case 'a':
            info.level = atoi(optarg);
            break;
        case 'b':
            info.blocksPerStrip = atoi(optarg);
            break;
        case 'c':
            info.numDisks = atoi(optarg);
            break;
        case 'd':
            info.blocksPerDisk = atoi(optarg);
            break;
        case 'e':
            if((tracefile = (char *) malloc(strlen(optarg) + 1)))
            {
                strcpy(tracefile, optarg);
                break;
            }
            else
            {
                fprintf(stderr, "Error: Could not allocate memory for filename!\n");
                exit(1);
            }
        case '?':
            //Getopt should have already printed an error for us.
            exit(1);
        default:
            fprintf(stderr, "Getopt failed.  Congratulations? How did you even manage that?\n");
            exit(1);
        }
    }
    //end parse: getopt.h


    //checking command line arguments
    if (info.numDisks <= 0 || (info.level == 10 && info.numDisks %2 != 0) || (info.level >= 4 && info.numDisks <= 2))
    {
        fprintf(stderr, "Error: invalid number of disks\n");
        exit(1);
    }
    if (!(info.level == 0 || info.level == 10 || info.level == 4 || info.level == 5))
    {
        fprintf(stderr, "Error: invalid RAID level specified\n");
        exit(1);
    }
    if (info.blocksPerStrip <= 0)
    {
        fprintf(stderr, "Error: invalid number of blocks per strip\n");
        exit(1);
    }
    if (info.blocksPerDisk <= 0)
    {
        fprintf(stderr, "Error: invalid number of blocks per disk\n");
        exit(1);
    }
    if (tracefile == NULL)
    {
        fprintf(stderr, "Error: No tracefile provided.\n");
        //This should have been caught above, but just to be safe.
    }
    //end checking
    

    //open trace file
    FILE * tracef;
    tracef = fopen(tracefile, "r");
    if (tracef == NULL)
    {
        fprintf(stderr, "Error: Could not open file %s", tracefile);
    }
    
    char buffer[200]; //I certainly hope we don't have any lines longer than 200 characters!
    
    da = disk_array_create("virtual disk array", info.numDisks, info.blocksPerDisk);
    if (!da)
    {
        fprintf(stderr, "Could not create disk array - %s\n", strerror(errno));
        exit(1);
    }
    








    //Set up our functions with the proper pointers
    switch(info.level)
    {
        case(0):
            read_f = r0_read;
            write_f = r0_write;
            recover_f = r0_recover;
        break;
        default:
        break;
    }
    
    //loops through each line in the trace file
    //parsing the line and making calls 
    
    //This buffer is made for use with the commands WRITE <LBA> 5 <VAL> and READ <LBA> 5.
    //We write to here as well as the write buffer so we can compare the data we read back.
    #ifdef DEBUG
        char knownval[BLOCK_SIZE * 5];
        memset(knownval, 0, BLOCK_SIZE * 5);
    #endif
    
    
    while (fgets(buffer, 200, tracef))
    {
    #ifdef DEBUG
        printf("Executing command: %s", buffer);
    #endif
        if (strncmp(buffer, "READ", 4) == 0)
        {
            //read data
            //TODO: Change this to strtok to match  below.
            int lba = atoi(&buffer[5]);
            int size = atoi(strrchr(buffer, ' ')); //This gets us the address of the last space, eg. the second number.
            fprintf(stderr, "lba: %i, size: %i\n", lba, size);
            char readbuf[BLOCK_SIZE * size]; //TODO: Not efficient to repeatedly allocate/deallocate arrays.
            read_f(lba, size, readbuf);
            #ifdef DEBUG
            // Ensure that the known value we wrote matches what we read back.
                for (int i = 0; i < BLOCK_SIZE * size; i++)
                {
                    if (knownval[i] != readbuf[i])
                    {
                        printf("Diverging index: %i\n", i);
                        //exit(1);
                    }
                    printf("knownVal: %c\n", knownval[i]);
                    printf("read val: %c\n", readbuf[i]);
                }
            #endif
        }
        else if (strncmp(buffer, "WRITE", 5) == 0)
        {
            //write data
            
            //parse line
            strtok(buffer, " ");
            int lba = atoi(strtok(NULL, " "));
            int size = atoi(strtok(NULL, " "));
            char val[4];
            //Copy 4 bytes from tracefile into a buffer
            memcpy(val, strtok(NULL, " "), 4*sizeof(char));
            //make repeated array of that value for writing
            char writebuf[BLOCK_SIZE * size];
            //TODO: NOTE: We could use a buffer of just one block, repeated.
            for (int i = 0; i < BLOCK_SIZE * size; i++)
            {
                writebuf[i] = val[i % 4]; //TODO: Could be more efficient
            #ifdef DEBUG //This is just for testing read, no effect on actual execution
                knownval[i] = val[i % 4];
            #endif
            }
            #ifdef DEBUG
                printf("lba: %i, size: %i, val: %i\n", lba, size, (int) *val);
            #endif
            write_f(lba, size, writebuf);
        }
        else if (strncmp(buffer, "FAIL", 4) == 0)
        {
            //fail a disk
            int disk = atoi(&buffer[5]);
            if (disk != -1)
            {
                disk_array_fail_disk(da, disk);
            }
            else
            {
                fprintf(stderr, "Invalid disk specified for failure!\n");
            }
        }
        else if (strncmp(buffer, "RECOVER", 7) == 0)
        {
            //recover a disk as blank
            int disk = atoi(&buffer[8]);
            if (disk != -1 && disk > 0 && disk <= info.numDisks) //TODO: Could inline this to clean things up
            {
                disk_array_recover_disk(da, disk);
            }
            else
            {
                fprintf(stderr, "Invalid disk specified for failure!\n");
            }
        }
	else if (strncmp(buffer, "END", 3) == 0) {
		//last command in trace
		break;
	}
        else
        {
            fprintf(stderr, "Bad input: %s", buffer);
            fclose(tracef);
            exit(1);
        }
    }    
    
    disk_array_print_stats(da);
    
    fclose(tracef);
    free(tracefile);
    return 0;
}
