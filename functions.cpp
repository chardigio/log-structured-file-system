#include "Lab7.h"

std::vector<std::string> split(const std::string &str) {
  std::string buf;
  std::stringstream ss(str);
  std::vector<std::string> tokens;
  int no_tokens = 0;
  while (ss >> buf && no_tokens++ < 128+2) tokens.push_back(buf);
  return tokens;
}

void initSegmentSummary(){
  for (int i = 0; i < BLOCKS_IN_SEG; ++i) {
    for (int j = 0; j < 2; ++j){
      SEGMENT_SUMMARY[i][j] = (unsigned int) -1;
    }
  }
}

void initSegmentSummary(unsigned int** summary){
  for (int i = 0; i < BLOCKS_IN_SEG; ++i) {
    for (int j = 0; j < 2; ++j){
      summary[i][j] = (unsigned int) -1;
    }
  }
}

void readInCheckpointRegion(){
  std::fstream cpr;
  cpr.open("DRIVE/CHECKPOINT_REGION", std::ios::binary | std::ios::out | std::ios::in);

  char address_str[4];
  unsigned int address_int = 0;

  for (int i = 0; i < IMAP_BLOCKS; ++i){
    cpr.read(address_str, 4);
    std::memcpy(&CHECKPOINT_REGION[i], address_str, 4);
  }

  cpr.read(CLEAN_SEGMENTS, NO_SEGMENTS);

  cpr.close();
}

void findNextAvailableBlock(){
  bool at_least_one_imap_piece = false;
  unsigned int most_recent_imap_pos = 0;
  for (int i = 0; i < IMAP_BLOCKS; ++i){
    if (CHECKPOINT_REGION[i] != (unsigned int) -1 && CHECKPOINT_REGION[i] >= most_recent_imap_pos){
      most_recent_imap_pos = CHECKPOINT_REGION[i];
      at_least_one_imap_piece = true;
    }
  }

  AVAILABLE_BLOCK = (at_least_one_imap_piece) ? (most_recent_imap_pos % BLOCK_SIZE) + 1 : 0;
  SEGMENT_NO = 1 + most_recent_imap_pos / BLOCK_SIZE;
}

void readInSegment(){
  std::fstream segment_file("DRIVE/SEGMENT" + std::to_string(SEGMENT_NO), std::fstream::binary | std::ios::in);

  segment_file.read(SEGMENT, ASSIGNABLE_BLOCKS * BLOCK_SIZE);

  char buffer[SUMMARY_BLOCKS * BLOCK_SIZE];
  segment_file.read(buffer, SUMMARY_BLOCKS * BLOCK_SIZE);
  std::memcpy(&SEGMENT_SUMMARY, buffer, SUMMARY_BLOCKS * BLOCK_SIZE);

  segment_file.close();
}

void readInImapBlock(unsigned int address, unsigned int fragment_no){
  // only works if the segment in memory has been written to drive
  unsigned int segment_no = (address / BLOCKS_IN_SEG) + 1;
  unsigned int block_start_pos = (address % BLOCKS_IN_SEG) * BLOCK_SIZE;
  std::fstream segment_file("DRIVE/SEGMENT" + std::to_string(segment_no), std::fstream::binary | std::ios::in | std::ios::out);

  segment_file.seekg(block_start_pos);
  char buffer[BLOCK_SIZE];
  segment_file.read(buffer, BLOCK_SIZE);
  std::memcpy(&IMAP[fragment_no*BLOCK_SIZE], buffer, BLOCK_SIZE);

  segment_file.close();
}

void readInImap(){
  for (unsigned int i = 0; i < IMAP_BLOCKS; ++i){
    if (CHECKPOINT_REGION[i] != (unsigned int) -1)
      readInImapBlock(CHECKPOINT_REGION[i], i);
  }
}

void writeOutSegment(){
  std::fstream segment_file("DRIVE/SEGMENT"+std::to_string(SEGMENT_NO), std::fstream::binary | std::ios::out);

  segment_file.write(SEGMENT, ASSIGNABLE_BLOCKS * BLOCK_SIZE);
  segment_file.write(reinterpret_cast<const char*>(&SEGMENT_SUMMARY), SUMMARY_BLOCKS * BLOCK_SIZE);

  SEGMENT_NO++;
  AVAILABLE_BLOCK = 0;

  segment_file.close();
}

