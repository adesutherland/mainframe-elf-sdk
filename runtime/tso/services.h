#ifndef LAB_TSO_SERVICES_H
#define LAB_TSO_SERVICES_H
/* Native assembler owns MVS linkage. Each compiled profile has its own
   pointer width, table version and bridge; tables are never interchangeable. */
#ifdef __MAINFRAME_LAB_TSO64__
#define LAB_TSO_SERVICE_VERSION 0x6402U
#else
#define LAB_TSO_SERVICE_VERSION 5U
#endif
typedef unsigned int LabTsoWord;
typedef struct {
    LabTsoWord version;
    int (*putline)(const char *, LabTsoWord);
    void *(*allocate)(LabTsoWord);
    int (*release)(void *, LabTsoWord);
    int (*filecall)(LabTsoWord, const void *);
    void (*finish)(int);
    int (*readline)(unsigned char *,LabTsoWord,LabTsoWord *);
#ifdef __MAINFRAME_LAB_TSO64__
    unsigned char *work;
    LabTsoWord work_size;
#endif
    /* Extension appended after all established fields. Returns the actual
       caller PSW mode (31 or 64) only when problem state is also set. */
    LabTsoWord (*inspect_state)(void);
} LabTsoServices;
extern const LabTsoServices *lab_tso_services;
#ifdef __MAINFRAME_LAB_TSO64__
/* Marshal LP64 arguments and high buffers into the low native work area. */
int lab_tso_filecall(LabTsoWord,const void *);
#else
static inline int lab_tso_filecall(LabTsoWord operation,const void *args)
{ return lab_tso_services->filecall(operation,args); }
#endif
/* filecall: 0 open, 1 read, 2 write, 3 close, 4 allocate DD, 5 free DD.
   0--4 use the pinned MVSSUPA parameter layouts. 5 takes an EBCDIC DD[8]. */
int lab_tso_io_finish(void);
unsigned lab_tso_heap_used(void);
int lab_tso_last_open_status(void);
#endif
