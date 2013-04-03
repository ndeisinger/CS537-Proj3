#include "util.h"

int r10_read(int block, int size, char * data);

int r10_write(int block, int size, const char * data);

int r10_recover(int disk); 

static inline int localAdd_r10(int block)
{
    int total = block;
    total /= BPS;
    total /= (info.numDisks/2);
    total *= BPS; // This looks really redundant, but it works thanks to integer division.
    total += (block % BPS);
    return total;
};
