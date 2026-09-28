/* Flat CMS directory snapshot subset. This is not a POSIX filesystem. */
#ifndef LAB_CMS_DIRENT_H
#define LAB_CMS_DIRENT_H
struct dirent { char d_name[18]; };
typedef struct LabCmsDir DIR;
DIR *opendir(const char *path);
struct dirent *readdir(DIR *dir);
int closedir(DIR *dir);
#endif
