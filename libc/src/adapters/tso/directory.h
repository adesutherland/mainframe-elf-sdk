/* SPDX-License-Identifier: MIT
   Selected PDS directory reader. Names are returned in UTF-8/ASCII,
   independent of mainframe_set_text_conversion(). No path policy is added. */
#ifndef MF_TSO_DIRECTORY_H
#define MF_TSO_DIRECTORY_H
#include <stddef.h>
void *lab_tso_directory_open(const char *);
int lab_tso_directory_next(void *,char *,size_t);
int lab_tso_directory_close(void *);
#endif
