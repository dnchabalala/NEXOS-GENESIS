#ifndef NEXOS_CTYPE_H
#define NEXOS_CTYPE_H
static inline int isspace(int c) { return c == ' ' || (c >= '\t' && c <= '\r'); }
static inline int isalnum(int c) { return (c >= '0' && c <= '9') ||
    (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
static inline int isascii(int c) { return (unsigned)c < 128U; }
static inline int tolower(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }
#endif
