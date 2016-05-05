#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cmath>
#include <stdbool.h>

#define NO_SEGMENTS 32
#define SEG_SIZE 1048576 // 1MB
#define BLOCK_SIZE 1024  // 1KB
#define IMAP_BLOCKS 40

unsigned int IMAP[IMAP_BLOCKS*BLOCK_SIZE];
unsigned int SEGMENT[BLOCK_SIZE][BLOCK_SIZE];
unsigned int SEGMENT_NO = 1;

/*
Lab 7:
 - import
 - remove (BUG)
 - list
 - exit
 - restart (read in the files and list them after
            running the file system process again)
           (not a command)
 - note: assume importing files does not fill disk

Program 2:
 - cat
 - display
 - overwrite
 - segment cleaning
*/
