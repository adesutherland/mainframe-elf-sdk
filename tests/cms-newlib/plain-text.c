/* Plain-C exercise of the installed CMS text adapter and binary fopen. */
#include <cms/text.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(x) do { if (!(x)) { ++failures; printf("CNL TEXT FAIL %d\n", __LINE__); } } while (0)

static void roundtrip(const char *path, const char *source, const char *expected)
{
    char input[128] = {0};
    FILE *f = lab_cms_text_open(path, "w");
    CHECK(f != 0);
    if (!f) return;
    CHECK(fwrite(source, 1, strlen(source), f) == strlen(source));
    CHECK(fclose(f) == 0);
    f = lab_cms_text_open(path, "r");
    CHECK(f != 0);
    if (!f) return;
    CHECK(fread(input, 1, strlen(expected), f) == strlen(expected));
    CHECK(!memcmp(input, expected, strlen(expected)));
    CHECK(fgetc(f) == EOF && feof(f));
    CHECK(fclose(f) == 0);
}

int main(void)
{
    const char native[] = "Abc #[]{} ! \xc2\xa3\xc3\xa9\n\nlast";
    const char native_expected[] = "Abc #[]{} ! \xc2\xa3\xc3\xa9\n \nlast\n";
    const char utf8[] = "Abc #[]{} ! \xe2\x82\xac\n\nlast";
    FILE *f;
    int first;
    CHECK(lab_cms_text_encoding("IBM1047") == 0);
    roundtrip("CNLTXT.TXT", native, native_expected);
    f = fopen("CNLTXT.TXT", "rb");
    CHECK(f != 0);
    if (f) {
        first = fgetc(f);
        CHECK(first == 0xc1); /* native record, not an ASCII copy */
        CHECK(fclose(f) == 0);
    }
    CHECK(lab_cms_text_encoding("UTF-8") == 0);
    roundtrip("CNLUTF.TXT", utf8, utf8);
    f = fopen("CNLUTF.TXT", "rb");
    CHECK(f != 0);
    if (f) {
        first = fgetc(f);
        CHECK(first == 'A'); /* explicit UTF-8 remains opaque bytes */
        CHECK(fclose(f) == 0);
    }
    errno = 0;
    CHECK(lab_cms_text_encoding("ASCII") == -1 && errno == EINVAL);
    CHECK(lab_cms_text_encoding("IBM1047") == 0);
    f = lab_cms_text_open("CNLERR.TXT", "w");
    CHECK(f != 0);
    if (f) {
        errno = 0;
        CHECK(fwrite("\xe2\x82\xac", 1, 3, f) != 3 || fflush(f) == EOF);
        CHECK(fclose(f) != 0);
    }
    puts(failures ? "CNL TEXT FAIL" : "CNL TEXT PASS");
    return failures ? 16 : 43;
}
