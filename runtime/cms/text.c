/* Explicit plain-C CMS text FILE adapter, also used by cREXX. Native IBM1047
   records are the default. UTF-8 selection stores validated bytes without
   conversion. Binary fopen modes bypass this adapter. */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "adapter.h"
#include "application.h"
#include "text.h"
#include "text-codec.h"
#define MAX_LINE 32760U
#define TEXT_SLOTS 4
typedef struct {
    LabCmsFile file;
    FILE *raw;
    unsigned char id[18], *record, *decoded;
    unsigned capacity, used, position, available;
    int writing, utf8, failed, eof, records;
    LabUtf8 decoder;
} Text;
static Text *streams[TEXT_SLOTS];
static int selected_utf8;
extern int lab_newlib_file_busy(const unsigned char id[18]);
static int fail(Text *s,int error) { s->failed=error; errno=error; return -1; }
int lab_cms_text_encoding(const char *name)
{
    if (name && (!strcmp(name,"UTF8") || !strcmp(name,"utf8") ||
                 !strcmp(name,"UTF-8") || !strcmp(name,"utf-8"))) selected_utf8=1;
    else if (name && (!strcmp(name,"IBM1047") || !strcmp(name,"ibm1047"))) selected_utf8=0;
    else { errno=EINVAL; return -1; }
    return 0;
}
int lab_cms_text_busy(const unsigned char id[18])
{
    for (unsigned i=0;i<TEXT_SLOTS;++i)
        if (streams[i] && !streams[i]->utf8 && !memcmp(streams[i]->id,id,17)) return 1;
    return 0;
}
static int validate(Text *s,const unsigned char *bytes,unsigned n)
{
    for (unsigned i=0;i<n;++i) {
        unsigned scalar=0;
        int rc=lab_utf8_byte(&s->decoder,bytes[i],&scalar);
        if (rc<0 || (rc && !scalar)) return fail(s,EILSEQ);
    }
    return 0;
}
static int read_text(void *cookie,char *out,int count)
{
    Text *s=cookie;
    unsigned done=0;
    if (s->failed) return fail(s,s->failed);
    if (s->utf8) {
        size_t n=fread(out,1,(size_t)count,s->raw);
        if (ferror(s->raw)) return fail(s,EIO);
        if (validate(s,(const unsigned char *)out,(unsigned)n)) return -1;
        if (feof(s->raw) && s->decoder.remaining) return fail(s,EILSEQ);
        return (int)n;
    }
    while (done<(unsigned)count) {
        if (s->position==s->available) {
            int length=0, rc;
            if (s->eof) break;
            rc=lab_cms_read(&s->file,&length);
            if (rc==12) { s->eof=1; break; }
            if (rc || length<0 || (unsigned)length>s->capacity) return fail(s,EIO);
            s->position=s->available=0;
            for (int i=0;i<length;++i) {
                unsigned char bytes[2];
                unsigned n=lab_1047_utf8(s->record[i],bytes);
                if (!bytes[0]) return fail(s,EILSEQ);
                for (unsigned j=0;j<n;++j) s->decoded[s->available++]=bytes[j];
            }
            s->decoded[s->available++]='\n';
        }
        unsigned n=s->available-s->position;
        if (n>(unsigned)count-done) n=(unsigned)count-done;
        memcpy(out+done,s->decoded+s->position,n);
        s->position+=n; done+=n;
    }
    return (int)done;
}
static int emit_record(Text *s)
{
    unsigned length=s->used;
    /* CMS blank logical lines are represented by a one-blank record. */
    if (!length) { s->record[0]=0x40; length=1; }
    if (lab_cms_write(&s->file,(int)length)) return fail(s,EIO);
    s->records=1; s->used=0;
    return 0;
}
static int append_byte(Text *s,unsigned value)
{
    if (s->used==MAX_LINE) return fail(s,EOVERFLOW);
    if (s->used==s->capacity) {
        unsigned capacity=s->capacity*2;
        if (capacity>MAX_LINE) capacity=MAX_LINE;
        unsigned char *grown=realloc(s->record,capacity);
        if (!grown) return fail(s,ENOMEM);
        s->record=grown; s->capacity=capacity;
        s->file.buffer=grown; s->file.capacity=(int)capacity;
    }
    s->record[s->used++]=(unsigned char)value;
    return 0;
}
static int write_text(void *cookie,const char *input,int count)
{
    Text *s=cookie;
    const unsigned char *bytes=(const unsigned char *)input;
    if (s->failed) return fail(s,s->failed);
    if (s->utf8) {
        if (validate(s,bytes,(unsigned)count)) return -1;
        if (fwrite(input,1,(size_t)count,s->raw)!=(size_t)count) return fail(s,EIO);
        return count;
    }
    for (int i=0;i<count;++i) {
        unsigned scalar=0;
        int rc=lab_utf8_byte(&s->decoder,bytes[i],&scalar), encoded;
        if (rc<0 || (rc && !scalar)) return fail(s,EILSEQ);
        if (!rc) continue;
        if (scalar=='\n') { if (emit_record(s)) return -1; continue; }
        encoded=lab_unicode_1047(scalar);
        if (encoded<0) return fail(s,EILSEQ);
        if (append_byte(s,(unsigned)encoded)) return -1;
    }
    return count;
}
static int close_text(void *cookie)
{
    Text *s=cookie;
    int error=s->failed;
    if (s->writing && s->decoder.remaining) error=EILSEQ;
    if (s->raw) {
        if (fclose(s->raw) && !error) error=EIO;
    } else {
        if (s->writing && !error && (s->used || !s->records) && emit_record(s)) error=s->failed;
        if (s->file.record!=1 && lab_cms_close(&s->file) && !error) error=EIO;
    }
    for (unsigned i=0;i<TEXT_SLOTS;++i) if (streams[i]==s) streams[i]=0;
    free(s->decoded); free(s->record); free(s);
    if (error) { errno=error; return -1; }
    return 0;
}
FILE *lab_cms_text_open(const char *path,const char *mode)
{
    unsigned slot, cap=256;
    unsigned char id[18], fst[40];
    int writing, rc;
    if (!mode || (strcmp(mode,"r") && strcmp(mode,"w"))) { errno=ENOSYS; return 0; }
    writing=*mode=='w';
    for (slot=0;slot<TEXT_SLOTS && streams[slot];++slot) {}
    if (slot==TEXT_SLOTS) { errno=EMFILE; return 0; }
    if (!selected_utf8) {
        if (lab_cms_path(path,id)) { errno=EINVAL; return 0; }
        if (lab_newlib_file_busy(id)) { errno=EBUSY; return 0; }
        if (!writing) {
            rc=lab_cms_state(id,fst);
            if (rc) { errno=rc==28?ENOENT:EIO; return 0; }
            cap=(unsigned)fst[32]<<24|(unsigned)fst[33]<<16|(unsigned)fst[34]<<8|fst[35];
            if (!cap || cap>MAX_LINE) { errno=EOVERFLOW; return 0; }
        }
    }
    Text *s=calloc(1,sizeof *s);
    if (!s) return 0;
    s->writing=writing; s->utf8=selected_utf8;
    if (s->utf8) {
        s->raw=fopen(path,writing?"wb":"rb");
        if (!s->raw) { free(s); return 0; }
    } else {
        memcpy(s->id,id,18);
        s->capacity=cap; s->record=malloc(cap);
        if (!writing) s->decoded=malloc(2*cap+1);
        if (!s->record || (!writing && !s->decoded)) {
            free(s->record); free(s->decoded); free(s); errno=ENOMEM; return 0;
        }
        if (writing) {
            rc=lab_cms_erase(id);
            if (rc && rc!=28) { free(s->record); free(s); errno=EIO; return 0; }
        }
        rc=lab_cms_open_text(&s->file,id,s->record,(int)cap);
        if (rc && !(writing && rc==28)) {
            free(s->record); free(s->decoded); free(s); errno=EIO; return 0;
        }
    }
    FILE *f=funopen(s,writing?0:read_text,writing?write_text:0,0,close_text);
    if (!f) {
        s->failed=ENOMEM; close_text(s); errno=ENOMEM; return 0;
    }
    streams[slot]=s;
    return f;
}
int lab_cms_text_finish(void)
{
    int failed=0;
    /* exit normally closes FILEs first. _exit only delivers data already
       passed to these callbacks, without flushing outer C stream buffers. */
    for (unsigned i=0;i<TEXT_SLOTS;++i)
        if (streams[i] && close_text(streams[i])) failed=-1;
    return failed;
}
int crexx_cms_text_encoding(const char *name)
{
    return lab_cms_text_encoding(name);
}
FILE *crexx_cms_text_open(const char *path,const char *mode)
{
    return lab_cms_text_open(path,mode);
}
