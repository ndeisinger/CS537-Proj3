#include "raid0.h"
extern disk_array_t da;

int r0_read(int disk, int block, int size, char * data)
{
    return 0;
}

int r0_write(int disk, int block, int size, const char * data)
{
    return 0;
}

int r0_recover(int disk)
{
    return -1;
}
