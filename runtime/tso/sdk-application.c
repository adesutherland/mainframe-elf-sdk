/* Single synchronous TSO CALL. Newlib owns normal exit/atexit/FILE cleanup. */
#include <stdlib.h>
#include <unistd.h>
#include "services.h"
#include "text-codec.h"
#ifndef LAB_TSO_PROGRAM
#define LAB_TSO_PROGRAM "tso-c"
#endif
const LabTsoServices *lab_tso_services;
extern int main(int, char **);

void _exit(int status)
{
    if (lab_tso_io_finish() && !status) status=16;
    lab_tso_services->finish(status);
    __builtin_unreachable();
}
void abort(void)
{
    lab_tso_services->putline("C ABORT",7,0);
    _exit(12);
}

/* TSO passes a length-prefixed EBCDIC tail. Quoting preserves embedded
   spaces; a doubled quote inside a quoted argument denotes that quote. */
__attribute__((section(".text.entry")))
int lab_tso_start(const unsigned char *raw, unsigned length,
                  const LabTsoServices *services)
{
    char text[8192], *argv[34];
    unsigned used=0, pos=0;
    int argc=1;
#ifdef __MAINFRAME_LAB_TSO64__
    if (!services || (services->version!=0x6401U &&
                      services->version!=0x6402U &&
                      services->version!=LAB_TSO_SERVICE_VERSION)) return 12;
#else
    if (!services || (services->version!=3 && services->version!=4 &&
                      services->version!=5 &&
                      services->version!=LAB_TSO_SERVICE_VERSION)) return 12;
#endif
    lab_tso_services=services;
    if (length>4095) return 24;
    for (unsigned i=0;i<length;++i) {
        unsigned char bytes[2];
        unsigned n=lab_1047_utf8(raw[i],bytes);
        if (!bytes[0]) return 24;
        for (unsigned j=0;j<n;++j) text[used++]=(char)bytes[j];
    }
    text[used]=0;
    argv[0]=LAB_TSO_PROGRAM;
    used=0;
    while (text[pos]) {
        int quote=0;
        while (text[pos]==' ' || text[pos]=='\t') ++pos;
        if (!text[pos]) break;
        if (argc==33) return 24;
        argv[argc++]=text+used;
        while (text[pos]) {
            int c=(unsigned char)text[pos++];
            if (quote) {
                if (c==quote) {
                    if (text[pos]==quote) ++pos;
                    else { quote=0; continue; }
                }
            } else {
                if (c==' ' || c=='\t') break;
                if (c=='\'' || c=='"') { quote=c; continue; }
            }
            text[used++]=(char)c;
        }
        if (quote) return 24;
        text[used++]=0;
    }
    argv[argc]=0;
    exit(main(argc,argv));
}
