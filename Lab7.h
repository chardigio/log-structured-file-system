#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cmath>
#include <set>
#include <stdbool.h>

#define NO_SEGMENTS 32
#define SEG_SIZE 1048576 // 1MB
#define BLOCKS_IN_SEG 1024 // 1KB
#define BLOCK_SIZE 1024  // 1KB
#define IMAP_BLOCKS 40
#define MAX_FILESIZE 10
#define MAX_DATA_BLOCKS 128
#define MAX_FILES 10240 // 10K
#define SUMMARY_BLOCKS 8
#define CLEAN 0
#define DIRTY 1
#define ASSIGNABLE_BLOCKS 1016 // 1024 - 8
#define FILEMAP_BLOCK_SIZE 256 // first byte: 0/1 valid byte (0 means free, 1 means used),
                               // next 254: filename, last: null terminator

char INVALID[1] = {0};
char VALID[1] = {127}; // used to tag filenames in the filemap
unsigned int IMAP[IMAP_BLOCKS*BLOCK_SIZE]; // entire imap
char SEGMENT[SEG_SIZE]; // current segment in memory
unsigned int SEGMENT_NO = 1; // current segment 1-32
unsigned int AVAILABLE_BLOCK = 0; // next free block on current segment 0-1023
unsigned int SEGMENT_SUMMARY[BLOCKS_IN_SEG][2];
unsigned int CHECKPOINT_REGION[IMAP_BLOCKS];
char CLEAN_SEGMENTS[NO_SEGMENTS]; //0 = clean, 1 = not clean

typedef struct {
  char filename[FILEMAP_BLOCK_SIZE - 1];
  int size; // in true bytes rather than number of blocks
  int block_locations[MAX_DATA_BLOCKS]; // global block locations
} inode;
