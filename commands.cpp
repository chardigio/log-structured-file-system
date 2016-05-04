#include "functions.cpp"

void import(std::string filename, std::string lfs_filename) {
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
  findAvailableSpace(SEGMENT_NO, last_imap_pos);
  unsigned int data_block_start_pos;

  if (last_imap_pos > SEG_SIZE){ // if this is our first time importing
    data_block_start_pos = 0;
  }else if (BLOCK_SIZE - last_imap_pos%BLOCK_SIZE < ((in_size+BLOCK_SIZE-1)/BLOCK_SIZE) + 2) { // not enough space left in segment
    writeOutSegment();
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

  //look for next sequential inode number
  int inode_number = nextInodeNumber();

  //update filename map
  updateFilenameMap(inode_number, lfs_filename);

  //write old imap and new inode number to next block
  unsigned int inode_block_no = data_block_start_pos + i;
  unsigned int imap_block_no = inode_block_no + 1;

  IMAP[inode_number] = inode_block_no;
  printf("IMAP[%u] = %u\n", inode_number, inode_block_no);
  unsigned int imap_fragment_no = (inode_number)/BLOCK_SIZE; //the block of the imap we need to copy to the segment
  std::memcpy(SEGMENT[imap_block_no], &IMAP[imap_fragment_no*BLOCK_SIZE], BLOCK_SIZE);

  //update checkpoint region
  updateCR(imap_fragment_no, imap_block_no+(SEGMENT_NO-1)*BLOCK_SIZE);

  /*
  printf("%s\n", "let's import!");
  printf("inode#:%d\n", inode_number);
  printf("in_size:%d\n", in_size);
  printf("segno, last_imap_pos:%d, %d\n", SEGMENT_NO, last_imap_pos);
  printf("data_block_start_pos%d\n", data_block_start_pos);
  //std::cout << "inode meta:" << inode_meta << std::endl;
  printf("imap_fragment_no%u\n", imap_fragment_no);
  printf("latest_stored_at:%u\n", imap_block_no);
  */

  printf("Reached: %d\n", imap_block_no);

  in.close();
}

void remove(std::string lfs_filename) {
  std::fstream filename_map;
  std::string line;
  std::string inode_string;
  filename_map.open("DRIVE/FILENAME_MAP", std::ios::binary | std::ios::in);
  while(getline(filename_map, line)){
    if(line.length() > 1){
      if(lfs_filename.compare(split(line)[0]) == 0){
        split(line)[1] = inode_string;
        removeLineFromFilenameMap(lfs_filename);
      }
    }
  }
  const char * inode_num = inode_string.c_str();
  int inode_number_int = atoi(inode_num);
  unsigned int inode_number = (unsigned int) inode_number_int;
  IMAP[inode_number] = -1;

  /*
  DONE go to filename_map -> find the inode# related to the filename
                     -> remove contents of this line before \n (this may require creating a new FILENAME_MAP and deleting the old one)
  DONE go to imap in memory -> change IMAP[inode#] to -1
  go to segment in memory (as long as there's >= 1 block left) -> write out imap BLOCK_SIZE-sized fragment at IMAP[inode#/BLOCK_SIZE]
  go to checkpoint region -> update byte (inode#/BLOCK_SIZE) with block_no+SEG_NO*BLOCK_SIZE
  */
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
  writeOutSegment();
	//printFileNames();
	exit(0);
}
