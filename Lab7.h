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
#include "commands.cpp"

#define NO_SEGMENTS 32
#define SEG_SIZE 1048576 // 1MB
#define BLOCK_SIZE 1024  // 1KB
#define IMAP_BLOCKS 80
/*
Lab 7:
 - import
 - remove
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
