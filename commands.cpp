#include "functions.cpp"

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
  printFileNames();
}

void exit() {
	printFileNames();
	exit(1);
  //nothing
}