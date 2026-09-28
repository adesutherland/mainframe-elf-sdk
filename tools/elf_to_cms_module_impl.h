/* Fixed-origin VM/370 or experimental relocatable CMS 20 MODULE contract.
 * See docs/compiler/MODULES.md and docs/compiler/MODULE31.md.
 * Project-authored serialization, not copied CMS implementation code.
 * Host transport: BE16 record length, then untouched binary record bytes.
 */
static void module_put32(unsigned char *p, uint32_t v)
{ p[0]=(unsigned char)(v>>24); p[1]=(unsigned char)(v>>16); p[2]=(unsigned char)(v>>8); p[3]=(unsigned char)v; }

static uint32_t module_origin(const char *arg)
{
    size_t n=strlen(arg);
#ifdef RELOCATABLE_MODULE31
    if (!n || n>8 || strspn(arg,"0123456789abcdefABCDEF")!=n)
        fail("origin must be 1..8 hexadecimal digits");
#else
    if (!n || n>6 || strspn(arg,"0123456789abcdefABCDEF")!=n)
        fail("origin must be 1..6 hexadecimal digits");
#endif
    uint32_t origin=(uint32_t)strtoul(arg,NULL,16);
    if (origin<0x20000U || (origin&7)) fail("origin must be >= 020000 and aligned to 8 bytes");
#ifdef RELOCATABLE_MODULE31
    if (origin>=0x80000000U) fail("origin must be a 31-bit address");
#endif
    return origin;
}

static void module_record(FILE *f, const unsigned char *data, uint32_t size, int verify)
{
    unsigned char prefix[2], actual[65535];
    put16(prefix,size);
    if (verify) {
        unsigned char length[2];
        if (fread(length,1,2,f)!=2 || memcmp(length,prefix,2) ||
            fread(actual,1,size,f)!=size || memcmp(actual,data,size))
            fail("MODULE record differs from checked ELF/origin (or is truncated)");
    } else if (fwrite(prefix,1,2,f)!=2 || fwrite(data,1,size,f)!=size)
        fail("MODULE record write failed");
}

