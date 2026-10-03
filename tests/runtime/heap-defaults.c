/* Preprocess the actual syscall translation unit, then verify its policy.
   This checks default/override selection, not a host emulation of the ABI. */
#if LAB_TEST_CMS
#include "../../libc/src/adapters/cms/newlib_syscalls.c"
#define LAB_ACTUAL_HEAP LAB_CMS_HEAP_SIZE
#else
#include "../../libc/src/adapters/tso/io.c"
#define LAB_ACTUAL_HEAP LAB_TSO_HEAP_SIZE
#endif
#if LAB_ACTUAL_HEAP != LAB_EXPECT_HEAP
#error Unexpected application heap default or override
#endif
