/* Only the C memory primitive required by the ABI fixtures. */
typedef __SIZE_TYPE__ size_t;
void *memcpy(void *out,const void *in,size_t n)
{ unsigned char *d=out; const unsigned char *s=in; while(n--) *d++=*s++; return out; }
