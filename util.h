#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include "disk-array.h"


typedef struct _diskInfo {
    int numDisks;
    int blocksPerDisk;
    int blocksPerStrip;
    int numStrips; //This is not provided; we calculate it.
    int level;
} diskInfo;
