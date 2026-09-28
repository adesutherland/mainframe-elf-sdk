/* Independent plain-C CMS runtime consumer. Expected success status is 42.
   The record file is deliberately replaced on each invocation. */
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int failures;
#define CHECK(x) do { if (!(x)) { ++failures; printf("CNL FAIL %d\n", __LINE__); } } while (0)
static void ending(void) { puts("CNL ATEXIT"); }

int main(int argc, char **argv)
{
    unsigned char output[257], input[257];
    char formatted[40];
    struct stat st;
    FILE *f;
    CHECK(argc >= 1 && argv && argv[0] && argv[0][0]);
    if (argc > 1) CHECK(!strcmp(argv[1], "CHECK"));
    CHECK(atexit(ending) == 0);
    CHECK(!strcmp("CMS", "CMS") && strlen("newlib") == 6);
    CHECK(strtol("123x", 0, 10) == 123);
    CHECK(snprintf(formatted, sizeof formatted, "%s:%d", "CMS", 24) == 6);
    CHECK(!strcmp(formatted, "CMS:24"));
    CHECK(sqrt(81.0) == 9.0 && sin(0.0) == 0.0 && cos(0.0) == 1.0);
    {
        unsigned char *p = malloc(1024);
        CHECK(p != 0);
        if (p) {
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
            CHECK((unsigned long)p >= 0x01000000UL &&
                  (unsigned long)p < 0x80000000UL);
#else
            CHECK((unsigned long)p < 0x01000000UL);
#endif
            memset(p, 0xa5, 1024);
            p = realloc(p, 2048);
            CHECK(p != 0 && p[0] == 0xa5 && p[1023] == 0xa5);
            free(p);
        }
        p = calloc(128, 4);
        CHECK(p != 0 && (!p || (p[0] == 0 && p[511] == 0)));
        free(p);
    }
    errno = 0;
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
    CHECK(malloc(32U * 1024U * 1024U) == 0 && errno == ENOMEM);
#else
    CHECK(malloc(512U * 1024U) == 0 && errno == ENOMEM);
#endif
    errno = 0;
    f = fopen("CNLNONE.BIN", "rb");
    CHECK(f == 0 && errno == ENOENT);
    for (unsigned i = 0; i < sizeof output; ++i) output[i] = (unsigned char)i;
    f = fopen("CNLTEST.BIN", "wb");
    CHECK(f != 0);
    if (f) {
        CHECK(fwrite(output, 1, sizeof output, f) == sizeof output);
        CHECK(fclose(f) == 0);
    }
    CHECK(stat("CNLTEST.BIN", &st) == 0 && st.st_size == sizeof output);
    f = fopen("CNLTEST.BIN", "rb");
    CHECK(f != 0);
    if (f) {
        CHECK(fread(input, 1, sizeof input, f) == sizeof input);
        CHECK(!memcmp(input, output, sizeof input));
        CHECK(fgetc(f) == EOF && feof(f));
        errno = 0;
        CHECK(fseek(f, 0, SEEK_SET) != 0 && errno == ESPIPE);
        CHECK(fclose(f) == 0);
    }
    puts("CNL RENAME START"); fflush(stdout);
    CHECK(rename("CNLTEST.BIN", "CNLREN.BIN") == 0);
    puts("CNL RENAME END"); fflush(stdout);
    errno = 0;
    CHECK(stat("CNLTEST.BIN", &st) == -1 && errno == ENOENT);
    puts("CNL REMOVE START"); fflush(stdout);
    CHECK(remove("CNLREN.BIN") == 0);
    puts("CNL REMOVE END"); fflush(stdout);
    errno = 0;
    CHECK(remove("CNLREN.BIN") == -1 && errno == ENOENT);
    errno = 0;
    f = fopen("CNLTEST.BIN", "ab");
    CHECK(f == 0 && errno == ENOSYS);
    if (!failures) puts("CNL CORE PASS");
    else printf("CNL CORE FAILURES %d\n", failures);
    fflush(stdout);
    return failures ? 16 : 42;
}
