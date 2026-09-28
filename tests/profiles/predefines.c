#ifndef __MAINFRAME_LAB_VM370_4381__
#error Missing named historical profile
#endif
#ifdef __ARCH__
#error Historical profile must not advertise a modern architecture level
#endif
#if defined(__zarch__) || defined(__s390x__) || defined(__VX__) || defined(__HTM__)
#error Modern facility escaped historical profile
#endif
#if __CHAR_BIT__ != 8 || __SIZEOF_SHORT__ != 2 || __SIZEOF_INT__ != 4 || __SIZEOF_LONG__ != 4 || __SIZEOF_POINTER__ != 4 || __SIZEOF_LONG_LONG__ != 8
#error Unexpected historical data model
#endif
#ifndef __CHAR_UNSIGNED__
#error Unexpected plain-char signedness
#endif
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
#error Unexpected byte order
#endif
int historical_predefines(void) { return 370; }
