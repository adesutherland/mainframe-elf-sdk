/* CMSSTOR standard SVC interface, checked with tests/cms31/storage.asm.
   Own parameter construction; no generated macro code is included. */
#include "adapter.h"
#include "storage31.h"
extern int lab_cms_storage_call(void *, unsigned, void *, unsigned long[2]);
typedef struct {
    unsigned char command[8], subpool[8];
    unsigned bytes, reserved, address_register;
    unsigned char flags, flags2, subpool_code, reserved2;
} Storage;
typedef char size_check[sizeof(Storage)==32?1:-1];
static void name(unsigned char out[8],const char *s)
{
    for (unsigned i=0;i<8;++i) out[i]=(unsigned char)lab_ascii_to_ebcdic(s[i]);
}
void *lab_cms_obtain31(unsigned bytes)
{
    Storage p={0};
    unsigned long result[2];
    if (!bytes || bytes>0x7ffffff8U || (bytes&7)) return 0;
    name(p.command,"DMSFROSV"); name(p.subpool,"USER    ");
    p.bytes=bytes; p.flags=0x82; p.subpool_code=3;
    if (lab_cms_storage_call(&p,bytes,0,result)) return 0;
    return (void *)result[1];
}
int lab_cms_release31(void *address,unsigned bytes)
{
    Storage p={0};
    unsigned long result[2];
    if (!address || !bytes || bytes>0x7ffffff8U || (bytes&7)) return -1;
    name(p.command,"DMSFRRSV"); name(p.subpool,"        ");
    p.bytes=bytes; p.address_register=8; p.flags=8; p.flags2=2;
    return lab_cms_storage_call(&p,bytes,address,result)?-1:0;
}