void writeOutCheckpointRegion(){
  std::fstream cpr("DRIVE/CHECKPOINT_REGION", std::fstream::binary | std::ios::out);

  cpr.write(reinterpret_cast<const char*>(&CHECKPOINT_REGION), IMAP_BLOCKS * 4);
  cpr.write(CLEAN_SEGMENTS, NO_SEGMENTS);

  cpr.close();
}

unsigned int nextInodeNumber(){
  std::ifstream filemap("DRIVE/FILEMAP");

  for (int i = 0; i < MAX_FILES; ++i){
    filemap.seekg(i*FILEMAP_BLOCK_SIZE);
    char valid[1];
    filemap.read(valid, 1);

    if (!valid[0]) {
      filemap.close();
      return i;
    }
  }

  filemap.close();

  return -1;
}

void updateFilemap(unsigned int inode_number, std::string lfs_filename){
  std::fstream filemap("DRIVE/FILEMAP", std::fstream::binary | std::ios::out | std::ios::in);

  filemap.seekp(inode_number * FILEMAP_BLOCK_SIZE);
  filemap.write(VALID, 1);
  filemap.write(lfs_filename.c_str(), lfs_filename.length()+1);

  filemap.close();
}

void writeInode(const inode& node, unsigned int inode_number){
  SEGMENT_SUMMARY[AVAILABLE_BLOCK][0] =  inode_number;
  SEGMENT_SUMMARY[AVAILABLE_BLOCK][1] = (unsigned int) -1;

  //write that inode to the next BLOCK
  std::memcpy(&SEGMENT[AVAILABLE_BLOCK*BLOCK_SIZE], &node, sizeof(inode));
  AVAILABLE_BLOCK++;
}

void updateImap(unsigned int inode_number, unsigned int block_position){
  if (AVAILABLE_BLOCK == BLOCKS_IN_SEG)
    writeOutSegment();

  IMAP[inode_number] = block_position;

  unsigned int fragment_no = inode_number / BLOCKS_IN_SEG;

  std::memcpy(&SEGMENT[AVAILABLE_BLOCK * BLOCK_SIZE], &IMAP[fragment_no * BLOCK_SIZE], BLOCK_SIZE);

  SEGMENT_SUMMARY[AVAILABLE_BLOCK][0] = -1;
  SEGMENT_SUMMARY[AVAILABLE_BLOCK][1] = fragment_no;

  CHECKPOINT_REGION[fragment_no] = AVAILABLE_BLOCK + (SEGMENT_NO - 1) * BLOCKS_IN_SEG;

  CLEAN_SEGMENTS[SEGMENT_NO - 1] = DIRTY;

  AVAILABLE_BLOCK++;
}

int getFileSize(int inode_number){
  unsigned int block_position = IMAP[inode_number];
  unsigned int segment_location = block_position/BLOCKS_IN_SEG + 1;
  unsigned int local_block_pos = (block_position % BLOCKS_IN_SEG) * BLOCK_SIZE;
  if(block_position == -1){
    //the node doesn't exist
    //this should never get here and would be an error on our part
    std::cout << "Lethal error dude slow your roll." << std::endl;
  }

  inode meta;
  if(SEGMENT_NO == segment_location){
    std::memcpy(&meta, &SEGMENT[local_block_pos], sizeof(inode));
  }else{
    std::fstream disk_segment("DRIVE/SEGMENT" + std::to_string(segment_location), std::ios::binary | std::ios::in);

    disk_segment.seekg(local_block_pos);
    char buffer[sizeof(inode)];
    disk_segment.read(buffer, sizeof(inode));
    std::memcpy(&meta, buffer, sizeof(inode));

    disk_segment.close();
  }

  return meta.size;
}

unsigned int getInodeNumberOfFile(std::string lfs_filename){
  std::fstream filemap("DRIVE/FILEMAP", std::ios::binary | std::ios::in | std::ios::out);

  for (unsigned int i = 0; i < MAX_FILES; ++i){
    filemap.seekg(i*FILEMAP_BLOCK_SIZE);
    char valid[1];
    filemap.read(valid, 1);

    if (valid[0]) {
      char filename_buffer[FILEMAP_BLOCK_SIZE-1];
      filemap.read(filename_buffer, FILEMAP_BLOCK_SIZE-1);

      std::string filename(filename_buffer);
      if (filename == lfs_filename){
        filemap.close();
        return i;
      }
    }
  }

  filemap.close();

  return (unsigned int) -1;
}

