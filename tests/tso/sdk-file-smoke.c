/* Selected TSO sequential/PDS services; allocate SDKTXT and SDKPDS first. */
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    char line[32];
    FILE *stream;

    stream = fopen("DD:SDKTXT", "w");
    if (!stream) return 31;
    if (fputs("sdk line\n", stream) < 0 || fclose(stream)) return 32;
    stream = fopen("DD:SDKTXT", "r");
    if (!stream) return 33;
    if (!fgets(line, sizeof line, stream) || strcmp(line, "sdk line\n") ||
        fclose(stream)) return 34;
    puts("SDK SEQUENTIAL PASS");

    stream = fopen("DD:SDKPDS", "r");
    if (!stream) return 41;
    if (fgetc(stream) != '/' || fgetc(stream) != '*' || fclose(stream))
        return 42;
    puts("SDK PDS PASS");

    errno = 0;
    stream = fopen("DD:SDKMISS", "rb");
    if (stream) { fclose(stream); return 51; }
    if (errno != ENOENT) return 52;
    puts("SDK MISSING DD PASS");
    return 42;
}
