#include "Lab7.h"

std::vector<std::string> split(const std::string &str) {
  std::string buf;
  std::stringstream ss(str);
  std::vector<std::string> tokens;
  while (ss >> buf) tokens.push_back(buf);
  return tokens;
}

void updateCR(bool new_imap, unsigned int imap_pos){
  std::fstream cr;
  cr.open("DRIVE/CHECKPOINT_REGION", std::ios::binary | std::ios::out | std::ios::in);

  char current_address_str[4];
  unsigned int last_current_address_int = 0;
  unsigned int new_current_address_int = 0;
  int checks = -1;

  while ((new_current_address_int != 0 || checks == -1) && ++checks < IMAP_BLOCKS){
    last_current_address_int = new_current_address_int;
    new_current_address_int = 0;
    cr.read(current_address_str, 4);

    for (int i = 0; i < 4; ++i){
      new_current_address_int += ((unsigned int) (current_address_str[i])) * pow(256,(3-i));
    }
  }

  if (checks == 0 || new_imap) cr.seekp((checks)*4);
  else cr.seekp((checks-1)*4);

  cr.write(reinterpret_cast<const char*>(&imap_pos), 4);

  cr.close();
}

void findAvailableSpace(unsigned int& segment, unsigned int& last_imap_pos){
  std::fstream cpr;
  cpr.open("DRIVE/CHECKPOINT_REGION", std::ios::binary | std::ios::out | std::ios::in);

  char current_address_str[4];
  unsigned int last_current_address_int = 0;
  unsigned int new_current_address_int = 0;
  int checks = 0;

  while ((new_current_address_int != 0 || checks == 0) && ++checks < IMAP_BLOCKS){
    last_current_address_int = new_current_address_int;
    new_current_address_int = 0;
    cpr.read(current_address_str, 4);

    for (int i = 0; i < 4; ++i)
      new_current_address_int += ((unsigned int) (current_address_str[i])) * pow(256,(i));
  }

  cpr.close();

  last_imap_pos = last_current_address_int % BLOCK_SIZE;
  segment = 1 + last_current_address_int / BLOCK_SIZE;
}

void copyImap(const std::string& segment, const unsigned int& position, char* new_imap_block, unsigned long& imap_end_pos){
  std::fstream old_imap_block;
  old_imap_block.open(segment, std::ios::binary | std::ios::out | std::ios::in);
  old_imap_block.seekg(position);

  unsigned int last_current_address_int = 0;
  unsigned int new_current_address_int = 0;
  int checks = -1;

  while ((new_current_address_int != 0 || checks == -1) && ++checks < BLOCK_SIZE/4){
    last_current_address_int = new_current_address_int;
    new_current_address_int = 0;
    old_imap_block.read(new_imap_block+4*checks, 4);

    for (int i = 0; i < 4; ++i)
      new_current_address_int += ((unsigned int) (new_imap_block[i+4*checks])) * pow(256,(i));
  }

  old_imap_block.close();
  imap_end_pos = checks * 4;
}

unsigned int nextInodeNumber(std::string lfs_filename){
  unsigned int next_inode_number = 1;
  std::ifstream filenames("DRIVE/FILENAME_MAP");
  std::string line;
  while (getline(filenames, line)){
    std::vector<std::string> components = split(line);
    if (split(line)[0] == lfs_filename) return 0;
    next_inode_number = atoi(split(line)[1].c_str()) + 1;
  }

  filenames.close();

  return next_inode_number;
}

void updateFilenameMap(unsigned int inode_number, std::string lfs_filename){
  std::fstream filename_map("DRIVE/FILENAME_MAP", std::ofstream::out | std::ofstream::app);
  filename_map << lfs_filename << " " << std::to_string(inode_number) << std::endl;
  filename_map.close();
}

