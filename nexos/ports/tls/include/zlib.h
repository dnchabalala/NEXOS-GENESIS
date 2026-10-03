#ifndef NEXOS_ZLIB_H
#define NEXOS_ZLIB_H
typedef struct nexos_gz_file *gzFile;
typedef unsigned char Bytef;
typedef unsigned int uInt;
typedef unsigned long uLong;
typedef struct z_stream_s {
    Bytef *next_in;
    uInt avail_in;
    Bytef *next_out;
    uInt avail_out;
    void *zalloc;
    void *zfree;
    void *opaque;
} z_stream;
#define Z_NULL ((void *)0)
#define MAX_WBITS 15
#define Z_OK 0
#define Z_STREAM_END 1
#define Z_NO_FLUSH 0
int inflateInit2(z_stream *, int);
int inflate(z_stream *, int);
int inflateEnd(z_stream *);
gzFile gzopen(const char *, const char *);
char *gzgets(gzFile, char *, int);
int gzclose(gzFile);
#endif
