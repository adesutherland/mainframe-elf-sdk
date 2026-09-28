#ifndef LAB_CMS_TEXT_H
#define LAB_CMS_TEXT_H
#include <stdio.h>
/* Plain-C CMS record text adapter. Binary fopen modes bypass this adapter. */
FILE *lab_cms_text_open(const char *path,const char *mode);
int lab_cms_text_encoding(const char *encoding);
/* Existing cREXX hook names remain for source and binary compatibility. */
FILE *crexx_cms_text_open(const char *path,const char *mode);
int crexx_cms_text_encoding(const char *encoding);
int lab_cms_text_busy(const unsigned char id[18]);
int lab_cms_text_finish(void);
#endif
