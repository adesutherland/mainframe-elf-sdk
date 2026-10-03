/* Small file-boundary codec; cREXX's language/runtime Unicode rules stay upstream. */
#include "text-codec.h"
#include "text1047.h"
int lab_utf8_byte(LabUtf8 *s,unsigned byte,unsigned *scalar)
{
    if (byte>255) return -1;
    if (!s->remaining) {
        if (byte<0x80) { *scalar=byte; return 1; }
        if (byte>=0xc2 && byte<=0xdf) {
            s->value=byte&31; s->minimum=0x80; s->remaining=1;
        } else if (byte>=0xe0 && byte<=0xef) {
            s->value=byte&15; s->minimum=0x800; s->remaining=2;
        } else if (byte>=0xf0 && byte<=0xf4) {
            s->value=byte&7; s->minimum=0x10000; s->remaining=3;
        } else return -1;
        return 0;
    }
    if ((byte&0xc0)!=0x80) return -1;
    s->value=(s->value<<6)|(byte&63);
    if (--s->remaining) return 0;
    if (s->value<s->minimum || s->value>0x10ffff ||
        (s->value>=0xd800 && s->value<=0xdfff)) return -1;
    *scalar=s->value;
    return 1;
}
unsigned lab_1047_utf8(unsigned byte,unsigned char out[2])
{
    unsigned value=lab_1047_unicode[byte&255];
    if (value<128) { out[0]=(unsigned char)value; return 1; }
    out[0]=(unsigned char)(0xc0|(value>>6));
    out[1]=(unsigned char)(0x80|(value&63));
    return 2;
}
int lab_unicode_1047(unsigned scalar)
{
    if (scalar>255) return -1;
    for (unsigned i=0;i<256;++i) if (lab_1047_unicode[i]==scalar) return (int)i;
    return -1;
}