void import(std::string filename, std::string lfs_filename) {
  //look for next sequential inode number, also check for filename dupe
  int inode_number = nextInodeNumber(lfs_filename);
  if (inode_number == 0){
    std::cout << "Error: Duplicate filename." << std::endl;
    return;
  }

  //open file that we're importing
  std::ifstream in(filename);
  if (!in.good()){
    std::cout << "Could not find file." << std::endl;
    return;
  }

  //get input file length
  in.seekg(0, std::ios::end);
  int in_size = in.tellg();
  in.seekg(0, std::ios::beg);

  //find first available block and see what segment it is
  unsigned int segment_no;
  unsigned int last_imap_block_no;
  findAvailableSpace(segment_no, last_imap_block_no);
  std::string segment_name = "DRIVE/SEGMENT" + std::to_string(segment_no);
  unsigned long last_imap_pos = (last_imap_block_no) ? (last_imap_block_no-1)*BLOCK_SIZE : last_imap_block_no*BLOCK_SIZE;

  //get contents from last imap to copy to our new imap
  char* imap_block = new char[BLOCK_SIZE];
  unsigned long imap_end_pos = 0;
  bool new_imap = false;
  copyImap(segment_name, last_imap_pos, imap_block, imap_end_pos);

  //if this is the first imap or the last imap was full, we want a new imap
  if (imap_end_pos % BLOCK_SIZE < 8) { // 8 is the size of a byte pair
    imap_end_pos = 0;
    new_imap = true;
  }

  //open SEGMENT and seek to first available block
  std::fstream segment_file;
  segment_file.open(segment_name.c_str(), std::fstream::binary | std::ios::out | std::ios::in);
  if (last_imap_pos != 0) segment_file.seekp(last_imap_pos+BLOCK_SIZE);

  //read from file we're importing and write it in blocks of SEGMENT
  unsigned int i;
  for (i = 0; i*BLOCK_SIZE < in_size; ++i) { //ask if we need to end segment with inode&imap
    char block[BLOCK_SIZE];
    in.read(block, BLOCK_SIZE);
    segment_file.write(block, BLOCK_SIZE);
  }

  // inode block string formatting (filename size(inbytes) block1 block2 block3 ... blockN)
  std::string inode_meta = lfs_filename + " " + std::to_string(in_size) + " ";
  for (int j = 0; j < i; ++j)
    inode_meta += std::to_string(last_imap_block_no + j) + " ";

  //write that string to the next BLOCK
  segment_file.seekp((last_imap_block_no + i) * BLOCK_SIZE);
  segment_file.write(inode_meta.c_str(), inode_meta.length());

  //update filename map
  updateFilenameMap(inode_number, lfs_filename);

  //write old imap and new inode number to next block
  printf("%u\n", last_imap_block_no);
  unsigned int inode_block = last_imap_block_no + i + 1;
  segment_file.seekp((last_imap_block_no + i + 1) * BLOCK_SIZE);
  segment_file.write(imap_block, imap_end_pos);
  segment_file.write(reinterpret_cast<const char*>(&inode_number), 4);
  segment_file.write(reinterpret_cast<const char*>(&inode_block), 4);

  //update checkpoint region
  updateCR(new_imap, (segment_no-1)*SEG_SIZE/BLOCK_SIZE + (i+2) + last_imap_block_no); //i+2 to account for inode,imap blocks

  in.close();
  segment_file.close();
}

void remove(std::string lfs_filename) {
  //nothing
}

void cat(std::string lfs_filename) {
  //nothing
}

void display(std::string lfs_filename, std::string amount, std::string start) {
  //nothing
}

void overwrite(std::string lfs_filename, std::string amount, std::string start, std::string character) {
  //nothing
}

void list() {
  //nothing
}

void exit() {
  //nothing
}

void parseLine(std::string line) {
  if (line.length() == 0) return;

  std::vector<std::string> command = split(line);

  if      (command[0] == "import"    && command.size() == 3) import(command[1], command[2]);
  else if (command[0] == "remove"    && command.size() == 2) remove(command[1]);
  else if (command[0] == "cat"       && command.size() == 2) cat(command[1]);
  else if (command[0] == "display"   && command.size() == 4) display(command[1], command[2], command[3]);
  else if (command[0] == "overwrite" && command.size() == 5) overwrite(command[1], command[2], command[3], command[4]);
  else if (command[0] == "list"      && command.size() == 1) list();
  else if (command[0] == "exit"      && command.size() == 1) exit();
  else std::cout << "Command not recognized." << std::endl;
}

int main(int argc, char* argv[]){
  struct stat st = {0};

  if (stat("./DRIVE", &st) == -1){
    std::cout << "Could not find DRIVE. Please run 'make drive' and try again." << std::endl;
    return 1;
  }

  std::string line;

  std::cout << "LFS$ ";
  while (getline(std::cin, line)){
    parseLine(line);
    std::cout << "LFS$ ";
  }

  return 0;
}
