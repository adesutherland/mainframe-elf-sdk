/* Minimal host fixture for newlib syscall boundaries, not a target runtime. */
#ifndef SDK_BATCH_HOST_REENT_H
#define SDK_BATCH_HOST_REENT_H
typedef ssize_t _ssize_t;
struct _reent { int unused; };
extern struct _reent batch_reent;
#define _REENT (&batch_reent)
int *__errno(void);
ssize_t _write_r(struct _reent *,int,const void *,size_t);
ssize_t _read_r(struct _reent *,int,void *,size_t);
#endif
