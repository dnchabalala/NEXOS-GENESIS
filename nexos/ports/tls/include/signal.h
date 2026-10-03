#ifndef NEXOS_SIGNAL_H
#define NEXOS_SIGNAL_H

#define SIGPIPE 13
#define SIG_IGN ((void (*)(int))1)
static inline void (*signal(int signum, void (*handler)(int)))(int)
{
    (void)signum;
    (void)handler;
    return handler;
}

#endif
