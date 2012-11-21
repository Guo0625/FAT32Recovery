#ifndef FUNC_H
#define	FUNC_H

bool checkargs(int argc, char **argv);
void info(BootEntry &be);
void list(FILE *dev, BootEntry &be, unsigned int *fat, unsigned int dirclus, const char *pardir, int &ind);
void touppercase(char *str);
bool recover(FILE *dev, BootEntry &be, unsigned int *fat, unsigned int dirclus, const char *pardir, char *fname);

#endif

