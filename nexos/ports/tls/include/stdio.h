#ifndef NEXOS_TLS_STDIO_H
#define NEXOS_TLS_STDIO_H
#include <stddef.h>
#include <stdarg.h>
#define EOF (-1)
typedef struct nexos_tls_FILE FILE;
#define stderr ((FILE *)0)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
FILE *fopen(const char *, const char *);
int fclose(FILE *);
char *fgets(char *, int, FILE *);
int fseek(FILE *, long, int);
long ftell(FILE *);
size_t fread(void *, size_t, size_t, FILE *);
int feof(FILE *);
int fprintf(FILE *, const char *, ...);
int vfprintf(FILE *, const char *, va_list);
int fputc(int, FILE *);
int fputs(const char *, FILE *);
int fflush(FILE *);
int snprintf(char *, size_t, const char *, ...);
int vsnprintf(char *, size_t, const char *, __builtin_va_list);
int printf(const char *, ...);
int sscanf(const char *, const char *, ...);
#endif
