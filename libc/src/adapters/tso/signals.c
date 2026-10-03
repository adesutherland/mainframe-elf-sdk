/* Synchronous ISO C signal operations for the single-thread application.
   These do not install MVS program-interruption or asynchronous OS handlers.
   The existing abort hook retains its separately documented limited contract. */
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "services.h"

typedef void (*lab_signal_handler)(int);
static const int supported[] = {SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM};
static lab_signal_handler handlers[sizeof(supported) / sizeof(supported[0])];

static int slot(int number)
{
    unsigned i;
    for (i = 0; i < sizeof(supported) / sizeof(supported[0]); ++i)
        if (number == supported[i]) return (int)i;
    return -1;
}

lab_signal_handler signal(int number, lab_signal_handler handler)
{
    int i = slot(number);
    lab_signal_handler previous;
    if (i < 0 || handler == SIG_ERR) { errno = EINVAL; return SIG_ERR; }
    previous = handlers[i];
    handlers[i] = handler;
    return previous;
}

int raise(int number)
{
    int i = slot(number);
    lab_signal_handler handler;
    if (i < 0) { errno = EINVAL; return -1; }
    handler = handlers[i];
    if (handler == SIG_IGN) return 0;
    if (handler == SIG_DFL) {
        lab_tso_services->putline("UNHANDLED C SIGNAL", 18, 0);
        _exit(128 + number);
    }
    handlers[i] = SIG_DFL;
    handler(number);
    return 0;
}
