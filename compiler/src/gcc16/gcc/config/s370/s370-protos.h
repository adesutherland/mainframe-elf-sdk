/* Experimental historical S/370 support, split for Mainframe Lab,
   19 September 2026, from the downstream GCC 16.2 S/390 proof.
   The parent S/390 code is Copyright (C) 1999-2026 Free Software
   Foundation, Inc.; contributors include Hartmut Penner, Ulrich Weigand
   and Andreas Krebbel. Original notices remain in the parent files.

   This file is part of GCC and is distributed under the GNU General
   Public License, version 3 or (at your option) any later version,
   WITHOUT ANY WARRANTY. See COPYING3 for the full licence. */

extern const char *s390_output_s370_ri (const char *, rtx, rtx);
extern const char *s390_output_s370_base (rtx, rtx);
extern const char *s390_output_s370_jump (rtx, rtx, bool = false);
extern const char *s390_output_s370_di_arithmetic (rtx, rtx, bool);
extern void s390_s370_override_options (struct gcc_options *);
extern void s390_s370_select_profile (struct gcc_options *, struct gcc_options *);
