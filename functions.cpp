#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cmath>
#include <stdbool.h>

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

void printFileNames(){
	std::ifstream filenames("DRIVE/FILENAME_MAP");
	std::string line;
	while(getline(filenames, line)){
		std::vector<std::string> components = split(line);
		std::cout << split(line)[0] << std::endl;
	}
}