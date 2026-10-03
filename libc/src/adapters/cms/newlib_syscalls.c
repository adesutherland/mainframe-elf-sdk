/* Single-threaded CMS byte-stream syscalls: bounded heap and four file slots.
   The 31-bit profile obtains its heap from CMS; the historical profile uses
   static storage. Binary file records concatenate without conversion. The
   separate text adapter owns IBM1047/UTF-8 records, and console bytes cross
   that encoding boundary explicitly. */
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <reent.h>
#include "adapter.h"
#include "text-codec.h"
#ifdef CREXX_CMS_TEXT_IO
#include "text.h"
#endif
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
#include "storage31.h"
#endif
#ifdef LAB_CMS_APPLICATION
#include "application.h"
#endif
#ifndef LAB_CMS_HEAP_SIZE
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
#define LAB_CMS_HEAP_SIZE (64U*1024U*1024U)
#else
#define LAB_CMS_HEAP_SIZE 65536
#endif
#endif
#if defined(__MAINFRAME_LAB_CMS20_ESA31__) && (LAB_CMS_HEAP_SIZE <= 0 || (LAB_CMS_HEAP_SIZE & 7))
#error CMS 31-bit heap size must be positive and doubleword aligned
#endif
#undef errno
int errno;
typedef struct {
    LabCmsFile cms;
    unsigned char buffer[256];
    unsigned char id[18];
    int used, writing, position, length, eof;
} Slot;
static Slot slots[4];
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
static unsigned char *heap;
#else
static unsigned char heap[LAB_CMS_HEAP_SIZE] __attribute__((aligned(8)));
#endif
static unsigned heap_used;
#ifdef LAB_CMS_APPLICATION
int lab_newlib_file_busy(const unsigned char id[18])
{
    for (unsigned i=0; i<4; ++i)
        /* CMS exact-name lookup ignores the mode digit; preserve that alias. */
        if (slots[i].used && !memcmp(slots[i].id,id,17)) return 1;
#ifdef CREXX_CMS_TEXT_IO
    return lab_cms_text_busy(id);
#else
    return 0;
#endif
}
unsigned lab_newlib_heap_used(void) { return heap_used; }
static unsigned heap_failures, heap_failed_request;
unsigned lab_newlib_heap_failures(void) { return heap_failures; }
unsigned lab_newlib_heap_failed_request(void) { return heap_failed_request; }
#endif
static char line[130];
static unsigned line_used;
static unsigned char input[261];
static unsigned input_used, input_position;
void *_sbrk(ptrdiff_t increment)
{
    if (increment < 0 || (unsigned)increment > LAB_CMS_HEAP_SIZE - heap_used) {
#ifdef LAB_CMS_APPLICATION
        ++heap_failures;
        heap_failed_request = (unsigned)increment;
#endif
        errno = ENOMEM;
        return (void *)-1;
    }
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
    if (!heap) {
        heap=lab_cms_obtain31(LAB_CMS_HEAP_SIZE);
        if (!heap) {
#ifdef LAB_CMS_APPLICATION
            ++heap_failures;
            heap_failed_request=(unsigned)increment;
#endif
            errno=ENOMEM;
            return (void *)-1;
        }
    }
#endif
    void *p = heap + heap_used;
    heap_used += (unsigned)increment;
    return p;
}
static Slot *get(int fd)
{
    if (fd < 3 || fd >= 7 || !slots[fd - 3].used) {
        errno = EBADF;
        return 0;
    }
    return &slots[fd - 3];
}
int _open(const char *path, int flags, ...)
{
    unsigned char id[18];
#ifdef LAB_CMS_APPLICATION
    if (lab_cms_path(path, id)) { errno = EINVAL; return -1; }
#else
    if (lab_cms_name(path, id)) { errno = EINVAL; return -1; }
#endif
    int mode = flags & O_ACCMODE;
#ifdef CREXX_CMS_TEXT_IO
    if (lab_cms_text_busy(id)) { errno=EBUSY; return -1; }
#endif
    if ((flags & ~(O_ACCMODE | O_CREAT | O_TRUNC)) ||
        (mode == O_RDONLY && flags != O_RDONLY) ||
        (mode != O_RDONLY && mode != O_WRONLY) ||
        (mode == O_WRONLY && (flags & (O_CREAT | O_TRUNC)) != (O_CREAT | O_TRUNC))) {
        errno = ENOSYS;
        return -1;
    }
    int free_slot = -1;
    for (unsigned i = 0; i < 4; ++i) {
        if (slots[i].used && memcmp(id, slots[i].id, 18) == 0) {
            errno = EBUSY; return -1;
        }
        if (!slots[i].used && free_slot < 0) free_slot = (int)i;
    }
    if (free_slot < 0) { errno = EMFILE; return -1; }
    Slot *s = &slots[free_slot];
    memset(s, 0, sizeof *s);
    memcpy(s->id, id, 18);
    if (mode == O_WRONLY) {
        int rc = lab_cms_erase(id);
        if (rc != 0 && rc != 28) { errno = EIO; return -1; }
    }
    int rc = lab_cms_open(&s->cms, id, s->buffer, sizeof s->buffer);
    if (rc && (rc != 28 || mode == O_RDONLY)) {
        errno = rc == 28 ? ENOENT : EIO;
        return -1;
    }
    s->writing = mode == O_WRONLY;
    s->used = 1;
    return free_slot + 3;
}
int _close(int fd)
{
    Slot *s = get(fd);
    if (!s) return -1;
    /* STATE only describes an input file. CMS opens it on the first RDBUF;
       FINIS before that reports "not open". record remains 1 until RDBUF. */
    int rc = !s->writing && s->cms.record == 1 ? 0 : lab_cms_close(&s->cms);
    s->used = 0;
    if (rc) { errno = EIO; return -1; }
    return 0;
}
_ssize_t _write(int fd, const void *buffer, size_t length)
{
    const unsigned char *bytes = buffer;
    size_t done = 0;
    if (fd == 1 || fd == 2) {
        /* Validate before emitting to avoid silent partial conversion. */
        for (size_t i = 0; i < length; ++i)
            if (lab_cms_text_conversion_enabled() && bytes[i] != '\n' &&
                lab_ascii_to_ebcdic(bytes[i]) < 0) {
                errno = EILSEQ; return -1;
            }
        for (; done < length; ++done) {
            if (bytes[done] == '\n' || line_used == sizeof line) {
                if (lab_cms_line(line, line_used)) { errno = EIO; return -1; }
                line_used = 0;
            }
            if (bytes[done] != '\n') line[line_used++] = (char)bytes[done];
        }
        return (_ssize_t)done;
    }
    Slot *s = get(fd);
    if (!s) return -1;
    if (!s->writing) { errno = EBADF; return -1; }
    while (done < length) {
        size_t n = length - done;
        if (n > sizeof s->buffer) n = sizeof s->buffer;
        memcpy(s->buffer, bytes + done, n);
        if (lab_cms_write(&s->cms, (int)n)) {
            errno = EIO; return done ? (_ssize_t)done : -1;
        }
        done += n;
    }
    return (_ssize_t)done;
}
_ssize_t _read(int fd, void *buffer, size_t length)
{
    unsigned char *bytes = buffer;
    if (fd == 0) {
        size_t done=0;
        if (!length) return 0;
        while (done<length) {
            if (input_position==input_used) {
                unsigned char record[130];
                unsigned count=sizeof record;
                if (lab_cms_input(record,&count) || count>sizeof record) {
                    errno=EIO; return done ? (_ssize_t)done : -1;
                }
                input_used=input_position=0;
                for (unsigned i=0;i<count;++i) {
                    unsigned char encoded[2];
                    if (lab_cms_text_conversion_enabled()) {
                        unsigned n=lab_1047_utf8(record[i],encoded);
                        for (unsigned j=0;j<n;++j) input[input_used++]=encoded[j];
                    } else input[input_used++]=record[i];
                }
                input[input_used++]='\n';
            }
            size_t n=input_used-input_position;
            if (n>length-done) n=length-done;
            memcpy(bytes+done,input+input_position,n);
            input_position+=(unsigned)n;
            done+=n;
            if (input_position==input_used) break;
        }
        return (_ssize_t)done;
    }
    Slot *s = get(fd);
    if (!s) return -1;
    if (s->writing) { errno = EBADF; return -1; }
    size_t done = 0;
    while (done < length) {
        if (s->position == s->length) {
            if (s->eof) break;
            int rc = lab_cms_read(&s->cms, &s->length);
            s->position = 0;
            if (rc == 12) { s->eof = 1; break; }
            if (rc) { errno = EIO; return done ? (_ssize_t)done : -1; }
        }
        size_t n = (size_t)(s->length - s->position);
        if (n > length - done) n = length - done;
        memcpy(bytes + done, s->buffer + s->position, n);
        s->position += (int)n;
        done += n;
    }
    return (_ssize_t)done;
}
int _fstat(int fd, struct stat *st)
{
    if (fd < 0 || (fd > 2 && !get(fd))) { errno = EBADF; return -1; }
    memset(st, 0, sizeof *st);
    st->st_mode = fd <= 2 ? S_IFCHR : S_IFREG;
    st->st_blksize = 256;
    return 0;
}
int _isatty(int fd)
{
    if (fd >= 0 && fd <= 2) return 1;
    if (get(fd)) errno = ENOTTY;
    return 0;
}
off_t _lseek(int fd, off_t offset, int whence)
{
    (void)offset; (void)whence;
    if (fd >= 0 && fd <= 2) errno = ESPIPE;
    else if (get(fd)) errno = ESPIPE;
    return (off_t)-1;
}
int lab_newlib_finish(void)
{
    int rc = line_used ? lab_cms_line(line, line_used) : 0;
    line_used = 0;
#ifdef CREXX_CMS_TEXT_IO
    if (lab_cms_text_finish()) rc=-1;
#endif
    for (unsigned i = 0; i < 4; ++i)
        if (slots[i].used && _close((int)i + 3)) rc = -1;
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
    if (heap && lab_cms_release31(heap,LAB_CMS_HEAP_SIZE)) rc=-1;
    heap=0;
#endif
    return rc;
}
ssize_t write(int fd, const void *buffer, size_t length)
{
    return _write_r(_REENT, fd, buffer, length);
}
