#ifndef NEXOS_DIRENT_H
#define NEXOS_DIRENT_H
typedef struct nexos_DIR DIR;
struct dirent { unsigned long d_ino; unsigned short d_reclen; char d_name[256]; };
DIR *opendir(const char *);
struct dirent *readdir(DIR *);
int closedir(DIR *);
#endif
