#include "util.h"
#define NUM_DISKS info.numDisks

int r4_read(int block, int size, char * data);

int r4_write(int block, int size, const char * data);

int r4_recover(int disk); 

static int failedDisk;

static inline int localAdd_r4(int block)
{
    int total = block;
    total /= BPS;
    total /= ((NUM_DISKS - 1)/2);
    total *= BPS; // This looks really redundant, but it works thanks to integer division.
    total += (block % BPS);
    return total;
};
