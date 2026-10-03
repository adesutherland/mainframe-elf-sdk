/* File metadata and mutations for the single-threaded CMS application profile.
   STATE/FSTD and RENAME protocols: pinned VM/370 sources DMSSTT, FSTD, DMSRNM.
   No implicit replacement, cross-disk moves, wildcard or directory semantics. */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "adapter.h"
#include "application.h"
#include "files.h"

static int fail(int error) { errno = error; return -1; }
static int cms_error(int rc)
{
    return fail(rc == 28 ? ENOENT : rc == 36 ? EACCES :
                rc == 20 || rc == 24 ? EINVAL : EIO);
}
static unsigned word(const unsigned char *p) { return (unsigned)p[0]*256+p[1]; }
static int decimal(unsigned bcd)
{
    if ((bcd >> 4) > 9 || (bcd & 15) > 9) return -1;
    return (int)(bcd >> 4)*10 + (int)(bcd & 15);
}
int lab_cms_fst_stat(const unsigned char fst[40], struct stat *st)
{
    /* The legacy FST stores MMDD/HHMM as packed decimal and YY as EBCDIC.
       Explicit window 1970..2069, minute precision, guest clock treated as UTC.
       Access/creation times are unavailable; zero is not a measured timestamp. */
    static const unsigned days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int month=decimal(fst[16]), day=decimal(fst[17]);
    int hour=decimal(fst[18]), minute=decimal(fst[19]);
    if (fst[38]<0xf0 || fst[38]>0xf9 || fst[39]<0xf0 || fst[39]>0xf9)
        return fail(EIO);
    unsigned year=(fst[38]-0xf0)*10+fst[39]-0xf0;
    year += year < 70 ? 2000 : 1900;
    unsigned leap=year%4==0 && (year%100!=0 || year%400==0);
    if (month<1 || month>12 || day<1 || day>(int)(days[month-1]+(month==2 && leap)) ||
        hour<0 || hour>23 || minute<0 || minute>59) return fail(EIO);
    int64_t elapsed=0;
    for (unsigned y=1970; y<year; ++y) elapsed+=365+(y%4==0 && (y%100!=0 || y%400==0));
    for (int m=1; m<month; ++m) elapsed+=days[m-1]+(m==2 && leap);
    elapsed=((elapsed+day-1)*24+hour)*60*60+minute*60;
    if ((int64_t)(time_t)elapsed!=elapsed) return fail(EOVERFLOW);
    memset(st,0,sizeof *st);
    st->st_mode=S_IFREG|0444|((fst[31]&0x80) ? 0222 : 0);
    st->st_nlink=1;
    st->st_blksize=256;
    st->st_mtime=(time_t)elapsed;
    return 0;
}
int stat(const char *path, struct stat *st)
{
    unsigned char id[18], fst[40];
    if (!st || lab_cms_path(path,id)) return fail(EINVAL);
    if (lab_newlib_file_busy(id)) return fail(EBUSY);
    int rc=lab_cms_state(id,fst);
    if (rc) return cms_error(rc);
    if (fst[31]&7) return fail(EBUSY);
    struct stat result;
    if (lab_cms_fst_stat(fst,&result)) return -1;
    if (fst[30]==0xc6) { /* Fixed records have an exact size without opening. */
        uint64_t length=(uint64_t)fst[32]<<24 | (uint64_t)fst[33]<<16 |
                        (unsigned)fst[34]<<8 | fst[35];
        length*=word(fst+26);
        if ((off_t)length<0 || (uint64_t)(off_t)length!=length) return fail(EOVERFLOW);
        result.st_size=(off_t)length;
    } else if (fst[30]==0xe5) {
        /* Variable-record counts/LRECL are not byte length. Read through the
           bounded stream adapter, honoring its error and open-file limits. */
        unsigned char bytes[256];
        FILE *f=fopen(path,"rb");
        if (!f) return -1;
        uint64_t length=0;
        size_t n;
        while ((n=fread(bytes,1,sizeof bytes,f))!=0) length+=n;
        int bad=ferror(f);
        if (fclose(f)!=0) bad=1;
        if (bad) return fail(EIO);
        if ((off_t)length<0 || (uint64_t)(off_t)length!=length) return fail(EOVERFLOW);
        result.st_size=(off_t)length;
    } else return fail(ENOSYS);
    *st=result;
    return 0;
}
int remove(const char *path)
{
    unsigned char id[18],fst[40];
    if (lab_cms_path(path,id)) return fail(EINVAL);
    if (lab_newlib_file_busy(id)) return fail(EBUSY);
    int rc=lab_cms_state(id,fst);
    if (rc) return cms_error(rc);
    if (fst[31]&7) return fail(EBUSY);
    rc=lab_cms_erase(id);
    return rc ? cms_error(rc) : 0;
}
int rename(const char *oldpath,const char *newpath)
{
    unsigned char oldid[18],newid[18],fst[40];
    if (lab_cms_path(oldpath,oldid) || lab_cms_path(newpath,newid)) return fail(EINVAL);
    if (memcmp(oldid+16,newid+16,2)) return fail(EXDEV);
    if (lab_newlib_file_busy(oldid) || lab_newlib_file_busy(newid)) return fail(EBUSY);
    int rc=lab_cms_state(oldid,fst);
    if (rc) return cms_error(rc);
    if (fst[31]&7) return fail(EBUSY);
    if (!memcmp(oldid,newid,18)) return 0;
    rc=lab_cms_state(newid,fst);
    if (!rc) return fail(EEXIST); /* Never erase the destination to fake replacement. */
    if (rc!=28) return cms_error(rc);
    rc=lab_cms_rename(oldid,newid);
    return rc ? cms_error(rc) : 0;
}
