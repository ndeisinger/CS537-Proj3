#include "util.h"
#include "raid0.h"

int verbose;
int (*read_f)(int, int, int, char *);
int (*write_f)(int, int, int, const char *);
int (*recover_f)(int);
#define DEBUG 1;

int main(int argc, char * argv[])
{
    verbose = 0;
    diskInfo info = {-1, -1, -1, -1, -1};
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
    
    FILE * tracef;
    tracef = fopen(tracefile, "r");
    if (tracef == NULL)
    {
        fprintf(stderr, "Error: Could not open file %s", tracefile);
    }
    
    char buffer[200]; //I certainly hope we don't have any lines longer than 200 characters!
    
    disk_array_t da = disk_array_create("virtual disk array", info.numDisks, info.blocksPerDisk);
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
        break;
        default:
        break;
    }
    
    while (fgets(buffer, 200, tracef))
    {
    #ifdef DEBUG
        printf("Executing command: %s", buffer);
    #endif
        if (strncmp(buffer, "READ", 4) == 0)
        {
            //read data
        }
        else if (strncmp(buffer, "WRITE", 5) == 0)
        {
            //write data
        }
        else if (strncmp(buffer, "FAIL", 4) == 0)
        {
            //fail a disk
            int disk = atoi(&buffer[6]);
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
            int disk = atoi(&buffer[9]);
            if (disk != -1 && disk > 0 && disk <= info.numDisks) //TODO: Could inline this to clean things up
            {
                disk_array_recover_disk(da, disk);
            }
            else
            {
                fprintf(stderr, "Invalid disk specified for failure!\n");
            }
        }
        else
        {
            fprintf(stderr, "Bad input: %s", buffer);
            fclose(tracef);
            exit(1);
        }
    }    
    #ifdef DEBUG
        disk_array_print_stats(da);
    #endif
    fclose(tracef);
    return 0;
}
