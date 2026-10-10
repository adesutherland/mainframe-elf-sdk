/* SPDX-License-Identifier: MIT */
#ifndef MF_COMMAND_INTERNAL_H
#define MF_COMMAND_INTERNAL_H
#include "command.h"
#ifndef MF_COMMAND_HOST_TEST
typedef char mf_command_native_long_check[sizeof(unsigned long) == 4 ? 1 : -1];
typedef char mf_command_native_pointer_check[sizeof(void *) == 4 ? 1 : -1];
#endif
void mf_command_result_init(MfCommandResult *);
int mf_command_name(const char *, unsigned char [8]);
int mf_command_text(const char *, unsigned int, unsigned char *, unsigned int *);
#endif
