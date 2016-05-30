# log-structured-file-system

## CS350 Program 2 (Lab 7)

Log-structured file system simulation implemented in C++.

###### See PDF for more details on the project description.

### Group members:
 * Annika Wiesinger
 * Charlie DiGiovanna

#### How to run lab 7:
1. make drive (if first time)
2. make
3. ./Lab7

###### OR

1. make again (to start with a fresh drive)
2. ./Lab7

### Commands:
 * import \<filename> \<lfs_filename>
 	* Import textfile \<filename> and save under the new name \<lfs_filename>
 * remove \<lfs_filename>.
 	* Remove the file with name \<lfs_filename>.
 * cat \<lfs_filename>
  	* Display the contens of \<lfs_filename>.
 * display \<lfs_filename> \<howmany> \<start>
  	* Display \<howmany> bytes of \<lfs_filename> starting from byte \<start>.
 * overwrite \<lfs_filename> \<howmany> \<start> \<char>
  	* Write \<howmany> copies of the character \<char> into file \<lfs_filename> beginning at byte \<start>. If this exceeds the original size of the file, the overwrite will grow the file.
 * list _or_ ls
  	* List all the names and sizes of the files.
 * clean \<how-many>
  	* Take \<how-many> dirty segments and clean them.
 * exit
  	* Write out cached memory to disk and exit.
 * segmentno
  	* Print current active segment number.
 * summary \<how-many> \<start>
  	* Print \<how-many> unsigned int's of the segment summary of the active segment starting from \<start>.
 * mimport \<filename> \<lfs_filename> \<how-many>
  	* Call "import \<filename> \<lfs_filename>" \<how-many> times.
 * imap \<how-many> \<start>
  	* Print \<how-many unsigned int's from the imap of the active segment starting from \<start>.
 * cpr
  	* Print the contents of the checkpoint region.
 * cleans
  	* Print the clean segments remaining.
 * nextblock
  	* Print the next available block on the active segment (local index).
 * clear
  	* Clear the LFS shell.

### Thanks!
