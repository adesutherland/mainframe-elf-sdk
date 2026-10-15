/* SPDX-License-Identifier: MIT; shared direct native TSO facility client. */
#include <stdio.h>
#include <string.h>
#include "command.h"
int main(int argc,char **argv)
{
    MfCommandResult r;
    if(argc!=2||strcmp(argv[1],"TSO"))return 41;
    if(mf_command_host_execute("TIME",4,&r)||!r.command_rc_valid||r.command_rc)return 42;
    puts("BATCH ADDRESS TSO PASS");return 0;
}
