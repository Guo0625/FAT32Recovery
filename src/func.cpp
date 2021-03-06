#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "entry.h"

char *devname, *recfname;

char checkargs(int argc, char **argv) {
	if (argc < 4) {
		devname = recfname = NULL;
		return 0;
	}
	bool d, i, l, r;
	d = i = l = r = false;
	for (int j = 1; j < argc; j++) {
		if (!d && strcmp(argv[j], "-d") == 0) {
			if (++j >= argc) {
				devname = recfname = NULL;
				return 0;
			}
			d = true;
			devname = argv[j];
		} else if (!i && strcmp(argv[j], "-i") == 0) {
			i = true;
		} else if (!l && strcmp(argv[j], "-l") == 0) {
			l = true;
		} else if (!r && strcmp(argv[j], "-r") == 0) {
			if (++j >= argc) {
				devname = recfname = NULL;
				return 0;
			}
			r = true;
			recfname = argv[j];
		} else {
			devname = recfname = NULL;
			return 0;
		}
	}
	if (!d || (i && l) || (i && r) || (l && r)) {
		devname = recfname = NULL;
		return 0;
	}
	if (i) {
		return 'i';
	}
	if (l) {
		return 'l';
	}
	if (r) {
		return 'r';
	}
	return 0;
}

void info(BootEntry &be) {
	printf("Number of FATs = %hhu\n", be.BPB_NumFATs);
	printf("Number of bytes per sector = %hu\n", be.BPB_BytsPerSec);
	printf("Number of sectors per cluster = %hhu\n", be.BPB_SecPerClus);
	printf("Number of reserved sectors = %hu\n", be.BPB_RsvdSecCnt);
}

int parsefname(char fname[13], const unsigned char DIR_Name[11]) {
	int i, j;
	for (i = 0; i < 8 && DIR_Name[i] != ' '; i++) {
		fname[i] = DIR_Name[i];
	}
	if (DIR_Name[8] != ' ') {
		fname[i++] = '.';
		for (j = 8; j <= 10 && DIR_Name[j] != ' '; j++, i++) {
			fname[i] = DIR_Name[j];
		}
	}
	fname[i] = 0;
	return i;
}

void list(FILE *dev, BootEntry &be, unsigned int *fat, unsigned int dirclus, const char *pardir, int &ind) {
	unsigned int fatsize = be.BPB_FATSz16 + be.BPB_FATSz32;
	unsigned int clus0sec = be.BPB_RsvdSecCnt + fatsize * be.BPB_NumFATs - 2 * be.BPB_SecPerClus;
	unsigned int clussize = be.BPB_SecPerClus * be.BPB_BytsPerSec, delength = clussize / sizeof (DirEntry);
	DirEntry *de = new DirEntry[delength];
	unsigned int i, startclus;
	int fnamelen, pardirlen = strlen(pardir);
	char fname[13], *currdir = new char[pardirlen + 10];
	memcpy(currdir, pardir, pardirlen);
	for (unsigned int clus = dirclus & 0x0fffffff; clus && clus < EOC_LO; clus = fat[clus] & 0x0fffffff) {
		/*		for (long tmp = (long) (clus0sec + clus * be.BPB_SecPerClus) * be.BPB_BytsPerSec - 65536; tmp < (long) (clus0sec + clus * be.BPB_SecPerClus) * be.BPB_BytsPerSec + 65535; tmp++) {
					fseek(dev, tmp, SEEK_SET);
					fread(de, sizeof(DirEntry), 1, dev);
					de->DIR_Name[10] = 0;
					printf("%ld: %s\n\n", tmp, de->DIR_Name);
				}*/
		fseek(dev, (long) (clus0sec + clus * be.BPB_SecPerClus) * be.BPB_BytsPerSec, SEEK_SET);
		fread(de, sizeof (DirEntry), delength, dev);
		for (i = 0; i < delength; i++) {
			if (de[i].DIR_Name[0] != 0 && de[i].DIR_Name[0] != 0xe5 && de[i].DIR_Attr != 0x0f) {
				startclus = (((unsigned int) de[i].DIR_FstClusHI << 16) + de[i].DIR_FstClusLO) & 0x0fffffff;
				/*				for (j = 0; j < 8 && de[i].DIR_Name[j] != ' '; j++) {
									fname[j] = de[i].DIR_Name[j];
								}*/
//				printf("%11s\n", de[i].DIR_Name);
				fnamelen = parsefname(fname, de[i].DIR_Name);
				if (de[i].DIR_Attr & 0b00010000) {
					if (fname[0] != '.') {
						fname[fnamelen++] = '/';
						fname[fnamelen] = 0;
					}
					printf("%d, %s%s, %u, %u\n", ind++, pardir, fname, de[i].DIR_FileSize, startclus);
					if (fname[0] != '.') {
						memcpy(currdir + pardirlen, fname, fnamelen + 1);
						list(dev, be, fat, startclus, currdir, ind);
					}
				} else {
					/*					if (de[i].DIR_Name[8] != ' ') {
											fname[j++] = '.';
											for (k = 8; k <= 10 && de[i].DIR_Name[k] != ' '; k++, j++) {
												fname[j] = de[i].DIR_Name[k];
											}
										}
										fname[j] = 0;*/
					printf("%d, %s%s, %u, %u\n", ind++, pardir, fname, de[i].DIR_FileSize, startclus);
				}
			}
		}
	}
	delete[] de;
	delete[] currdir;
}