static void module_run(int check_only, int verify, uint32_t entry, uint32_t origin)
{
    /* DMSLDR rounds the location counter to a doubleword before GENMOD.
       Static image[] already provides zeroes for these final padding bytes. */
    uint32_t elf_size=image_size;
    image_size=(image_size+7U)&~7U;
    /* Keep even one-past address constants within the selected range. Guest storage and
       CMS free-area capacity remain additional run-time constraints. */
#ifdef RELOCATABLE_MODULE31
    if (image_size>=0x80000000U-origin) fail("origin plus image exceeds 31-bit address range");
#else
    if (image_size>=0x1000000U-origin) fail("origin plus image exceeds 24-bit address range");
#endif
    uint32_t records=1+(image_size+65534U)/65535U;
#ifdef RELOCATABLE_MODULE31
    /* FORMAT31: one control byte plus a four-byte address per fixup.
       Each record ends on an entry boundary. RLDRECS is a halfword. */
    uint32_t rld_records=(fix_count+13106U)/13107U;
    if (!rld_records || rld_records>65535U) fail("invalid MODULE relocation record count");
    records+=1+rld_records; /* one minimal load-map record */
#endif
    printf("ELF-CMS MODULE %s: image=%u padding=%u bss=%u entry=%06X origin=%06X fixups=%u records=%u limit=%u\n",
           check_only?"CHECK":(verify?"VERIFY":"WRITE"),image_size,image_size-elf_size,bss_size,origin+entry,origin,fix_count,records,MAX_IMAGE);
    if (check_only) return;
    for (uint32_t i=0;i<image_size;i++) if (fixes[i]) module_put32(image+i,get32(image+i)+origin);
    unsigned char header[80]={0};
    module_put32(header,origin+entry);   /* STRTADDR */
    module_put32(header+4,origin);       /* FRSTLOC */
    module_put32(header+8,origin+image_size);  /* LASTLOC, exclusive */
    module_put32(header+12,origin+image_size); /* LOCCNT */
    header[43]=0x80; /* NOMAP; TBENT=0, no STRINIT/SYSTEM/DOS/ALL flags. */
#ifdef RELOCATABLE_MODULE31
    /* MODHDCB fields, checked against native CMS 20 GENMOD output.
       These internal formats are version-qualified, not a supported IBM API.
       Keep reserved/session-dependent native header fields zero. */
    header[32]=0x48;                    /* first TEXT address / END entry valid */
    header[34]=0x02;                    /* START linkage: AMODE 31 */
    put16(header+40,3);                 /* CMS START requires >2 map entries */
    header[43]=0x03;                    /* MAP24BYT | POSTXA */
    header[44]=0xa0;                    /* AMODE31 | RMODEANY */
    header[45]=0xd0;                    /* RELODATA | FORMAT31 | CLEANMOD */
    if (rld_records>1) header[45]|=0x20; /* TWORLD: continuation RLD records */
    header[46]=0x80;                    /* INVLD370: requires an ESA guest */
    put16(header+48,rld_records);
#endif
    FILE *f;
    if (verify) f=fopen(output,"rb");
    else {
        temporary=malloc(strlen(output)+32);
        if (!temporary) fail("allocation failed");
        sprintf(temporary,"%s.tmp.XXXXXX",output);
        int fd=mkstemp(temporary);
        temporary_owned=fd>=0;
        f=fd<0?NULL:fdopen(fd,"wb");
    }
    if (!f) fail("cannot open MODULE file");
    module_record(f,header,80,verify);
    for (uint32_t at=0,n;at<image_size;at+=n) {
        n=image_size-at; if (n>65535) n=65535;
        module_record(f,image+at,n,verify);
    }
#ifdef RELOCATABLE_MODULE31
    unsigned char map[72]={0};
    const unsigned char boundary[8]={0xc5,0xd3,0xc6,0xd7,0xd6,0xc3,0x40,0x40};
    memcpy(map,boundary,8);             /* ELFPOC, the single image boundary */
    module_put32(map+12,(origin+entry)|0x80000000U);
    map[16]=0x62;                       /* native resolved 31-bit CSECT entry */
    module_put32(map+20,origin);
    /* Native maps include two nucleus symbols. They are not dependencies of
       this image: use two image-local aliases rather than pinning their
       release-dependent addresses. All three names resolve to the image. */
    memcpy(map+24,map,24);
    memcpy(map+48,map,24);
    const unsigned char start_name[8]={0xc5,0xd3,0xc6,0xe2,0xe3,0xc1,0xd9,0xe3};
    const unsigned char image_name[8]={0xc5,0xd3,0xc6,0xc9,0xd4,0xc1,0xc7,0xc5};
    memcpy(map+24,start_name,8);         /* ELFSTART */
    memcpy(map+48,image_name,8);         /* ELFIMAGE */
    module_record(f,map,sizeof map,verify);
    unsigned char rld[65535];
    uint32_t used=0;
    /* Native GENMOD writes this bounded positive fullword form in descending
       address order. Do not reinterpret signed or external relocations here. */
    for (uint32_t at=image_size;at-->0;) if (fixes[at]) {
        rld[used]=3;                    /* positive four-byte address constant */
        module_put32(rld+used+1,origin+at);
        used+=5;
        if (used==sizeof rld) { module_record(f,rld,used,verify); used=0; }
    }
    if (used) module_record(f,rld,used,verify);
#endif
    if (verify && (fgetc(f)!=EOF || ferror(f))) fail("extra or unreadable MODULE bytes");
    if (fclose(f)) fail("MODULE close failed");
    if (!verify) {
        if (rename(temporary,output)) fail("MODULE rename failed");
        temporary_owned=0;
        free(temporary); temporary=NULL;
    }
}
