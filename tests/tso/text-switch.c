/* Small TSO conversion switch check using an allocated DD:BTXT. */
#include <mainframe_text.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    FILE *f;
    f=fopen("DD:BTXT","w");
    if (!f || fputs("A\n",f)<0 || fclose(f)) return 11;
    f=fopen("DD:BTXT","rb");
    if (!f || fgetc(f)!=0xc1 || fclose(f)) return 12;
    mainframe_set_text_conversion(0);
    f=fopen("DD:BTXT","r");
    if (!f || fgetc(f)!=0xc1 || fgetc(f)!='\n' || fclose(f)) return 13;
    f=fopen("DD:BTXT","w");
    if (!f || fputc(0xc2,f)==EOF || fputc('\n',f)==EOF || fclose(f)) return 14;
    f=fopen("DD:BTXT","rb");
    if (!f || fgetc(f)!=0xc2 || fclose(f)) return 15;
    { const unsigned char native_line[]={0xc3,'\n'};
      if (write(1,native_line,sizeof native_line)!=(ssize_t)sizeof native_line) return 17;
    }
    mainframe_set_text_conversion(7);
    f=fopen("DD:BTXT","r");
    if (!f || fgetc(f)!='B' || fgetc(f)!='\n' || fclose(f)) return 16;
    puts("TSO TEXT SWITCH PASS");
    return 42;
}
