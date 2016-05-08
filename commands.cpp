#include "Functions.cpp"

void import(std::string filename, std::string lfs_filename) {
  std::ifstream in(filename);
  if (!in.good()){
    std::cout << "Could not find file." << std::endl;
    return;
  }

  if (lfs_filename.length() > 251){
    std::cout << "Filename too large." << std::endl;
    return;
  }

  if (getInodeNumberOfFile(lfs_filename) != (unsigned int) -1){
    std::cout << "Duplicate filename." << std::endl;
    return;
  }

  int inode_number = nextInodeNumber();

  if (inode_number == -1) {
    std::cout << "Max files reached." << std::endl;
    return;
  }
  updateFilemap(inode_number, lfs_filename);

  //get input file length
  in.seekg(0, std::ios::end);
  int in_size = in.tellg();
  in.seekg(0, std::ios::beg);

  if ((in_size / BLOCK_SIZE) + 2 > BLOCKS_IN_SEG - AVAILABLE_BLOCK) { // not enough space left in segment
    writeOutSegment();
    SEGMENT_NO++;
    AVAILABLE_BLOCK = 0;
  }

  //read from file we're importing and write it in blocks of SEGMENT
  char buffer[in_size];
  in.read(buffer, in_size);
  std::memcpy(&SEGMENT[AVAILABLE_BLOCK * BLOCK_SIZE], buffer, in_size);

  //inode blocks
  inode meta;
  for (int i = 0; i < lfs_filename.length(); ++i){
    meta.filename[i] = lfs_filename[i];
  }
  meta.filename[lfs_filename.length()] = '\0';
  meta.size = in_size;
  for (int i = 0; i < in_size/BLOCK_SIZE + 1; ++i){
    meta.block_locations[i] = AVAILABLE_BLOCK + (SEGMENT_NO-1)*BLOCKS_IN_SEG;
    AVAILABLE_BLOCK++;
  }

  //write that inode to the next BLOCK
  std::memcpy(&SEGMENT[AVAILABLE_BLOCK*BLOCK_SIZE], &meta, sizeof(inode));
  AVAILABLE_BLOCK++;

  //update imap (which also updates checkpoint region)
  updateImap(inode_number, (AVAILABLE_BLOCK - 1) + (SEGMENT_NO-1)*BLOCKS_IN_SEG);

  printf("Next free: %d\n", AVAILABLE_BLOCK);

  in.close();
}

void remove(std::string lfs_filename) {
  unsigned int inode_number = getInodeNumberOfFile(lfs_filename);

  if (inode_number == (unsigned int) -1){
    std::cout << "Could not find file." << std::endl;
    return;
  }

  std::fstream filemap("DRIVE/FILEMAP", std::ios::binary | std::ios::in | std::ios::out);

  filemap.seekp(inode_number*FILEMAP_BLOCK_SIZE);
  filemap.write(INVALID, 1);

  filemap.close();

  updateImap(inode_number, (unsigned int) -1);
}

void cat(std::string lfs_filename) {
  unsigned int inode_number = getInodeNumberOfFile(lfs_filename);

  if (inode_number == (unsigned int) -1){
    std::cout << "Could not find file." << std::endl;
    return;
  }

  unsigned int global_block_pos = IMAP[inode_number];
  unsigned int segment_no = (global_block_pos / BLOCKS_IN_SEG) + 1;
  unsigned int local_block_pos = (global_block_pos % BLOCKS_IN_SEG) * BLOCK_SIZE;

  inode meta;

  if (segment_no != SEGMENT_NO){
    std::fstream filemap("DRIVE/SEGMENT"+std::to_string(segment_no), std::ios::binary | std::ios::in);

    filemap.seekg(local_block_pos);
    char buffer[BLOCK_SIZE];
    filemap.read(buffer, BLOCK_SIZE);

    std::memcpy(&meta, buffer, sizeof(inode));

    filemap.close();
  }else{
    std::memcpy(&meta, &SEGMENT[local_block_pos], sizeof(inode));
  }

  int no_data_blocks = (meta.size / BLOCK_SIZE) + 1;

  for (int i = 0; i < no_data_blocks; ++i)
    printBlock(meta.block_locations[i], meta.size, (i == no_data_blocks - 1));
}

void display(std::string lfs_filename, std::string amount, std::string start) {
  /*
  std::fstream filename_map;
  std::string line;
  std::string inode_string;
  filename_map.open("DRIVE/FILENAME_MAP", std::ios::binary | std::ios::in);
  while(getline(filename_map, line)){
    if(line.length() > 1){
      if(lfs_filename.compare(split(line)[0]) == 0){
        inode_string = split(line)[1];
      }
    }
  }
  const char * inode_num = inode_string.c_str();
  int inode_number_int = atoi(inode_num);
  unsigned int inode_number = (unsigned int) inode_number_int;
  unsigned int blockPos = IMAP[inode_number];
  */
}

void overwrite(std::string lfs_filename, std::string amount, std::string start, std::string character) {
  /*
  std::fstream filename_map;
  std::string line;
  std::string inode_string;
  filename_map.open("DRIVE/FILENAME_MAP", std::ios::binary | std::ios::in);
  while(getline(filename_map, line)){
    if(line.length() > 1){
      if(lfs_filename.compare(split(line)[0]) == 0){
        inode_string = split(line)[1];
      }
    }
  }
  const char * inode_num = inode_string.c_str();
  int inode_number_int = atoi(inode_num);
  unsigned int inode_number = (unsigned int) inode_number_int;
  unsigned int blockPos = IMAP[inode_number];
  */
}

void list() {
  std::ifstream filemap("DRIVE/FILEMAP");

  for (int i = 0; i < MAX_FILES; ++i){
    filemap.seekg(i*FILEMAP_BLOCK_SIZE);
    char valid[1];
    filemap.read(valid, 1);

    if (valid[0]) {
      char filename[FILEMAP_BLOCK_SIZE-4];
      filemap.read(filename, FILEMAP_BLOCK_SIZE-4);

      printf("%s %d\n", filename, getFileSize(i));
    }
  }

  filemap.close();
}

void exit() {
  writeOutSegment();
  exit(0);
}
