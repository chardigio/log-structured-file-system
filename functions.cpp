#include "Lab7.h"

std::vector<std::string> split(const std::string &str) {
  std::string buf;
  std::stringstream ss(str);
  std::vector<std::string> tokens;
  int no_tokens = 0;
  while (ss >> buf && no_tokens++ < 128+2) tokens.push_back(buf);
  return tokens;
}

bool filenameIsUnique(std::string lfs_filename){
  std::ifstream filename_map("DRIVE/FILENAME_MAP");

  std::string token;
  bool is_unique = true;

  while(filename_map >> token){
    if(token == lfs_filename)
      is_unique = false;
  }

  filename_map.close();

  return is_unique;
}

void copyImapBlock(unsigned int address, unsigned int fragment_no){
  unsigned int segment_no = (address / BLOCK_SIZE) + 1;
  unsigned int block_start_pos = (address % BLOCK_SIZE) * BLOCK_SIZE;
  std::string segment_name = "DRIVE/SEGMENT" + std::to_string(segment_no);
  std::fstream segment_file;
  segment_file.open(segment_name.c_str(), std::fstream::binary | std::ios::in);

  segment_file.seekg(block_start_pos);
  char buffer[BLOCK_SIZE];
  segment_file.read(buffer, BLOCK_SIZE);
  std::memcpy(&IMAP[fragment_no*BLOCK_SIZE], buffer, BLOCK_SIZE);
  segment_file.close();
}

void copyInImap(){
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

  for (unsigned int fragment_no = 0; fragment_no < addresses.size(); ++fragment_no){
    copyImapBlock(addresses[fragment_no], fragment_no);
  }
}

void copyInSegment(){
  std::string segment_name = "DRIVE/SEGMENT" + std::to_string(SEGMENT_NO);
  std::fstream segment_file;
  segment_file.open(segment_name.c_str(), std::fstream::binary | std::ios::in);

  for (int i = 0; i < BLOCK_SIZE; ++i){
    char buffer[BLOCK_SIZE];
    segment_file.read(buffer, BLOCK_SIZE);
    std::memcpy(SEGMENT[i], buffer, BLOCK_SIZE);
  }

  segment_file.close();
}

void writeOutSegment(){
  std::string segment_name = "DRIVE/SEGMENT" + std::to_string(SEGMENT_NO);
  std::fstream segment_file;
  segment_file.open(segment_name.c_str(), std::fstream::binary | std::ios::out);

  for (int i = 0; i < BLOCK_SIZE; ++i)
    segment_file.write(reinterpret_cast<const char*>(&SEGMENT[i]), BLOCK_SIZE);

  segment_file.close();
}

void updateCR(unsigned int imap_fragment, unsigned int new_imap_pos){
  std::fstream cr;
  cr.open("DRIVE/CHECKPOINT_REGION", std::ios::binary | std::ios::out | std::ios::in);

  cr.seekp(imap_fragment*4);
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
    last_imap_pos = ((unsigned int) *most_recent_imap_pos) % BLOCK_SIZE;
    segment_no = 1+ ((unsigned int) *most_recent_imap_pos) / BLOCK_SIZE;
  }else{ // if this is the first import to your system
    last_imap_pos = (unsigned int) -1;
    segment_no = 1;
  }
}

unsigned int nextInodeNumber(){
  unsigned int next_inode_number = 0;
  std::ifstream filenames("DRIVE/FILENAME_MAP");
  std::string line;
  int minimum_inode_number_for_seg = (SEGMENT_NO-1)*BLOCK_SIZE;
  int lines = -1;

  while (getline(filenames, line)){
    if (++lines > minimum_inode_number_for_seg && line.length() <= 1) break;
    std::vector<std::string> components = split(line);
    next_inode_number++;
  }

  filenames.close();

  return (lines < minimum_inode_number_for_seg) ? minimum_inode_number_for_seg : next_inode_number;
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

  std::fstream filename_map2("DRIVE/FILENAME_MAP", std::ios::in | std::ios::out);
  filename_map2.seekp(file_pos);
  while (line_no++ < inode_number)
    filename_map2.write("\n", 1);
  std::string filename_map_string = lfs_filename + " " + std::to_string(inode_number) + "\n";
  filename_map2.write(filename_map_string.c_str(), filename_map_string.length());

  filename_map2.close();
}

std::string getFileSize(std::string inode_string){
  const char * inode_num = inode_string.c_str();
  int inode_number_int = atoi(inode_num);
  unsigned int inode_number = (unsigned int) inode_number_int;
  unsigned int block_position = IMAP[inode_number];
  std::string fileSize;
  if(block_position == -1){
    //the node doesn't exit
    //this should never get here and would be an error on our part
    std::cout << "Lethal error dude slow your roll." << std::endl;
  }

  unsigned int segment_location = inode_number/BLOCK_SIZE;
  if(SEGMENT_NO == (segment_location+1)){
    std::string tmp = (char*) (SEGMENT[block_position]);
    fileSize = split(tmp)[1];
  }
  else{
    std::string segment = "SEGMENT" + std::to_string(segment_location + 1);
    std::fstream disk_segment;
    disk_segment.open("DRIVE/"+segment, std::ios::binary | std::ios::in);
    unsigned int block_in_segment = block_position % BLOCK_SIZE;
    char block[BLOCK_SIZE];
    disk_segment.seekg(block_in_segment * BLOCK_SIZE);
    char buffer[BLOCK_SIZE];
    disk_segment.read(buffer, BLOCK_SIZE);
    std::memcpy(&block, &buffer, BLOCK_SIZE);
    std::string block_string(block);
    fileSize = split(block_string)[1];
  }

  return fileSize;
}

void printFileNames(){
  std::ifstream filenames("DRIVE/FILENAME_MAP");
  std::string line;
  while(getline(filenames, line)){
    if (line.length() > 1){
      std::vector<std::string> components = split(line);
      std::cout << split(line)[0] << ", " << getFileSize(split(line)[1]) << std::endl;
    }
  }
}

void removeLineFromFilenameMap(std::string lfs_filename){
  std::ifstream filename_map("DRIVE/FILENAME_MAP");
  std::ofstream tempFile("DRIVE/temp.txt");

  std::string fileName;
  std::string inode_number;

  while(filename_map >> fileName >> inode_number){
    if(fileName != lfs_filename){
      tempFile << fileName << " " << inode_number << std::endl;
    }
    else{
      tempFile << std::endl;
    }
  }
  filename_map.clear();
  filename_map.seekg(0, std::ios::beg);
  filename_map.close();
  tempFile.close();
  remove("DRIVE/FILENAME_MAP");
  rename("DRIVE/temp.txt", "DRIVE/FILENAME_MAP");
}