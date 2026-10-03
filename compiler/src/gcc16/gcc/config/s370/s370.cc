/* Experimental historical S/370 support, split for Mainframe Lab,
   19 September 2026, from the downstream GCC 16.2 S/390 proof.
   The parent S/390 code is Copyright (C) 1999-2026 Free Software
   Foundation, Inc.; contributors include Hartmut Penner, Ulrich Weigand
   and Andreas Krebbel. Original notices remain in the parent files.

   This file is part of GCC and is distributed under the GNU General
   Public License, version 3 or (at your option) any later version,
   WITHOUT ANY WARRANTY. See COPYING3 for the full licence. */

#define IN_TARGET_CODE 1
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "rtl.h"
#include "tm_p.h"
#include "output.h"
#include "diagnostic-core.h"
#include "flags.h"

/* Experimental historical instruction sequences.  Inline literals avoid
   creating new pseudos or constant-pool RTL after register allocation.
   Numeric labels are local to each sequence.  r1 is fixed by the option;
   every accepted image must additionally pass the external opcode audit.  */
const char *
s390_output_s370_ri (const char *opcode, rtx reg, rtx value)
{
  if (REGNO (reg) == 1)
    fatal_error (input_location, "experimental S/370 scratch register conflict");
  rtx args[2] = { reg, value };
  output_asm_insn ("balr\t%%r1,0\n0:\n\tb\t2f-0b(%%r1)\n1:\n\t.short\t%h1\n2:", args);
  char text[80];
  snprintf (text, sizeof text, "%s\t%%0,1b-0b(%%%%r1)", opcode);
  output_asm_insn (text, args);
  return "";
}

const char *
s390_output_s370_base (rtx reg, rtx label)
{
  rtx args[2] = { reg, label };
  output_asm_insn ("balr\t%0,0\n0:\n\tb\t2f-0b(%0)\n\t.balign\t4\n1:\n\t.long\t%l1\n2:\n\tl\t%0,1b-0b(%0)", args);
  return "";
}

const char *
s390_output_s370_jump (rtx label, rtx condition, bool inverted)
{
  rtx args[2] = { label, condition ? GEN_INT (s390_branch_condition_mask (condition)
                                           ^ (inverted ? 0 : 15)) : const0_rtx };
  output_asm_insn ("balr\t%%r1,0\n0:", args);
  if (condition)
    output_asm_insn ("bc\t%c1,3f-0b(%%r1)", args);
  output_asm_insn ("l\t%%r1,1f-0b(%%r1)\n\tbr\t%%r1\n\t.balign\t4\n1:\n\t.long\t%l0\n3:", args);
  return "";
}

/* S/370 has logical word addition/subtraction, but no ALCR/SLBR.  The low
   word sets CC; BALR leaves it unchanged, and BC skips the high-word carry
   adjustment when there is no carry (CC 0/1) or no borrow (CC 2/3).
   Logical operations deliberately avoid fixed-point-overflow exceptions.
   Both operands are register pairs, distinct through the earlyclobber
   constraint.  r1 is fixed and the complete sequence clobbers CC.  */
const char *
s390_output_s370_di_arithmetic (rtx result, rtx operand, bool subtract)
{
  rtx args[2] = { result, operand };
  output_asm_insn (subtract ? "slr\t%N0,%N1" : "alr\t%N0,%N1", args);
  output_asm_insn ("balr\t%%r1,0\n0:", args);
  output_asm_insn (subtract ? "bc\t3,1f-0b(%%r1)"
                            : "bc\t12,1f-0b(%%r1)", args);
  output_asm_insn ("la\t%%r1,1", args);
  output_asm_insn (subtract ? "slr\t%0,%%r1\n1:" : "alr\t%0,%%r1\n1:", args);
  output_asm_insn (subtract ? "slr\t%0,%1" : "alr\t%0,%1", args);
  return "";
}

/* Select before the parent derives architecture defaults.  This names the
   existing lab contract, not a new GCC triplet or a broader S/370 ISA. */
void
s390_s370_select_profile (struct gcc_options *opts, struct gcc_options *set)
{
  if (!opts->x_s370_profile)
    return;
  if (strcmp (opts->x_s370_profile, "vm370-4381-v1")
      && strcmp (opts->x_s370_profile, "vmkernel")
      && strcmp (opts->x_s370_profile, "cms20-esa31-v1"))
    error ("unknown S/370 profile %qs", opts->x_s370_profile);
  /* The CMS 20 profile retains this conservative integer instruction
     selection and the ELF C ABI.  Its separate runtime owns AMODE 31 entry,
     CMSCALL services and 31-bit storage; this does not enable a wider ISA. */
  opts->x_s390_s370 = 1;
  if (!(set->x_target_flags & MASK_64BIT))
    opts->x_target_flags &= ~MASK_64BIT;
  if (!(set->x_target_flags & MASK_ZARCH))
    opts->x_target_flags &= ~MASK_ZARCH;
  if (!(set->x_target_flags & MASK_SOFT_FLOAT))
    opts->x_target_flags |= MASK_SOFT_FLOAT;
  /* The parent common-option table enables MVCLE for -Os.  Size tuning
     must not introduce this later facility.  An explicit request is still
     diagnosed by the profile validation below. */
  if (!(set->x_target_flags & MASK_MVCLE))
    opts->x_target_flags &= ~MASK_MVCLE;
  if (set->x_s390_arch || set->x_s390_tune)
    error ("S/370 profile fixes instruction selection and tuning; "
           "%<-march=%> and %<-mtune=%> are unsupported");
}

/* Called at the original point in the S/390 option override hook. */
void
s390_s370_override_options (struct gcc_options *opts)
{
  /* Fail closed on modes outside the bounded historical proof.  The
     public interface remains the 31-bit ELF ABI inside a 24-bit image.  */
  if (opts->x_s390_s370)
    {
      if (opts->x_s370_profile
          && (opts->x_target_flags & (MASK_OPT_HTM | MASK_OPT_VX
              | MASK_HARD_DFP | MASK_PACKED_STACK | MASK_SMALL_EXEC
              | MASK_MVCLE | MASK_BACKCHAIN | MASK_ZVECTOR)))
        error ("S/370 profile rejects optional facilities and alternate stacks");
      if (TARGET_64BIT_P (opts->x_target_flags)
          || TARGET_ZARCH_P (opts->x_target_flags)
          || !TARGET_SOFT_FLOAT_P (opts->x_target_flags)
          || opts->x_flag_pic || opts->x_flag_pie
          || opts->x_flag_stack_protect || opts->x_flag_split_stack
          || opts->x_flag_stack_clash_protection
          || opts->x_profile_flag
          || opts->x_s390_arch != PROCESSOR_2064_Z900)
        error ("experimental S/370 requires -m31 -mesa -msoft-float, "
               "non-PIC code, no profiling/stack instrumentation, "
               "and the default architecture");
      opts->x_flag_optimize_sibling_calls = 0;
      opts->x_flag_shrink_wrap = 0;
      opts->x_flag_shrink_wrap_separate = 0;
    }
}
