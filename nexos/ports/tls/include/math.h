#ifndef NEXOS_MATH_H
#define NEXOS_MATH_H
static inline float nexos_floorf(float x) {
    int i = (int)x;
    return (x < (float)i) ? (float)(i - 1) : (float)i;
}
static inline float nexos_ceilf(float x) {
    int i = (int)x;
    return (x > (float)i) ? (float)(i + 1) : (float)i;
}
#define floorf nexos_floorf
#define ceilf nexos_ceilf
static inline double nexos_floord(double x) {
    long i = (long)x;
    return (x < (double)i) ? (double)(i - 1) : (double)i;
}
static inline double nexos_ceild(double x) {
    long i = (long)x;
    return (x > (double)i) ? (double)(i + 1) : (double)i;
}
#define floor nexos_floord
#define ceil nexos_ceild
static inline float nexos_fabsf(float x) { return x < 0.0f ? -x : x; }
static inline double nexos_fabs(double x) { return x < 0.0 ? -x : x; }
#define fabsf nexos_fabsf
#define fabs nexos_fabs
static inline int isnan(double x) { return x != x; }
#endif
