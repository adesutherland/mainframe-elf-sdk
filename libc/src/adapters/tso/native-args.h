#ifndef LAB_TSO_NATIVE_ARGS_H
#define LAB_TSO_NATIVE_ARGS_H
/* Internal C descriptions. ILP32 matches the native lists directly; LP64
   marshals these descriptions explicitly through libc/src/adapters/tso64/filebridge.c. */
typedef struct {
    const unsigned char *dd;
    int *mode, *recfm, *lrecl, *blksize;
    void **buffer;
    unsigned char *member;
} OpenArgs;
typedef struct { void *handle; unsigned char **record; unsigned *length; } IoArgs;
typedef struct { unsigned ddlen; unsigned char *dd; unsigned dsnlen;
                 unsigned char *dsn; } DynArgs;
#endif
