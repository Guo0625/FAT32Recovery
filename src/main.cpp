#include <cstdio>
#include <cstring>
#include "entry.h"
#include "func.h"

using namespace std;

int main(int argc, char **argv) {
	if (!checkargs(argc, argv)) {
		printf("Usage: %s -d [device filename] [other arguments]\n", argv[0]);
		puts("-i                    Print boot sector information");
		puts("-l                    List all the directory entries");
		puts("-r filename           File recovery");
		return 0;
	}
	FILE *dev;
	BootEntry be;
	if ((dev = fopen(argv[2], "r+")) == NULL) {
		perror(argv[2]);
		return 1;
	}
	fread(&be, sizeof(BootEntry), 1, dev);
	if (strcmp(argv[3], "-i") == 0) {
		info(be);
		return 0;
	}
	unsigned int fatsize = be.BPB_FATSz32 * be.BPB_BytsPerSec / 4, *fat = new unsigned int[fatsize];
	fseek(dev, (long) be.BPB_RsvdSecCnt * be.BPB_BytsPerSec, SEEK_SET);
	fread(fat, 4, fatsize, dev);
	for (int i = 0; i < 30; i++) {
		printf("%d: %x\n", i, fat[i]);
	}
	if (strcmp(argv[3], "-l") == 0) {
		int ind = 1;
		list(dev, be, fat, be.BPB_RootClus, "", ind);
		return 0;
	}
	if (strcmp(argv[3], "-r") == 0) {
		touppercase(argv[4]);
		recover(dev, be, fat, be.BPB_RootClus, "/", argv[4]);
	}
	return 0;
}
