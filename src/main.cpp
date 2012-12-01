#include <cstdio>
#include <cstring>
#include "entry.h"
#include "func.h"

using namespace std;

extern char *devname, *recfname;

int main(int argc, char **argv) {
	char op;
	if (!(op = checkargs(argc, argv))) {
		printf("Usage: %s -d [device filename] [other arguments]\n", argv[0]);
		puts("-i                    Print boot sector information");
		puts("-l                    List all the directory entries");
		puts("-r filename           File recovery");
		return 0;
	}
	FILE *dev;
	BootEntry be;
	if ((dev = fopen(devname, "r+")) == NULL) {
		perror(devname);
		return 1;
	}
	fread(&be, sizeof(BootEntry), 1, dev);
	if (op == 'i') {
		info(be);
		return 0;
	}
	unsigned int fatsize = (be.BPB_FATSz16 + be.BPB_FATSz32) * be.BPB_BytsPerSec / 4, *fat = new unsigned int[fatsize];
	fseek(dev, (long) be.BPB_RsvdSecCnt * be.BPB_BytsPerSec, SEEK_SET);
	fread(fat, 4, fatsize, dev);
/*	for (int i = 0; i < 30; i++) {
		printf("%d: 0x%x (%d)\n", i, fat[i], fat[i]);
	}*/
	if (op == 'l') {
		int ind = 1;
		list(dev, be, fat, be.BPB_RootClus, "", ind);
		delete[] fat;
		return 0;
	}
	if (op == 'r') {
		touppercase(recfname);
		recover(dev, be, fat, be.BPB_RootClus, "/", recfname);
		delete[] fat;
		return 0;
	}
	delete[] fat;
	return 0;
}
