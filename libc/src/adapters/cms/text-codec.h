#ifndef LAB_CMS_TEXT_CODEC_H
#define LAB_CMS_TEXT_CODEC_H
typedef struct { unsigned value, minimum, remaining; } LabUtf8;
/* 1: scalar ready; 0: incomplete; -1: malformed. State is per stream. */
int lab_utf8_byte(LabUtf8 *,unsigned byte,unsigned *scalar);
unsigned lab_1047_utf8(unsigned byte,unsigned char out[2]);
int lab_unicode_1047(unsigned scalar);
#endif
