#include "commands.cpp"

int shortcut_filename_ext = 0;

void restart(){
  readInCheckpointRegion();
  findNextAvailableBlock();
  readInSegment();
  readInImap();
}

void test(){
  for (int i = 0; i < MAX_FILES/3; ++i)
    import("makefile", "file"+std::to_string(i));
}

void printSegmentSummary(std::string amount_string, std::string start_string){
  int start = std::stoi(start_string);
  int end = (std::stoi(amount_string) + start < ASSIGNABLE_BLOCKS) ? std::stoi(amount_string) + start : ASSIGNABLE_BLOCKS;
  for (int i = start; i < end; ++i)
    printf("SUMMARY[%d]: {%u, %u}\n", i, SEGMENT_SUMMARY[i][0], SEGMENT_SUMMARY[i][1]);
}

void mimport(std::string filename, std::string lfs_filename, std::string amount_string) {
  int amount = (std::stoi(amount_string) < MAX_FILES) ? std::stoi(amount_string) : MAX_FILES;
  for (int i = 0; i < amount; ++i)
    import(filename, lfs_filename+std::to_string(i));
}

void printImap(std::string amount_string, std::string start_string){
  int start = std::stoi(start_string);
  int end = (std::stoi(amount_string) + start < MAX_FILES) ? std::stoi(amount_string) + start : MAX_FILES;
  for (int i = start; i < end; ++i)
    printf("IMAP[%d]: %u\n", i, IMAP[i]);
}

void printCheckpointRegion(){
  for (int i = 0; i < IMAP_BLOCKS; ++i)
    printf("CPR[%d]: %u\n", i, CHECKPOINT_REGION[i]);
}

void printCleans(){
  printf("Clean segments: ");
  for (int i = 0; i < NO_SEGMENTS; ++i){
    if (CLEAN_SEGMENTS[i] == CLEAN)
      printf("%d ", i+1);
  }
  printf("\n");
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
  else if (command[0] == "clean"     && command.size() == 2) clean(command[1]);
  else if (command[0] == "exit"      && command.size() == 1) exit();
  else if (command[0] == "b"         && command.size() == 1) import("b", "file"+std::to_string(shortcut_filename_ext++));
  else if (command[0] == "s"         && command.size() == 1) import("s", "file"+std::to_string(shortcut_filename_ext++));
  else if (command[0] == "r"         && command.size() == 1) import("makefile", "file"+std::to_string(shortcut_filename_ext++));
  else if (command[0] == "a"         && command.size() == 1) import("lorem.txt", "file"+std::to_string(shortcut_filename_ext++));
  else if (command[0] == "ls"        && command.size() == 1) list();
  else if (command[0] == "test"      && command.size() == 1) test();
  else if (command[0] == "segmentno" && command.size() == 1) printf("Current Segment: %d\n", SEGMENT_NO);
  else if (command[0] == "summary"   && command.size() == 3) printSegmentSummary(command[1], command[2]);
  else if (command[0] == "mimport"   && command.size() == 4) mimport(command[1], command[2], command[3]);
  else if (command[0] == "imap"      && command.size() == 3) printImap(command[1], command[2]);
  else if (command[0] == "cpr"       && command.size() == 1) printCheckpointRegion();
  else if (command[0] == "cleans"    && command.size() == 1) printCleans();
  else if (command[0] == "nextblock" && command.size() == 1) printf("Next Available Block: %u\n", AVAILABLE_BLOCK);
  else if (command[0] == "clear"     && command.size() == 1) std::system("clear");
  else std::cout << "Command not recognized." << std::endl;
}

int main(int argc, char* argv[]){
  struct stat st = {0};

  if (stat("./DRIVE", &st) == -1){
    std::cout << "Could not find DRIVE. Please run 'make drive' and try again." << std::endl;
    return 1;
  }

  restart();

  std::string line;

  std::cout << "LFS$ ";
  while (getline(std::cin, line)){
    parseLine(line);
    std::cout << "LFS$ ";
  }

  return 0;
}