void printBlock(unsigned int global_block_pos, unsigned int start_byte, unsigned int end_byte, bool first_block, bool last_block){
  unsigned int segment_no = (global_block_pos / BLOCKS_IN_SEG) + 1;
  unsigned int local_block_pos = (global_block_pos % BLOCKS_IN_SEG) * BLOCK_SIZE;
  if (first_block) local_block_pos += start_byte;

  unsigned int buffer_size;
  if (first_block && last_block) buffer_size = end_byte - start_byte;
  else if (last_block) buffer_size = end_byte % BLOCK_SIZE;
  else if (first_block) buffer_size = BLOCK_SIZE - start_byte;
  else buffer_size = BLOCK_SIZE;

  char buffer[buffer_size];

  if (segment_no != SEGMENT_NO){
    std::fstream seg_file("DRIVE/SEGMENT"+std::to_string(segment_no), std::ios::binary | std::ios::in);

    seg_file.seekg(local_block_pos);
    seg_file.read(buffer, buffer_size);

    seg_file.close();
  }else{
    std::memcpy(buffer, &SEGMENT[local_block_pos], buffer_size);
  }

  for (int i = 0; i < buffer_size; ++i)
    printf("%c", buffer[i]);

  if (last_block) printf("\n");

  /*
  printf("local_block_pos: %u\n", local_block_pos);
  printf("buffer_size: %u\n", buffer_size);
  printf("start_byte: %u\n", start_byte);
  printf("end_byte: %u\n", end_byte);
  */
}

inode getInode(unsigned int inode_number){
  unsigned int global_block_pos = IMAP[inode_number];
  unsigned int segment_no = (global_block_pos / BLOCKS_IN_SEG) + 1;
  unsigned int local_block_pos = (global_block_pos % BLOCKS_IN_SEG) * BLOCK_SIZE;

  inode meta;

  if (segment_no != SEGMENT_NO){
    std::fstream segment_file("DRIVE/SEGMENT"+std::to_string(segment_no), std::ios::binary | std::ios::in);

    segment_file.seekg(local_block_pos);
    char buffer[BLOCK_SIZE];
    segment_file.read(buffer, BLOCK_SIZE);

    std::memcpy(&meta, buffer, sizeof(inode));

    segment_file.close();
  }else{
    std::memcpy(&meta, &SEGMENT[local_block_pos], sizeof(inode));
  }

  return meta;
}

void writeCleanSegment(unsigned int**& clean_summary, char*& clean_segment, unsigned int& next_available_block_clean, int& clean_segment_no, std::vector<inode>& inodes, std::set<int>& fragments){
  for (int i = 0; i < inodes.size(); ++i) {
    std::memcpy(&clean_segment[next_available_block_clean * BLOCK_SIZE], &inodes[i], sizeof(inode));

    int inode_number = getInodeNumberOfFile(inodes[i].filename);

    clean_summary[next_available_block_clean][0] = inode_number;
    clean_summary[next_available_block_clean][1] = -1;

    IMAP[inode_number] = (clean_segment_no - 1) * BLOCKS_IN_SEG + next_available_block_clean;

    next_available_block_clean++;
  }

  for (auto fragment_no: fragments){
    std::memcpy(&clean_segment[next_available_block_clean * BLOCK_SIZE], &IMAP[fragment_no * BLOCK_SIZE], BLOCK_SIZE);

    clean_summary[next_available_block_clean][0] = -1;
    clean_summary[next_available_block_clean][1] = fragment_no;

    CHECKPOINT_REGION[fragment_no] = next_available_block_clean + (clean_segment_no - 1) * BLOCKS_IN_SEG;

    next_available_block_clean++;
  }

  std::fstream segment_file("DRIVE/SEGMENT"+std::to_string(clean_segment_no), std::fstream::binary | std::ios::out);
  segment_file.write(clean_segment, ASSIGNABLE_BLOCKS * BLOCK_SIZE);
  segment_file.write(reinterpret_cast<const char*>(&clean_summary), SUMMARY_BLOCKS * BLOCK_SIZE);
  segment_file.close();

  fragments.clear();
  inodes.clear();
  CLEAN_SEGMENTS[clean_segment_no - 1] = DIRTY;
  next_available_block_clean = 0;
  initSegmentSummary(clean_summary);
  clean_segment_no++;
}

