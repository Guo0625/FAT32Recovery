#ifndef FUNC_H
#define	FUNC_H

bool checkargs(int argc, char **argv);
void info(BootEntry &be);
void list(FILE *dev, BootEntry &be, int *fat, unsigned int dirclus, const char *pardir, int &ind);

#endif

