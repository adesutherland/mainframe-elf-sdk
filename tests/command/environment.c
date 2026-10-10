/* SPDX-License-Identifier: MIT
   Copyright (c) 2026 Adrian Sutherland
   Same C application logic for native CMS31 and TSO31. No registration. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"
#ifdef __MAINFRAME_LAB_CMS20_ESA31__
#define CLIENT "CMS31"
#define DEFAULT_ENV "CMS"
#define SUCCESS_COMMAND "QUERY TIME"
#define TEXT_COMMAND "STATE  ENVINPUT DATA A"
#define FAILURE_COMMAND "ZZNOEXST"
static const char *names[] = {"CMS", "COMMAND", "XEDIT", "ZZNOENV"};
#else
#define CLIENT "TSO31"
#define DEFAULT_ENV "TSO"
#define SUCCESS_COMMAND "TIME"
#define TEXT_COMMAND "LISTDS  'LABA01.ENVCMD.INPUT'"
#define FAILURE_COMMAND "LISTDS  'LABA01.ENVCMD.ABSENT'"
static const char *names[] = {"TSO", "MVS", "ISPEXEC", "ISREDIT", "ZZNOENV"};
#endif
static int failed;
static void check(const char *name, int ok)
{
    printf("ENV CHECK %s %s\n", name, ok ? "PASS" : "FAIL");
    if (!ok) ++failed;
}
static int execute(MfCommandSession *s, const char *text, int expect_success)
{
    MfCommandResult r;
    int status;
    printf("ENV OUTPUT BEGIN %s\n", text); fflush(stdout);
    status = mf_command_execute(s, DEFAULT_ENV, text,
                                (unsigned int)strlen(text), &r);
    printf("ENV OUTPUT END status=%d service=%d command=%d valid=%u\n",
           status, r.service_rc, r.command_rc, r.command_rc_valid);
    printf("ENV DETAIL reason=%d abend=%u module=%d\n",
           r.reason_rc, r.abend_code, r.module_rc);
    check(expect_success ? "successful-command" : "failing-command",
          status == 0 && r.command_rc_valid &&
          (expect_success ? r.command_rc == 0 : r.command_rc != 0));
    return status;
}
int main(int argc, char **argv)
{
    MfCommandSession s;
    MfCommandResult r;
    unsigned char *guard;
    FILE *input;
    char line[80];
    char input_name[24];
    const char *input_path;
    unsigned int i;
    int present = -1, status;
    memset(&s, 0, sizeof(s));
    printf("ENV FIXTURE %s ELF/NEWLIB V1\n", CLIENT);
#ifdef __MAINFRAME_LAB_CMS20_ESA31__
    if (argc != 4 || strlen(argv[1]) > 8 || strlen(argv[2]) > 8 || strlen(argv[3]) > 2) {
        puts("usage: ENVFIX filename filetype filemode"); return 16;
    }
    strcpy(input_name,argv[1]); strcat(input_name," ");
    strcat(input_name,argv[2]); strcat(input_name," "); strcat(input_name,argv[3]);
    input_path=input_name;
#else
    (void)input_name;
    if (argc != 2) { puts("usage: ENV31 input-file (two marker records)"); return 16; }
    input_path=argv[1];
#endif
    input = fopen(input_path, "r");
    if (!input) { puts("ENV FAIL input-open"); return 16; }
    if (setvbuf(input, 0, _IONBF, 0) != 0) {
        fclose(input); puts("ENV FAIL input-unbuffered"); return 16;
    }
    check("input-unbuffered", 1);
    guard = (unsigned char *)malloc(131072U);
    if (!guard) { fclose(input); puts("ENV FAIL guard-allocation"); return 16; }
    memset(guard, 0xa5, 131072U);
    check("input-before", fgets(line, sizeof(line), input) &&
          strcmp(line, "ENV FIRST\n") == 0);
    status = mf_command_open(&s, &r);
    printf("ENV OPEN status=%d service=%d\n", status, r.service_rc);
    check("open-existing-context", status == 0);
    if (!status) {
        for (i=0; i<sizeof(names)/sizeof(names[0]); ++i) {
            present=-1;
            status=mf_command_query(&s, names[i], &present, &r);
            printf("ENV QUERY %s status=%d service=%d present=%d\n",
                   names[i], status, r.service_rc, present);
            check(names[i], status == 0 &&
                  (strcmp(names[i], DEFAULT_ENV) ?
                   (strcmp(names[i], "ZZNOENV") ? 1 : present == 0) : present == 1));
        }
        execute(&s, SUCCESS_COMMAND, 1);
        execute(&s, TEXT_COMMAND, 1);
        execute(&s, FAILURE_COMMAND, 0);
        execute(&s, SUCCESS_COMMAND, 1);
        check("input-after", fgets(line, sizeof(line), input) &&
              strcmp(line, "ENV SECOND\n") == 0);
        for (i=0; i<131072U && guard[i] == 0xa5; ++i) { }
        check("caller-storage", i == 131072U);
        check("close-borrowed-context", mf_command_close(&s) == 0);
        present=-1;
        check("closed-query-rejected", mf_command_query(&s, DEFAULT_ENV,
              &present, &r) == MF_COMMAND_INVALID && present == -1);
        check("reopen", mf_command_open(&s, &r) == 0);
        if (s.active) { execute(&s, SUCCESS_COMMAND, 1); mf_command_close(&s); }
    }
    check("input-close", fclose(input) == 0);
    free(guard);
    printf("ENV FIXTURE %s %s failures=%d\n", CLIENT, failed ? "FAIL" : "PASS", failed);
    return failed ? 16 : 0;
}
