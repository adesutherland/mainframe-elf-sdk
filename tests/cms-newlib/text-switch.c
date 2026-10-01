/* Small CMS conversion switch check against one existing CMS text file. */
#include <cms/text.h>
#include <mainframe_text.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    FILE *f;
    if (lab_cms_text_encoding("IBM1047")) return 10;
    f=lab_cms_text_open("CNLSW.TXT","w");
    if (!f || fputs("A\n",f)<0 || fclose(f)) return 11;
    f=fopen("CNLSW.TXT","rb");
    if (!f || fgetc(f)!=0xc1 || fclose(f)) return 12;
    mainframe_set_text_conversion(0);
    f=lab_cms_text_open("CNLSW.TXT","r");
    if (!f || fgetc(f)!=0xc1 || fgetc(f)!='\n' || fclose(f)) return 13;
    f=lab_cms_text_open("CNLSW.TXT","w");
    if (!f || fputc(0xc2,f)==EOF || fputc('\n',f)==EOF || fclose(f)) return 14;
    f=fopen("CNLSW.TXT","rb");
    if (!f || fgetc(f)!=0xc2 || fclose(f)) return 15;
    { const unsigned char native_line[]={0xc3,'\n'};
      if (write(1,native_line,sizeof native_line)!=(ssize_t)sizeof native_line) return 17;
    }
    mainframe_set_text_conversion(7);
    f=lab_cms_text_open("CNLSW.TXT","r");
    if (!f || fgetc(f)!='B' || fgetc(f)!='\n' || fclose(f)) return 16;
    puts("CNL TEXT SWITCH PASS");
    return 42;
}
