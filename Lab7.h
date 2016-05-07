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

 so what's up with the segment summary???
 -every segment has a segment summary (duh?)
 -8 bytes at the beginning of memory
 -there is a space for each block in the segment
 What goes in each "space"? (I say space cause I don't know how big 8 bytes really is)
 for each inode: <inode number, -1> (the -1 tags the pair as an inode)
 for each imap piece: <128, 0-39> (the 128 tags the pair as a piece of the imap, the 0-39 says which imap segment /40 it is)
 for each data member: <inode, logical block # (block position)>

 btw the segment summary needs to be built as each file is imported

 AND THEN: 
 1. the number of segments to clean are specified
 2. for each segment that is being used and is going to be cleaned:
 	create a copy of the segment and iterate through the map to decide what to copy over
 	if the logical block number (block position) associated with the data member points to the location in memory it says it does, 
 	then copy it over. If it's a -1 or something then don't.
 3. you should end up with less segments in the end
*/
