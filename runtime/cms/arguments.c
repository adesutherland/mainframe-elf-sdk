/* CMS's tokenized PLIST has eight-byte EBCDIC words and an FF sentinel.
   This first application contract uses the ordinary PLIST only: no quoting,
   extended parameter list, case restoration or tokens longer than 8 bytes.
   Protocol reference: retained public-domain GCCLIB cmsruntm.c / cmsentry. */
#include "application.h"
#include "adapter.h"

int lab_cms_arguments(const unsigned char *plist,
                      char text[LAB_CMS_MAX_ARGS][9], char **argv)
{
    if (!plist) return -1;
    for (unsigned n = 0; n <= LAB_CMS_MAX_ARGS; ++n) {
        if (plist[n * 8] == 255) { argv[n] = 0; return (int)n; }
        if (n == LAB_CMS_MAX_ARGS) return -1;
        unsigned j = 0;
        for (; j < 8 && plist[n * 8 + j] != 0x40; ++j) {
            int c = lab_ebcdic_to_ascii(plist[n * 8 + j]);
            if (c < 33 || c > 126) return -1;
            text[n][j] = (char)c;
        }
        if (!j) return -1;
        for (unsigned k = j; k < 8; ++k)
            if (plist[n * 8 + k] != 0x40) return -1;
        text[n][j] = 0;
        argv[n] = text[n];
    }
    return -1;
}

/* Accept NAME TYPE MODE, ./NAME.TYPE[.MODE], or A1/A2/A5/NAME.TYPE. The
   filemode prefix maps compiler import roots to distinct CMS collections
   on the accessed A disk; it is not a host directory hierarchy. */
int lab_cms_path(const char *path, unsigned char id[18])
{
    char name[21];
    unsigned n = 0, dots = 0, root_mode = 0;
    if (!path) return -1;
    if (path[0] == '.' && path[1] == '/') path += 2;
    else if ((path[0] == 'A' || path[0] == 'a') &&
             (path[1] == '1' || path[1] == '2' || path[1] == '5') &&
             path[2] == '/') {
        root_mode = (unsigned)path[1];
        path += 3;
    }
    for (const char *p = path; *p; ++p)
        if (*p == ' ') return root_mode ? -1 : lab_cms_name(path, id);
    while (*path) {
        unsigned c = (unsigned char)*path++;
        if (n >= sizeof name - 1) return -1;
        if (c == '.') { if (++dots > 2) return -1; c = ' '; }
        name[n++] = (char)c;
    }
    if (dots == 1) {
        if (n + 3 >= sizeof name) return -1;
        name[n++] = ' '; name[n++] = 'A';
        name[n++] = (char)(root_mode ? root_mode : '1');
    } else if (dots != 2 || root_mode) return -1;
    name[n] = 0;
    return lab_cms_name(name, id);
}
