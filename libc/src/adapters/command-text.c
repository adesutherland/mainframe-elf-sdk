/* SPDX-License-Identifier: MIT
   Copyright (c) 2026 Adrian Sutherland */
#include "command-internal.h"
#include "cms/text-codec.h"
#include <string.h>
void mf_command_result_init(MfCommandResult *r)
{
    if (r) {
        r->service_rc = -1; r->command_rc = 0; r->command_rc_valid = 0;
        r->reason_rc = 0; r->abend_code = 0; r->module_rc = 0;
    }
}
int mf_command_name(const char *name, unsigned char out[8])
{
    unsigned int i;
    unsigned int c;
    if (!name || !*name) return MF_COMMAND_INVALID;
    memset(out, 0x40, 8);
    for (i = 0; name[i]; ++i) {
        if (i == 8) return MF_COMMAND_INVALID;
        c = (unsigned char)name[i];
        if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '@' || c == '$' || c == '#')) return MF_COMMAND_INVALID;
        out[i] = (unsigned char)lab_unicode_1047(c);
    }
    return MF_COMMAND_OK;
}
int mf_command_text(const char *text, unsigned int length,
                    unsigned char *out, unsigned int *bytes)
{
    LabUtf8 state;
    unsigned int i, n, scalar;
    int step, native;
    if (!text || !out || !bytes || !length || length > MF_COMMAND_MAX_BYTES)
        return MF_COMMAND_INVALID;
    memset(&state, 0, sizeof(state));
    n = 0;
    for (i = 0; i < length; ++i) {
        step = lab_utf8_byte(&state, (unsigned char)text[i], &scalar);
        if (step < 0) return MF_COMMAND_INVALID;
        if (!step) continue;
        if (!scalar) return MF_COMMAND_INVALID;
        native = lab_unicode_1047(scalar);
        if (native < 0) return MF_COMMAND_INVALID;
        out[n++] = (unsigned char)native;
    }
    if (state.remaining || !n) return MF_COMMAND_INVALID;
    *bytes = n;
    return MF_COMMAND_OK;
}
