#ifndef LAB_TSO64_PROFILE_H
#define LAB_TSO64_PROFILE_H
/* The maintained GCC's ordinary z900 LP64 ABI, not its experimental S/370 ABI. */
typedef char lab_tso64_pointer_width[(sizeof(void *)==8)?1:-1];
typedef char lab_tso64_integer_width[(sizeof(int)==4 && sizeof(long)==8)?1:-1];
#endif
