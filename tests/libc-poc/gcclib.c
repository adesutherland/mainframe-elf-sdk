/* Stage one: actual GCCLIB algorithms plus its CMS file-service protocol
   rebuilt for the modern ELF ABI. Full GCCLIB stdio/startup is not ported. */
#include <string.h>
#include <stdlib.h>
#include <gcccrab.h>
#include "adapter.h"
GCCCRAB lab_gcclib_crab;
int lab_check_count;
extern int lab_arithmetic_tests(void);
static int compare(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}
#define CHECK(x) do { if (!(x)) return lab_cms_failure(__LINE__); } while (0)
int lab_application(void)
{
    CHECK(lab_arithmetic_tests() == 0);
    char *end;
    int numbers[] = {99, -42, 7, 0, 17};
    const int expected[] = {-42, 0, 7, 17, 99};
    unsigned char id[18], buf[256];
    LabCmsFile f;
    CHECK(strtol("-12345x", &end, 10) == -12345 && *end == 'x');
    qsort(numbers, 5, sizeof numbers[0], compare);
    CHECK(memcmp(numbers, expected, sizeof expected) == 0);
    CHECK(lab_cms_name("GCLPOC DATA A1", id) == 0);
    int rc = lab_cms_erase(id);
    CHECK(rc == 0 || rc == 28);
    CHECK(lab_cms_open(&f, id, buf, sizeof buf) == 28);
    for (unsigned i = 0; i < sizeof buf; ++i) buf[i] = (unsigned char)i;
    CHECK(lab_cms_write(&f, sizeof buf) == 0);
    CHECK(lab_cms_close(&f) == 0);
    memset(buf, 0xa5, sizeof buf);
    CHECK(lab_cms_open(&f, id, buf, sizeof buf) == 0);
    int length = -1;
    CHECK(lab_cms_read(&f, &length) == 0 && length == sizeof buf);
    for (unsigned i = 0; i < sizeof buf; ++i) CHECK(buf[i] == i);
    CHECK(lab_cms_read(&f, &length) == 12 && length == 0);
    CHECK(lab_cms_close(&f) == 0);
    CHECK(lab_cms_line("GCCLIB ADAPTER PASS", 19) == 0);
    lab_check_count = 1;
    return 0;
}
