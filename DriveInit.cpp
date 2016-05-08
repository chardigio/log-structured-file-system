#include "Lab7.h"

void makeDriveDir(){
  struct stat st = {0};

  if (stat("./DRIVE", &st) == -1)
    mkdir("./DRIVE", 0700);
}

void initSegments(){
  std::ofstream outs[NO_SEGMENTS];
  for (int i = 0; i < NO_SEGMENTS; ++i){
    std::string filename = "DRIVE/SEGMENT" + std::to_string(i+1);
    outs[i].open(filename, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);

    char true_zero = 0;
    for (int j = 0; j < SEG_SIZE; ++j)
      outs[i] << true_zero;

    outs[i].close();
  }
}

void initCheckpointRegion(){
  std::ofstream out;
  out.open("DRIVE/CHECKPOINT_REGION", std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);

  unsigned int neg1 = -1;
  for (int i = 0; i < IMAP_BLOCKS; ++i)
    out.write(reinterpret_cast<const char*>(&neg1), 4);

  out.close();
}

void initFilenameMap(){
  std::ofstream out;
  out.open("DRIVE/FILEMAP", std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);

  char true_zero = 0;
  for (int i = 0; i < FILEMAP_BLOCK_SIZE * MAX_FILES; ++i)
    out << true_zero;

  out.close();
}

void makeTestFiles(){
  std::ofstream out1;
  out1.open("s", std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);

  for (int i = 0; i < BLOCK_SIZE/5; ++i)
    out1 << (char) (i % 256);

  out1.close();

  std::ofstream out2;
  out2.open("b", std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);

  for (int i = 0; i < BLOCK_SIZE*120; ++i)
    out2 << (char) (i % 256);

  out2.close();
}

int main(int argc, char const *argv[]){
  makeDriveDir();
  initSegments();
  initCheckpointRegion();
  initFilenameMap();
  makeTestFiles(); // shortcutplsdelete
  printf("%s\n", "Drive created.");
  return 0;
}
