/* Run the original GCC PR69709 body with a recoverable failure recorder.
   Upstream source remains in the pinned, notice-preserving vendor tree.
   The original z10/O3 dg-options are replaced by the historical profile. */
extern void lab_regression_abort(void);
#define __builtin_abort lab_regression_abort
#define main lab_gcc_69709
#include <gcc/testsuite/gcc.target/s390/pr69709.c>
