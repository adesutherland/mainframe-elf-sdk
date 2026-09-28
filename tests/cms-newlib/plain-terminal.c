/* One bounded console input line through newlib stdin. */
#include <stdio.h>
#include <string.h>
int main(void)
{
    char line[32];
    puts("CNL INPUT READY");
    fflush(stdout);
    if (!fgets(line, sizeof line, stdin)) {
        puts("CNL INPUT ERROR");
        return 16;
    }
    if (strcmp(line, "HELLO\n")) {
        puts("CNL INPUT MISMATCH");
        return 16;
    }
    puts("CNL INPUT PASS");
    return 44;
}
