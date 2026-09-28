/* C99 language and selected library facilities, not a conformance suite. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "adapter.h"
extern int lab_newlib_finish(void);
int lab_check_count;
extern int lab_arithmetic_tests(void);
#define CHECK(x) do { if (!(x)) return lab_cms_failure(__LINE__); } while (0)
static int format_twice(char *a, char *b, size_t n, const char *format, ...)
{
    va_list first, second;
    va_start(first, format);
    va_copy(second, first);
    int x = vsnprintf(a, n, format, first);
    int y = vsnprintf(b, n, format, second);
    va_end(second);
    va_end(first);
    return x == y ? x : -1;
}
int lab_application(void)
{
    CHECK(lab_arithmetic_tests() == 0);
    struct Pair { int a, b; } p = {.b = 17, .a = 25};
    bool valid = p.a + p.b == 42;
    CHECK(valid && sizeof(uint32_t) == 4 && __STDC_VERSION__ >= 199901L);
    char a[96], b[96];
    const char expected[] = "C99 42 -7 65535 23 abc 8 9 10";
    int n = format_twice(a, b, sizeof a, "%s %d %d %u %zu %s %d %d %d",
                         "C99", p.a + p.b, -7, 65535U, (size_t)23, "abc", 8, 9, 10);
    CHECK(n == (int)strlen(expected));
    CHECK(strcmp(a, expected) == 0 && strcmp(b, expected) == 0);
    char small[5] = {1, 1, 1, 1, 1};
    CHECK(snprintf(small, sizeof small, "%s", "abcdef") == 6);
    CHECK(memcmp(small, "abcd\0", 5) == 0);
    CHECK(snprintf(NULL, 0, "%u", 12345U) == 5);

    uint8_t *bytes = calloc(513, 1);
    CHECK(bytes != NULL);
    for (size_t i = 0; i < 513; ++i) CHECK(bytes[i] == 0);
    for (size_t i = 0; i < 513; ++i) bytes[i] = (uint8_t)i;
    uint8_t *grown = realloc(bytes, 777);
    CHECK(grown != NULL);
    bytes = grown;
    for (size_t i = 0; i < 513; ++i) CHECK(bytes[i] == (uint8_t)i);
    for (size_t i = 513; i < 777; ++i) bytes[i] = (uint8_t)i;

    FILE *f = fopen("NLPOC DATA A1", "wb");
    CHECK(f != NULL);
    CHECK(fwrite(bytes, 1, 777, f) == 777);
    CHECK(fclose(f) == 0);
    memset(bytes, 0xa5, 777);
    f = fopen("NLPOC DATA A1", "rb");
    CHECK(f != NULL);
    /* Exercise reads across differently sized CMS record boundaries. */
    CHECK(fread(bytes, 1, 137, f) == 137);
    CHECK(fread(bytes + 137, 1, 640, f) == 640);
    for (size_t i = 0; i < 777; ++i) CHECK(bytes[i] == (uint8_t)i);
    CHECK(fgetc(f) == EOF && feof(f) && !ferror(f));
    CHECK(fseek(f, 0, SEEK_SET) != 0 && errno == ESPIPE);
    clearerr(f);
    CHECK(!ferror(f) && !feof(f));
    CHECK(fclose(f) == 0);
    free(bytes);

    unsigned char missing[18];
    CHECK(lab_cms_name("NLMISS DATA A1", missing) == 0);
    int erase_rc = lab_cms_erase(missing);
    CHECK(erase_rc == 0 || erase_rc == 28);
    errno = 0;
    CHECK(fopen("NLMISS DATA A1", "rb") == NULL && errno == ENOENT);
    CHECK(fopen("NLPOC DATA A1", "ab") == NULL && errno == ENOSYS);
    errno = 0;
    CHECK(write(99, "x", 1) == -1 && errno == EBADF);
    CHECK(malloc(1024UL * 1024UL) == NULL);
    CHECK(printf("NEWLIB C99 POC PASS %d\n", 42) == 23);
    CHECK(fflush(stdout) == 0 && lab_newlib_finish() == 0);
    lab_check_count = 1;
    return 0;
}
