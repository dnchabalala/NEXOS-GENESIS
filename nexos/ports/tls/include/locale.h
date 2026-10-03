#ifndef NEXOS_LOCALE_H
#define NEXOS_LOCALE_H

/* NetSurf only uses locale initialisation to select the process locale.
 * NexOS currently provides the C/POSIX byte-oriented locale, with no hosted
 * locale database or mutable process environment. */
#define LC_ALL 0
static inline char *setlocale(int category, const char *locale)
{
    (void)category;
    (void)locale;
    return (char *)"C";
}

#endif
