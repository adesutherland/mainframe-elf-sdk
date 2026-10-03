/* Native CMS SD/ER/A-type RLD contract; see docs/compiler/SEGMENTED-TEXT.md.
   Keep ordinary one-deck export independent of this explicitly selected route. */
#include <dirent.h>
static void part_path(char *path, size_t cap, const char *directory, unsigned part)
{
    if (snprintf(path,cap,"%s/part%u.text",directory,part)<0) fail("part path formatting failed");
}
static void segments_cleanup(void)
{
    if (!segment_stage_owned) return;
    size_t cap=strlen(segment_stage)+32;
    char *path=malloc(cap);
    if (!path) return;
    for (unsigned p=0;p<MAX_PARTS;++p) { part_path(path,cap,segment_stage,p); unlink(path); }
    snprintf(path,cap,"%s/layout",segment_stage); unlink(path);
    rmdir(segment_stage); free(path);
}
static void segment_emit(FILE *f, int verify)
{
    unsigned char actual[80];
    if (verify) {
        if (fread(actual,1,80,f)!=80 || memcmp(actual,card,80)) fail("package card differs from checked ELF/layout");
    } else if (fwrite(card,1,80,f)!=80) fail("deck write failed");
}
static FILE *part_open(const char *path, int verify)
{
    struct stat st;
    if (verify && (lstat(path,&st) || !S_ISREG(st.st_mode) || st.st_size<0 || st.st_size>80L*MAX_CARDS))
        fail("invalid package member type/size");
    FILE *f=fopen(path,verify?"rb":"wb");
    if (!f) fail("cannot open package member");
    return f;
}
static void segments_run(int check_only, int verify, uint32_t entry)
{
    unsigned parts=(image_size+PART_BYTES-1)/PART_BYTES;
    unsigned counts[MAX_PARTS]={0}, cards[MAX_PARTS]={0};
    char layout[2048];
    int length=snprintf(layout,sizeof layout,"MLAB-SEGMENTS-LAYOUT-1\nIMAGE %u ENTRY %u PARTS %u\n",image_size,entry,parts);
    if (entry>=PART_BYTES) fail("entry must be in the first control section");
    for (unsigned i=0;i<image_size;++i) if (fixes[i]) ++counts[i/PART_BYTES];
    for (unsigned p=0;p<parts;++p) {
        unsigned start=p*PART_BYTES, size=image_size-start;
        if (size>PART_BYTES) size=PART_BYTES;
        cards[p]=2+(size+55)/56+(counts[p]+6)/7;
        if (cards[p]>MAX_CARDS) fail("part exceeds CMS record limit");
        length+=snprintf(layout+length,sizeof layout-(size_t)length,
            "PART %u part%u.text %u %u %u %u\n",p,p,start,size,counts[p],cards[p]);
    }
    if (length<0 || (size_t)length>=sizeof layout) fail("layout exceeds bound");
    if (check_only) { printf("ELF-CMS SEGMENTS CHECK: image=%u entry=%u parts=%u fixups=%u limit=%u\n",image_size,entry,parts,fix_count,MAX_IMAGE); return; }
    const char *directory=output;
    if (!verify) {
        segment_stage=malloc(strlen(output)+32);
        if (!segment_stage) fail("allocation failed");
        sprintf(segment_stage,"%s.work.XXXXXX",output);
        if (!mkdtemp(segment_stage)) fail("cannot create package staging directory");
        segment_stage_owned=1; directory=segment_stage;
    }
    size_t cap=strlen(directory)+32;
    char *path=malloc(cap);
    if (!path) fail("allocation failed");
    for (unsigned p=0;p<parts;++p) {
        unsigned start=p*PART_BYTES, size=image_size-start;
        if (size>PART_BYTES) size=PART_BYTES;
        part_path(path,cap,directory,p);
        FILE *f=part_open(path,verify);
        begin_card((const unsigned char *)"\305\342\304"); /* ESD */
        put16(card+10,p?32:16); put16(card+14,1);
        if (!p) memcpy(card+16,"\305\323\306\327\326\303\100\100",8); /* ELFPOC */
        else { memcpy(card+16,"\305\323\306\360\360\360\360\360",8); card[23]=(unsigned char)(0xf0+p); } /* ELF0000n */
        card[24]=0; put24(card+25,0); put24(card+29,size); /* SD, id 1 */
        if (p) { /* ER id 2 resolves to the first section's absolute base. */
            memcpy(card+32,"\305\323\306\327\326\303\100\100",8);
            card[40]=2; put24(card+41,0); put24(card+45,0);
        }
        segment_emit(f,verify);
        for (unsigned at=0,n;at<size;at+=n) {
            n=size-at; if (n>56) n=56;
            begin_card((const unsigned char *)"\343\347\343");
            put24(card+5,at); put16(card+10,n); put16(card+14,1);
            memcpy(card+16,image+start+at,n); segment_emit(f,verify);
        }
        unsigned used=0;
        for (unsigned at=0;at<size;++at) if (fixes[start+at]) {
            if (!used) begin_card((const unsigned char *)"\331\323\304");
            put16(card+16+used,p?2:1); put16(card+18+used,1);
            card[20+used]=0x0c; put24(card+21+used,at); used+=8;
            if (used==56) { put16(card+10,used); segment_emit(f,verify); used=0; }
        }
        if (used) { put16(card+10,used); segment_emit(f,verify); }
        begin_card((const unsigned char *)"\305\325\304");
        if (!p) { put24(card+5,entry); put16(card+14,1); }
        segment_emit(f,verify);
        if (verify && (fgetc(f)!=EOF || ferror(f))) fail("extra or unreadable package cards");
        if (fclose(f)) fail("package close failed");
    }
    snprintf(path,cap,"%s/layout",directory);
    FILE *f=part_open(path,verify);
    if (verify) {
        char actual[sizeof layout];
        size_t n=fread(actual,1,sizeof actual,f);
        if (n!=(size_t)length || memcmp(actual,layout,n) || ferror(f)) fail("package layout differs");
    } else if (fwrite(layout,1,(size_t)length,f)!=(size_t)length) fail("layout write failed");
    if (fclose(f)) fail("layout close failed");
    if (verify) {
        DIR *d=opendir(directory); struct dirent *e;
        if (!d) fail("cannot inspect package directory");
        unsigned files=0;
        while ((e=readdir(d))) {
            if (!strcmp(e->d_name,".") || !strcmp(e->d_name,"..")) continue;
            if (!strcmp(e->d_name,"package.profile")) {
                struct stat st;
                snprintf(path,cap,"%s/package.profile",directory);
                if (lstat(path,&st) || !S_ISREG(st.st_mode) || st.st_size<0 || st.st_size>2048)
                    fail("invalid profile manifest type/size");
                continue;
            }
            ++files;
        }
        if (closedir(d) || files!=parts+1) fail("unexpected package members");
    } else {
        struct stat st;
        if (!lstat(output,&st) || errno!=ENOENT) fail("output appeared during export");
        if (rename(segment_stage,output)) fail("package directory publication failed");
        segment_stage_owned=0;
    }
    printf("ELF-CMS SEGMENTS %s: image=%u parts=%u fixups=%u\n",verify?"VERIFY":"EXPORT",image_size,parts,fix_count);
    free(path); free(segment_stage); segment_stage=NULL;
}
