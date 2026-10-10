/* SPDX-License-Identifier: MIT; actual nested application argument/RC control. */
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv)
{
    if(argc!=2||strcmp(argv[1],"HELLO"))return 16;
    puts("PDENV CHILD HELLO RC7");return 7;
}
