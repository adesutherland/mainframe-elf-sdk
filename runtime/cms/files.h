/* Explicit subset for the historical CMS application runtime. */
#ifndef LAB_CMS_FILES_H
#define LAB_CMS_FILES_H
#include <sys/stat.h>
int lab_cms_fst_stat(const unsigned char fst[40], struct stat *st);
int lab_newlib_file_busy(const unsigned char id[18]);
#endif
