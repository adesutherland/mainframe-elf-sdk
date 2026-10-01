/* CMS20 terminal qualification: a genuine empty line with one Enter.
 * Run on the leased application console; no files or guest settings change. */
#include <stdio.h>
#include "adapter.h"

int main(void)
{
    unsigned char bytes[130];
    unsigned count = sizeof bytes;
    int result;
    puts("CMS EMPTY INPUT: press Enter once");
    result = lab_cms_input(bytes, &count);
    printf("CMS EMPTY RESULT: RC=%d LENGTH=%u\n", result, count);
    if (result != 0 || count != 0) return 1;
    puts("CMS EMPTY INPUT PASS");
    return 0;
}
