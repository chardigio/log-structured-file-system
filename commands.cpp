#include "functions.cpp"

void import(std::string filename, std::string lfs_filename) {
  //look for next sequential inode number
  int inode_number = nextInodeNumber();

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
  unsigned int last_imap_pos;
  findAvailableSpace(SEGMENT_NO, last_imap_pos); //
  unsigned int data_block_start_pos;

  if (last_imap_pos > SEG_SIZE){ // if this is our first time importing
    data_block_start_pos = 0;
  }else if (BLOCK_SIZE - last_imap_pos%BLOCK_SIZE < ((in_size+BLOCK_SIZE-1)/BLOCK_SIZE) + 2) { // not enough space left in segment
    writeOutSegment(SEGMENT_NO);
    SEGMENT_NO++;
    data_block_start_pos = 0;
  }else{
    data_block_start_pos = last_imap_pos + 1;
  }

  //read from file we're importing and write it in blocks of SEGMENT
  unsigned int i;
  for (i = 0; i*BLOCK_SIZE < in_size; ++i) {
    char block[BLOCK_SIZE];
    in.read(block, BLOCK_SIZE);
    std::memcpy(SEGMENT[data_block_start_pos+i], block, BLOCK_SIZE);
  }

  // inode block string formatting (filename size(inbytes) block1 block2 block3 ... blockN)
  std::string inode_meta = lfs_filename + " " + std::to_string(in_size) + " ";
  for (int j = 0; j < i; ++j)
    inode_meta += std::to_string(data_block_start_pos + j) + " ";

  //write that string to the next BLOCK
  std::memcpy(SEGMENT[data_block_start_pos+i], inode_meta.c_str(), inode_meta.length());

  //update filename map
  updateFilenameMap(inode_number, lfs_filename);

  //write old imap and new inode number to next block
  unsigned int inode_block_no = data_block_start_pos + i;
  unsigned int imap_block_no = inode_block_no + 1;

  IMAP[inode_number] = inode_block_no;
  unsigned int imap_fragment_no = (imap_block_no)/BLOCK_SIZE; //the block of the imap we need to copy to the segment
  std::memcpy(SEGMENT[imap_block_no], &IMAP[imap_fragment_no*BLOCK_SIZE], BLOCK_SIZE);

  //update checkpoint region
  updateCR(imap_fragment_no, imap_block_no);

  /*
  printf("%s\n", "let's import!");
  printf("inode#:%d\n", inode_number);
  printf("in_size:%d\n", in_size);
  printf("segno, last_imap_pos:%d, %d\n", SEGMENT_NO, last_imap_pos);
  printf("data_block_start_pos%d\n", data_block_start_pos);
  std::cout << "inode meta:" << inode_meta << std::endl;
  printf("imap_fragment_no%u\n", imap_fragment_no);
  printf("latest_stored_at:%u\n", imap_block_no);
  */

  in.close();
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
  writeOutSegment(SEGMENT_NO);
	printFileNames();
	exit(0);
  //nothing
}
