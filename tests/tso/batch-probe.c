/* SPDX-License-Identifier: MIT; independent direct MVS batch consumer. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
int main(int argc,char **argv)
{
    char line[256];unsigned char data[256];FILE *f;unsigned i;
    if(argc!=2 || strcmp(argv[1],"BATCH"))return 41;
    if(isatty(0)||isatty(1)||isatty(2))return 42;
    if(!fgets(line,sizeof line,stdin) || strncmp(line,"Input caf\xc3\xa9!",12))return 43;
    for(i=12;line[i] && line[i]!='\n';++i)if(line[i]!=' ')return 44;
    if(line[i]!='\n')return 45;
    if(!fgets(line,sizeof line,stdin))return 46;
    for(i=0;line[i] && line[i]!='\n';++i)if(line[i]!=' ')return 47;
    if(line[i]!='\n' || fgetc(stdin)!=EOF || ferror(stdin))return 48;
    f=fopen("DD:BYTES","rb");if(!f)return 49;
    if(fread(data,1,sizeof data,f)!=sizeof data)return 50;
    for(i=0;i<256;++i)if(data[i]!=(unsigned char)i)return 51;
    if(fgetc(f)!=EOF || ferror(f) || fclose(f))return 52;
    f=fopen("DD:RESULT","wb");if(!f)return 53;
    if(fwrite(data,1,sizeof data,f)!=sizeof data || fclose(f))return 54;
    if(fopen("DD:NOSUCH","rb"))return 55;
    puts("BATCH STDOUT caf\xc3\xa9! [] ^");
    if(fflush(stdout))return 57;
    fputs("BATCH STDERR PASS\n",stderr);
    if(fflush(stderr))return 58;
    puts("BATCH INPUT BINARY MISSING PASS");
    return fflush(stdout)||fflush(stderr)?56:0;
}
