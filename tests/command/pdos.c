/* SPDX-License-Identifier: MIT; optional native PDOS command vocabulary. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"
static int failed;
static void check(const char *name,int ok)
{printf("PDENV CHECK %s %s\n",name,ok?"PASS":"FAIL");if(!ok)++failed;}
static void command(MfCommandSession *s,const char *text,int rc)
{
    MfCommandResult r;int status;
    printf("PDENV OUTPUT BEGIN %s\n",text);fflush(stdout);
    status=mf_pdos_command_execute(s,text,(unsigned int)strlen(text),&r);
    printf("PDENV OUTPUT END status=%d service=%d command=%d valid=%u\n",status,r.service_rc,r.command_rc,r.command_rc_valid);
    check(text,status==0&&r.command_rc_valid&&r.command_rc==rc);
}
int main(int argc,char **argv)
{
    MfCommandSession s;MfCommandResult r;FILE *input;char line[80];
    unsigned char *guard;unsigned int i;int present=0;char path[24];
    memset(&s,0,sizeof s);
#ifdef __MAINFRAME_LAB_CMS20_ESA31__
    if(argc!=4||strlen(argv[1])>8||strlen(argv[2])>8||strlen(argv[3])>2)return 16;
    strcpy(path,argv[1]);strcat(path," ");strcat(path,argv[2]);strcat(path," ");strcat(path,argv[3]);
#else
    if(argc!=2||strlen(argv[1])>=sizeof path)return 16;
    strcpy(path,argv[1]);
#endif
    input=fopen(path,"r");if(!input)return 16;
    if(setvbuf(input,0,_IONBF,0)){fclose(input);return 16;}
    guard=(unsigned char *)malloc(131072U);if(!guard){fclose(input);return 16;}memset(guard,0xa5,131072U);
    check("input-before",fgets(line,sizeof line,input)&&!strcmp(line,"ENV FIRST\n"));
    check("open",mf_command_open(&s,&r)==0);
    if(s.active){
        check("PDOS-present",mf_command_query(&s,"PDOS",&present,&r)==0&&present==1);
        command(&s,"ECHO  mixed Case 'quoted'",0);
        command(&s,"VERSION",0);
        command(&s,"ZZNOEXST",-3);
        command(&s,"ENVCHILD HELLO",7);
        {int status=mf_pdos_command_execute(&s,"ENVFAULT",8U,&r);
         printf("PDENV FAULT status=%d service=%d valid=%u\n",status,r.service_rc,r.command_rc_valid);
#ifdef __MAINFRAME_LAB_CMS20_ESA31__
         check("contained-child-fault",status==0&&r.command_rc_valid&&r.command_rc==-4);
#else
         check("contained-child-fault",status==MF_COMMAND_NATIVE_ERROR&&r.service_rc==12&&!r.command_rc_valid);
#endif
        }
        command(&s,"ECHO caller survives",0);
        check("input-after",fgets(line,sizeof line,input)&&!strcmp(line,"ENV SECOND\n"));
        for(i=0U;i<131072U&&guard[i]==0xa5;++i){}
        check("caller-storage",i==131072U);
        check("close",mf_command_close(&s)==0);
    }
    check("input-close",fclose(input)==0);free(guard);
    printf("PDENV FIXTURE %s failures=%d\n",failed?"FAIL":"PASS",failed);return failed?16:0;
}
