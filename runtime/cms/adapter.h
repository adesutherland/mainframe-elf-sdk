/* Bounded, single-threaded historical CMS interface for ELF C libraries.
   FSCB/FST layout and service protocol follow CMS-370-GCCLIB cmssys.h and
   cmssys.assemble (Robert O'Hara, Paul Edwards, Dave Wade and contributors;
   public domain), checked against the running guest's CMS macros. */
#ifndef LAB_CMS_ADAPTER_H
#define LAB_CMS_ADAPTER_H
typedef struct {
    char command[8], filename[8], filetype[8], filemode[2];
    short record;
    unsigned char *buffer;
    int capacity;
    char format[2];
    short count;
    int bytes;
} LabCmsFile;
#ifndef LAB_CMS_HOST_TEST
typedef char lab_fscb_size_check[sizeof(LabCmsFile) == 44 ? 1 : -1];
#endif
int lab_cms_svc202(void *, unsigned long result[2]);
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
int lab_cms_svc204(void *, unsigned long result[2]);
int lab_cms_svc204_copy(void *, unsigned long result[2]);
#define lab_cms_service lab_cms_svc204
#else
#define lab_cms_service lab_cms_svc202
#endif
int lab_cms_name(const char *, unsigned char id[18]);
int lab_cms_open(LabCmsFile *, const unsigned char id[18], void *, int);
int lab_cms_open_text(LabCmsFile *, const unsigned char id[18], void *, int);
int lab_cms_read(LabCmsFile *, int *);
int lab_cms_write(LabCmsFile *, int);
int lab_cms_close(LabCmsFile *);
int lab_cms_erase(const unsigned char id[18]);
int lab_cms_state(const unsigned char id[18], unsigned char fst[40]);
int lab_cms_rename(const unsigned char oldid[18], const unsigned char newid[18]);
int lab_cms_line(const char *, unsigned);
int lab_cms_input(unsigned char *, unsigned *);
int lab_ascii_to_ebcdic(unsigned);
int lab_cms_failure(unsigned);
/* Runtime-internal state shared by the CMS console and text-file adapters. */
int lab_cms_text_conversion_enabled(void);
#endif
