/* Snapshot the pinned CMS FST lookup before yielding to application code.
   No LISTFILE command, shared temporary file or persistent nucleus cursor. */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "adapter.h"
#include "application.h"
#include "dirent.h"
#ifndef LAB_CMS_MAX_DIRECTORY_ENTRIES
#define LAB_CMS_MAX_DIRECTORY_ENTRIES 1024
#endif
int lab_cms_fst_lookup(void *plist, unsigned long state[2], int resume);
struct LabCmsDir { struct dirent *entries; unsigned count, next; };

static int filemode(const char *path)
{
    if (!path || !strcmp(path, "") || !strcmp(path, ".") ||
        !strcmp(path, "./")) return 1;
    if ((path[0]=='A' || path[0]=='a') &&
        (path[1]=='1' || path[1]=='2' || path[1]=='5') &&
        (!path[2] || (path[2]=='/' && !path[3])))
        return path[1]-'0';
    return 0;
}

static int field(char *out,const unsigned char *in)
{
    unsigned i=0;
    for (;i<8 && in[i]!=0x40;++i) {
        int c=lab_ebcdic_to_ascii(in[i]);
        if (!((c>='A' && c<='Z') || (c>='0' && c<='9'))) return -1;
        out[i]=(char)(c>='A' && c<='Z' ? c+'a'-'A' : c);
    }
    if (!i) return -1;
    for (unsigned j=i;j<8;++j) if (in[j]!=0x40) return -1;
    return (int)i;
}
DIR *opendir(const char *path)
{
    /* A1, A2 and A5 are stable filemode collections on the accessed A disk.
       The current directory remains A1; no host path hierarchy is implied. */
    int mode=filemode(path);
    if (!mode) { errno=ENOTDIR; return NULL; }
    DIR *d=calloc(1,sizeof *d);
    if (!d) { errno=ENOMEM; return NULL; }
    unsigned char plist[32];
    memset(plist,0x40,sizeof plist);
    plist[8]=plist[16]=0x5c; /* '*' */
    plist[24]=0xc1; plist[25]=(unsigned char)(0xf0+mode);
    unsigned char fst_copy[40];
    int state_rc=lab_cms_state(plist+8,fst_copy);
    if (state_rc && state_rc!=28) {
        free(d); errno=state_rc==36 ? ENODEV : EIO; return NULL;
    }
    unsigned long cursor[2]={0,0};
    unsigned capacity=0;
    int resume=0;
    for (;;) {
        int rc=lab_cms_fst_lookup(plist,cursor,resume);
        if (rc==1) break;
        if (rc) { errno=EIO; goto failed; }
        resume=1;
        const unsigned char *fst=(const unsigned char *)cursor[1];
        struct dirent entry;
        int n=field(entry.d_name,fst), t;
        if (n<0) continue; /* Outside the profile's alphanumeric name domain. */
        entry.d_name[n++]='.';
        t=field(entry.d_name+n,fst+8);
        if (t<0) continue;
        entry.d_name[n+t]=0;
        if (d->count==LAB_CMS_MAX_DIRECTORY_ENTRIES) { errno=EOVERFLOW; goto failed; }
        if (d->count==capacity) {
            unsigned next=capacity ? capacity*2 : 16;
            if (next>LAB_CMS_MAX_DIRECTORY_ENTRIES) next=LAB_CMS_MAX_DIRECTORY_ENTRIES;
            struct dirent *p=realloc(d->entries,next*sizeof *p);
            if (!p) { errno=ENOMEM; goto failed; }
            d->entries=p; capacity=next;
        }
        d->entries[d->count++]=entry;
    }
    return d;
failed:
    free(d->entries); free(d); return NULL;
}
struct dirent *readdir(DIR *d)
{
    if (!d) { errno=EBADF; return NULL; }
    return d->next<d->count ? &d->entries[d->next++] : NULL;
}
int closedir(DIR *d)
{
    if (!d) { errno=EBADF; return -1; }
    free(d->entries); free(d); return 0;
}
