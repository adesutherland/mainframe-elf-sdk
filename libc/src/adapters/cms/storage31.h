#ifndef LAB_CMS_STORAGE31_H
#define LAB_CMS_STORAGE31_H
/* CMS 20 USER subpool, above 16 MiB. NULL/-1 means service failure. */
void *lab_cms_obtain31(unsigned bytes);
int lab_cms_release31(void *address, unsigned bytes);
#endif