void cleanSegment(int dirty_segment_no, unsigned int**& clean_summary, char*& clean_segment, unsigned int& next_available_block_clean, int& clean_segment_no, std::vector<inode>& inodes, std::set<int>& fragments){
  // import dirty segment into memory
  unsigned int dirty_summary[BLOCKS_IN_SEG][2];
  char dirty_segment[ASSIGNABLE_BLOCKS * BLOCK_SIZE];
  if (dirty_segment_no == SEGMENT_NO){
    std::memcpy(dirty_summary, SEGMENT_SUMMARY, SUMMARY_BLOCKS * BLOCK_SIZE);
    std::memcpy(dirty_segment, SEGMENT, ASSIGNABLE_BLOCKS * BLOCK_SIZE);
  }else{
    char summary_buffer[SUMMARY_BLOCKS * BLOCK_SIZE];
    std::fstream segment_file("DRIVE/SEGMENT"+std::to_string(dirty_segment_no), std::ios::in | std::ios::out | std::ios::binary);
    segment_file.read(dirty_segment, ASSIGNABLE_BLOCKS * BLOCK_SIZE);
    segment_file.read(summary_buffer, SUMMARY_BLOCKS * BLOCK_SIZE);
    std::memcpy(dirty_segment, summary_buffer, SUMMARY_BLOCKS * BLOCK_SIZE);
    segment_file.close();
  }

  for (int i = 0; i < ASSIGNABLE_BLOCKS; ++i){
    unsigned int inode_no = dirty_summary[i][0];
    unsigned int block_no = dirty_summary[i][1];
    if (inode_no != (unsigned int) -1 && block_no != (unsigned int) -1){       //--------datablock--------
      if (ASSIGNABLE_BLOCKS - next_available_block_clean < 3 + fragments.size() + inodes.size())
        writeCleanSegment(clean_summary, clean_segment, next_available_block_clean, clean_segment_no, inodes, fragments);

      if (getInode(inode_no).block_locations[block_no] == (dirty_segment_no-1) * BLOCKS_IN_SEG + i) { //if data block is live
        //deal with imap piece
        fragments.insert(inode_no / BLOCKS_IN_SEG);

        //deal with inode
        inode old_node = getInode(inode_no);
        bool duplicate_inode = false; // sees whether this inode was already in our vector
        for (int j = 0; j < inodes.size(); ++j) {
          if (inodes[j].filename == old_node.filename){
            inodes[j].block_locations[block_no] = (clean_segment_no-1) * BLOCKS_IN_SEG + next_available_block_clean;
            duplicate_inode = true;
            break;
          }
        }
        if (!duplicate_inode)
          inodes.push_back(old_node);

        //deal with actual data
        std::memcpy(&clean_segment[next_available_block_clean * BLOCK_SIZE], &dirty_segment[i * BLOCK_SIZE], BLOCK_SIZE);
        clean_summary[next_available_block_clean][0] = inode_no;
        clean_summary[next_available_block_clean][0] = block_no;
        next_available_block_clean++;
      }
    }else if (inode_no != (unsigned int) -1 && block_no == (unsigned int) -1){ //--------inode block--------
      if (ASSIGNABLE_BLOCKS - next_available_block_clean < 2 + fragments.size() + inodes.size())
        writeCleanSegment(clean_summary, clean_segment, next_available_block_clean, clean_segment_no, inodes, fragments);

      fragments.insert(inode_no / BLOCKS_IN_SEG);

      inode old_node = getInode(inode_no);
      bool duplicate_inode = false; // sees whether this inode was already in our vector
      for (int j = 0; j < inodes.size(); ++j) {
        if (inodes[j].filename == old_node.filename){
          inodes[j].block_locations[block_no] = (clean_segment_no-1) * BLOCKS_IN_SEG + i;
          duplicate_inode = true;
          break;
        }
      }
      if (!duplicate_inode)
        inodes.push_back(old_node);
    }else if (inode_no == (unsigned int) -1 && block_no != (unsigned int) -1){ //--------imap fragment--------
      if (ASSIGNABLE_BLOCKS - next_available_block_clean < 1 + fragments.size() + inodes.size())
        writeCleanSegment(clean_summary, clean_segment, next_available_block_clean, clean_segment_no, inodes, fragments);

      fragments.insert(inode_no / BLOCKS_IN_SEG);
    }
  }
}
