#ifndef LAB_TSO_SDK_PATH_H
#define LAB_TSO_SDK_PATH_H
#include <stddef.h>
/* The SDK's ordinary C path policy; applications may provide another adapter. */
const char *lab_tso_path(const char *, char *, size_t);
#endif
