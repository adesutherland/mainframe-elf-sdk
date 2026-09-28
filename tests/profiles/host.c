#include <stdio.h>
extern int lab_application(void);
extern volatile unsigned lab_check_count;
int lab_write_ascii_line(const char *s,unsigned n) { return printf("%.*s\n",(int)n,s)<0; }
int main(void) { int rc=lab_application(); printf("matrix reference: %u checks, %d failures\n",lab_check_count,rc); return rc; }