void touppercase(char *str) {
	for (int i = 0; str[i]; i++) {
		if (str[i] >= 'a' && str[i] <= 'z') {
			str[i] -= ('a' - 'A');
		}
	}
}

bool recover(FILE *dev, BootEntry &be, unsigned int *fat, unsigned int dirclus, const char *pardir, char *tarname) {
	unsigned int fatsize = be.BPB_FATSz16 + be.BPB_FATSz32;
	unsigned int clus0sec = be.BPB_RsvdSecCnt + fatsize * be.BPB_NumFATs - 2 * be.BPB_SecPerClus;
	unsigned int clussize = be.BPB_SecPerClus * be.BPB_BytsPerSec, delength = clussize / sizeof (DirEntry);
	DirEntry *de = new DirEntry[delength];
	unsigned int i, j, startclus, totalclus;
	int fnamelen, pardirlen = strlen(pardir);
	char *currdir = new char[pardirlen + 10], fname[13];
	memcpy(currdir, pardir, pardirlen);
	for (unsigned int clus = dirclus & 0x0fffffff; clus && clus < EOC_LO; clus = fat[clus] & 0x0fffffff) {
		fseek(dev, (long) (clus0sec + clus * be.BPB_SecPerClus) * be.BPB_BytsPerSec, SEEK_SET);
		fread(de, sizeof (DirEntry), delength, dev);
		for (i = 0; i < delength; i++) {
			if (de[i].DIR_Name[0] != 0 && de[i].DIR_Attr != 0x0f) {
				if (de[i].DIR_Name[0] == 0xe5) {
//					printf("%11s\n", de[i].DIR_Name);
					startclus = (((unsigned int) de[i].DIR_FstClusHI << 16) + de[i].DIR_FstClusLO) & 0x0fffffff;
					fnamelen = parsefname(fname, de[i].DIR_Name);
					if (strcmp(fname + 1, tarname + 1) == 0) {
						fseek(dev, (long) (clus0sec + clus * be.BPB_SecPerClus) * be.BPB_BytsPerSec + i * sizeof (DirEntry), SEEK_SET);
						fwrite(tarname, 1, 1, dev);
						if (!de[i].DIR_FileSize) {
							printf("%s: recovered in %s\n", tarname, pardir);
							delete[] de;
							delete[] currdir;
							return true;
						}
						totalclus = (de[i].DIR_FileSize - 1) / clussize + 1;
						for (j = startclus; j - startclus < totalclus; j++) {
							if (fat[j] & 0x0fffffff) {
								printf("%s: error - fail to recover\n", tarname);
								delete[] de;
								delete[] currdir;
								return false;
							}
						}
						for (j = startclus; j - startclus < totalclus - 1; j++) {
							fat[j] = (j + 1) | (fat[j] & 0xf0000000);
						}
						fat[j] = EOC_HI | (fat[j] & 0xf0000000);
						for (j = 0; j < be.BPB_NumFATs; j++) {
							fseek(dev, (be.BPB_RsvdSecCnt + j * (long) fatsize) * be.BPB_BytsPerSec + startclus * 4, SEEK_SET);
							fwrite(fat + startclus, 4, totalclus, dev);
						}
						printf("%s: recovered in %s\n", tarname, pardir);
						delete[] de;
						delete[] currdir;
						return true;
					}
				} else if (de[i].DIR_Attr & 0b00010000) {
					startclus = (((unsigned int) de[i].DIR_FstClusHI << 16) + de[i].DIR_FstClusLO) & 0x0fffffff;
					fnamelen = parsefname(fname, de[i].DIR_Name);
					if (fname[0] != '.') {
						fname[fnamelen++] = '/';
						fname[fnamelen] = 0;
						memcpy(currdir + pardirlen, fname, fnamelen + 1);
						if (recover(dev, be, fat, startclus, currdir, tarname)) {
							delete[] de;
							delete[] currdir;
							return true;
						}
					}
				}
			}
		}
	}
	if (dirclus == be.BPB_RootClus) {
		printf("%s: error - file not found\n", tarname);
	}
	delete[] de;
	delete[] currdir;
	return false;
}

