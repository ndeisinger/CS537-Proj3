#include "util.h"

int r0_read(int disk, int block, int size, char * data);

int r0_write(int disk, int block, int size, const char * data);

int r0_recover(int disk); //Will always fail
