/* SPDX-License-Identifier: MIT
   Copyright (c) 2026 Adrian Sutherland
   Selected synchronous CMS31/TSO31 command-environment extension.
   UTF-8 names/text are converted explicitly, independent of file text mode.
   A session is initialized to all zeroes, is not copied, and is closed once.
   Single-threaded use; callbacks/registration are not part of this interface. */
#ifndef MF_COMMAND_H
#define MF_COMMAND_H
#define MF_COMMAND_OK 0
#define MF_COMMAND_INVALID (-1)
#define MF_COMMAND_UNSUPPORTED (-2)
#define MF_COMMAND_NATIVE_ERROR (-3)
#define MF_COMMAND_NO_ENVIRONMENT (-4)
#define MF_COMMAND_NO_MEMORY (-5)
#define MF_COMMAND_MAX_BYTES 32767U
typedef struct {
    unsigned long environment;
    unsigned int active;
} MfCommandSession;
typedef struct {
    int service_rc;
    int command_rc;
    unsigned int command_rc_valid;
    int reason_rc;
    unsigned int abend_code;
    int module_rc;
} MfCommandResult;
/* open borrows the existing native context. It never creates or terminates
   a TSO Rexx processor or registers a CMS environment. */
int mf_command_open(MfCommandSession *, MfCommandResult *);
int mf_command_close(MfCommandSession *);
/* query reports actual availability of ONE name, not an enumerated table.
   Missing is OK with *present=0; service/encoding errors are distinct. */
int mf_command_query(MfCommandSession *, const char *, int *, MfCommandResult *);
/* Output uses the native environment's console/DD. It is not captured here.
   Nonzero command RC is a completed call, not a local adapter error.
   Generic TSO C invokes only TSO via IKJEFTSR; other Rexx environments are
   queryable but return UNSUPPORTED for invocation. No IRXHST fallback. */
int mf_command_execute(MfCommandSession *, const char *, const char *,
                       unsigned int, MfCommandResult *);
/* Optional PDOS vocabulary. Query PDOS first; native CMS/TSO absence is
   UNSUPPORTED. TSO uses the separately named PDOSCMD facility, never an
   alias that changes the meaning of mf_command_execute(...,"TSO",...). */
int mf_pdos_command_execute(MfCommandSession *, const char *, unsigned int,
                            MfCommandResult *);
#endif
