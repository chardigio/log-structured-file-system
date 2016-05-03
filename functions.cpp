#include "Lab7.h"

std::vector<std::string> split(const std::string &str) {
  std::string buf;
  std::stringstream ss(str);
  std::vector<std::string> tokens;
  while (ss >> buf) tokens.push_back(buf);
  return tokens;
}

void writeOutSegment(unsigned int segment_no){
  std::string segment_name = "DRIVE/SEGMENT" + std::to_string(segment_no);
  std::fstream segment_file;
  segment_file.open(segment_name.c_str(), std::fstream::binary | std::ios::out);

  //segment_file.write(reinterpret_cast<const char*>(&SEGMENT), SEG_SIZE); //not sure why this doesnt work

  for (int i = 0; i < BLOCK_SIZE; ++i)
    segment_file.write(reinterpret_cast<const char*>(&SEGMENT[i]), BLOCK_SIZE);

  segment_file.close();
}

void updateCR(unsigned int new_imap_block_no, unsigned int new_imap_pos){
  std::fstream cr;
  cr.open("DRIVE/CHECKPOINT_REGION", std::ios::binary | std::ios::out | std::ios::in);

  cr.seekp(new_imap_block_no);
  cr.write(reinterpret_cast<const char*>(&new_imap_pos), 4);

  cr.close();
}

void findAvailableSpace(unsigned int& segment_no, unsigned int& last_imap_pos){
  std::fstream cpr;
  cpr.open("DRIVE/CHECKPOINT_REGION", std::ios::binary | std::ios::out | std::ios::in);

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

  cpr.close();

  if (addresses.size() > 0){
    auto most_recent_imap_pos = std::max_element(std::begin(addresses), std::end(addresses));
    last_imap_pos = (unsigned int) *most_recent_imap_pos;
    segment_no = 1 + last_imap_pos / BLOCK_SIZE;
  }else{ // if this is the first import to your system
    last_imap_pos = (unsigned int) -1;
    segment_no = 1;
  }
}

unsigned int nextInodeNumber(){
  unsigned int next_inode_number = 0;
  std::ifstream filenames("DRIVE/FILENAME_MAP");
  std::string line;

  while (getline(filenames, line)){
    if (line.length() <= 1) break;
    std::vector<std::string> components = split(line);
    next_inode_number++;
  }

  filenames.close();

  return next_inode_number;
}

void updateFilenameMap(unsigned int inode_number, std::string lfs_filename){
  std::fstream filename_map("DRIVE/FILENAME_MAP", std::ios::binary | std::ios::in | std::ios::out);
  std::string line;
  int line_no = 0;
  int file_pos = 0;
  char next;

  while(filename_map.get(next) && line_no < inode_number){
    file_pos++;
    if (next == '\n') line_no++;
  }
  filename_map.close();

  //this is ratchet stop it charlie (but i dont know why seek breaks) (yeah me neither dude)

  std::fstream filename_map2("DRIVE/FILENAME_MAP", std::ios::in | std::ios::out);
  filename_map2.seekp(file_pos);
  std::string filename_map_string = lfs_filename + " " + std::to_string(inode_number) + "\n";
  filename_map2.write(filename_map_string.c_str(), filename_map_string.length());

  printf("writing %sat: %d, line: %d\n", filename_map_string.c_str(), (int)filename_map2.tellp(), line_no);

  filename_map2.close();
}

std::string getFileSize(std::string inode_string){
  const char * inode_num = inode_string.c_str();
  int inode_number_int = atoi(inode_num);
  unsigned int inode_number = (unsigned int) inode_number_int;
  unsigned int block_position = IMAP[inode_number];
  if(block_position == -1){
    //the node doesn't exit
  }

  unsigned int segment_location = block_position/BLOCK_SIZE;
  if(SEGMENT_NO == segment_location){
    std::string fileSize = split(std::to_string(SEGMENT[SEGMENT_NO][block_position]))[1];
  }
  else{
    std::string segment = "SEGMENT" + std::to_string(SEGMENT_NO);
    std::ifstream disk_segment("DRIVE/"+segment);
    unsigned int block_in_segment = block_position % BLOCK_SIZE;
    char block[BLOCK_SIZE];
    disk_segment.seekg(block_in_segment);
    char buffer[BLOCK_SIZE];
    disk_segment.read(buffer, BLOCK_SIZE);
    //block should be the inode
    memcpy(block, buffer, BLOCK_SIZE);
    std::string fileSize = split(std::to_string(block))[1];
    //use block to get the file size 
  }

}

void printFileNames(){
	std::ifstream filenames("DRIVE/FILENAME_MAP");
	std::string line;
	while(getline(filenames, line)){
		std::vector<std::string> components = split(line);
		std::cout << split(line)[0] << ", " << getFileSize(split(line)[1]) << std::endl;
	}
}
