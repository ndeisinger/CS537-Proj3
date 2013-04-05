#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include "disk-array.h"

#define BPS info.blocksPerStrip

typedef struct _diskInfo {
    int numDisks;
    int blocksPerDisk;
    int blocksPerStrip;
    int numStrips; //This is not provided; we calculate it.
    int level;
} diskInfo;

extern disk_array_t da;
extern diskInfo info;
extern int failedDisk;
