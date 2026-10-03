/* A bounded hosted lifecycle for this CMS application profile. Newlib exit
   owns atexit and stdio cleanup; _exit supplies the actual CMS return. There
   is no POSIX process model or installed SIGABRT handler contract here. */
#include <stdlib.h>
#include <unistd.h>
#include "adapter.h"
#include "application.h"

extern int main(int, char **);
extern int lab_newlib_finish(void);
extern unsigned lab_newlib_heap_used(void);
extern unsigned lab_newlib_heap_failures(void);
extern unsigned lab_newlib_heap_failed_request(void);
extern unsigned lab_application_observed[5];

void _exit(int status)
{
    /* Close CMS records and deliver bytes already handed to _write. Do not
       flush C FILE buffers or invoke atexit here: exit has already done so. */
    if (lab_newlib_finish() && !status) status = 16;
    lab_application_observed[2] = lab_newlib_heap_used();
    lab_application_observed[3] = lab_newlib_heap_failures();
    lab_application_observed[4] = lab_newlib_heap_failed_request();
    lab_cms_return(status);
}

void abort(void)
{
    lab_cms_line("C ABORT", 7);
    _exit(12);
}

void lab_cms_start(const unsigned char *plist)
{
    char text[LAB_CMS_MAX_ARGS][9];
    char *argv[LAB_CMS_MAX_ARGS + 1];
    int argc = lab_cms_arguments(plist, text, argv);
    if (argc < 1) {
        lab_cms_line("CMS ARGUMENT ERROR", 18);
        _exit(24);
    }
    exit(main(argc, argv));
}
