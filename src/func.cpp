#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "entry.h"

bool checkargs(int argc, char **argv) {
	if (argc < 4) {
		return false;
	}
	if (strcmp(argv[1], "-d") == 0) {
		if (argc == 4 && (strcmp(argv[3], "-i") == 0 || strcmp(argv[3], "-l") == 0)) {
			return true;
		}
		if (argc == 5 && strcmp(argv[3], "-r") == 0) {
			return true;
		}
		return false;
	}
	return false;
}

void info(BootEntry &be) {
	printf("Number of FATs = %hhu\n", be.BPB_NumFATs);
	printf("Number of bytes per sector = %hu\n", be.BPB_BytsPerSec);
	printf("Number of sectors per cluster = %hhu\n", be.BPB_SecPerClus);
	printf("Number of reserved sectors = %hu\n", be.BPB_RsvdSecCnt);
}

bool eoc(unsigned int clus) {
	clus &= 0x0fffffff;
	return clus >= EOC_LO && clus <= EOC_HI;
}

void list(FILE *dev, BootEntry &be, int *fat, unsigned int dirclus, const char *pardir, int &ind) {
	unsigned int clus0sec = be.BPB_RsvdSecCnt + (be.BPB_FATSz16 + be.BPB_FATSz32) * be.BPB_NumFATs;
	unsigned int clussize = be.BPB_SecPerClus * be.BPB_BytsPerSec, delength = clussize / sizeof(DirEntry);
	DirEntry *de = new DirEntry[delength];
	unsigned int i, j, k, startclus;
	char fname[13], *currdir;
	int fnamelen, pardirlen;
	for (unsigned int clus = dirclus; clus && !eoc(clus); clus = fat[clus]) {
/*		for (long tmp = (long) (clus0sec + clus * be.BPB_SecPerClus) * be.BPB_BytsPerSec - 65536; tmp < (long) (clus0sec + clus * be.BPB_SecPerClus) * be.BPB_BytsPerSec + 65535; tmp++) {
			fseek(dev, tmp, SEEK_SET);
			fread(de, sizeof(DirEntry), 1, dev);
			de->DIR_Name[10] = 0;
			printf("%ld: %s\n\n", tmp, de->DIR_Name);
		}*/
		fseek(dev, (long) (clus0sec + clus * be.BPB_SecPerClus - 2) * be.BPB_BytsPerSec, SEEK_SET);
		fread(de, sizeof(DirEntry), delength, dev);
		for (i = 0; i < delength; i++) {
			for (j = 0; j < 8 && de[i].DIR_Name[j] != ' '; j++) {
				fname[j] = de[i].DIR_Name[j];
			}
			if (de[i].DIR_Name[0] != 0 && de[i].DIR_Name[0] != 0xe5 && de[i].DIR_Attr != 0x0f) {
				startclus = ((unsigned int) de[i].DIR_FstClusHI << 16) + de[i].DIR_FstClusLO;
				if (de[i].DIR_Attr & 0b00010000) {
					if (fname[0] != '.') {
						fname[j++] = '/';
					}
					fname[j] = 0;
					printf("%d, %s%s, %u, %u\n", ind++, pardir, fname, de[i].DIR_FileSize, startclus);
					if (fname[0] != '.') {
						fnamelen = strlen(fname);
						pardirlen = strlen(pardir);
						currdir = (char *) malloc(pardirlen + fnamelen + 1);
						memcpy(currdir, pardir, pardirlen);
						memcpy(currdir + pardirlen, fname, fnamelen);
						currdir[pardirlen + fnamelen] = 0;
						list(dev, be, fat, startclus, currdir, ind);
					}
				} else {
					if (de[i].DIR_Name[8] != ' ') {
						fname[j++] = '.';
						for (k = 8; k <= 10 && de[i].DIR_Name[k] != ' '; k++, j++) {
							fname[j] = de[i].DIR_Name[k];
						}
					}
					fname[j] = 0;
					printf("%d, %s%s, %u, %u\n", ind++, pardir, fname, de[i].DIR_FileSize, startclus);
				}
			}
		}
	}
}
