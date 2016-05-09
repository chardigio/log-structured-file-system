#include "Lab7.h"

std::vector<std::string> split(const std::string &str) {
  std::string buf;
  std::stringstream ss(str);
  std::vector<std::string> tokens;
  int no_tokens = 0;
  while (ss >> buf && no_tokens++ < 128+2) tokens.push_back(buf);
  return tokens;
}

void findNextAvailableBlock(){
  std::fstream cpr("DRIVE/CHECKPOINT_REGION", std::ios::binary | std::ios::out | std::ios::in);

  char current_address_str[4];
  unsigned int new_current_address_int = 0;
  int checks = 0;
  std::vector<unsigned int> addresses;

  while ((new_current_address_int < SEG_SIZE) && ++checks < IMAP_BLOCKS){
    cpr.read(current_address_str, 4);

    std::memcpy(&new_current_address_int, &current_address_str, 4);

    if (new_current_address_int < SEG_SIZE)
      addresses.push_back(new_current_address_int);
  }

  if (addresses.size() > 0){
    auto most_recent_imap_pos = std::max_element(std::begin(addresses), std::end(addresses));
    AVAILABLE_BLOCK = ((unsigned int) *most_recent_imap_pos) % BLOCK_SIZE;
    SEGMENT_NO = 1+ ((unsigned int) *most_recent_imap_pos) / BLOCK_SIZE;
  }

  cpr.close();
}

void readInSegment(){
  std::fstream segment_file("DRIVE/SEGMENT" + std::to_string(SEGMENT_NO), std::fstream::binary | std::ios::in);

  segment_file.read(SEGMENT, SEG_SIZE);

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
  std::fstream cpr;
  cpr.open("DRIVE/CHECKPOINT_REGION", std::ios::binary | std::ios::out | std::ios::in);

  char current_address_str[4];
  unsigned int new_current_address_int = 0;
  int checks = 0;
  std::vector<unsigned int> addresses;

  while (checks++ < IMAP_BLOCKS){
    cpr.read(current_address_str, 4);

    std::memcpy(&new_current_address_int, &current_address_str, 4);

    if (new_current_address_int < SEG_SIZE)
      addresses.push_back(new_current_address_int);
    else
      break;
  }

  for (unsigned int i = 0; i < addresses.size(); ++i)
    readInImapBlock(addresses[i], i);

  cpr.close();
}

void writeOutSegment(){
  std::fstream segment_file("DRIVE/SEGMENT"+std::to_string(SEGMENT_NO), std::fstream::binary | std::ios::out);

  segment_file.write(SEGMENT, SEG_SIZE);

  SEGMENT_NO++;
  AVAILABLE_BLOCK = 0;

  segment_file.close();
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

void updateCR(unsigned int fragment, unsigned int block_position){
  std::fstream cpr("DRIVE/CHECKPOINT_REGION", std::ios::binary | std::ios::out | std::ios::in);

  cpr.seekp(fragment * 4);
  cpr.write(reinterpret_cast<const char*>(&block_position), 4);

  cpr.close();
}

void updateImap(unsigned int inode_number, unsigned int block_position){
  if (AVAILABLE_BLOCK == BLOCKS_IN_SEG)
    writeOutSegment();

  IMAP[inode_number] = block_position;

  unsigned int fragment = inode_number / BLOCKS_IN_SEG;

  std::memcpy(&SEGMENT[AVAILABLE_BLOCK * BLOCK_SIZE], &IMAP[fragment * BLOCK_SIZE], BLOCK_SIZE);

  AVAILABLE_BLOCK++;

  updateCR(inode_number/BLOCKS_IN_SEG, (AVAILABLE_BLOCK - 1) + (SEGMENT_NO - 1) * BLOCKS_IN_SEG);
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

 void writeInode(const inode &node, unsigned int inode_number){
  SEGMENT_SUMMARY[AVAILABLE_BLOCK][0] =  inode_number;
  SEGMENT_SUMMARY[AVAILABLE_BLOCK][1] = (unsigned int) -1;

  //write that inode to the next BLOCK
  std::memcpy(&SEGMENT[AVAILABLE_BLOCK*BLOCK_SIZE], &node, sizeof(inode));
  AVAILABLE_BLOCK++;
 }
