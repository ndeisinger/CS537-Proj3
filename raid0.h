#include "util.h"
#define BPS info.blocksPerStrip

extern disk_array_t da;
extern diskInfo info;

static inline int localAdd(int block)
{
    int total = block;
    total /= BPS;
    total /= info.numDisks;
    total *= BPS; // This looks really redundant, but it works thanks to integer division.
    total += (block % BPS);
    return total;
};

int r0_read(int block, int size, char * data);

int r0_write(int block, int size, const char * data);

int r0_recover(int disk); //Will always fail
