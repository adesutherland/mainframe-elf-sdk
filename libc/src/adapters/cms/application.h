/* Bounded CMS command application lifecycle; separate from proof startup. */
#ifndef LAB_CMS_APPLICATION_H
#define LAB_CMS_APPLICATION_H
#define LAB_CMS_MAX_ARGS 32
int lab_cms_arguments(const unsigned char *, char [LAB_CMS_MAX_ARGS][9], char **);
int lab_ebcdic_to_ascii(unsigned);
int lab_cms_path(const char *, unsigned char [18]);
void lab_cms_return(int) __attribute__((noreturn));
void lab_cms_start(const unsigned char *) __attribute__((noreturn));
#endif
