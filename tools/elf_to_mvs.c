/* Same bounded ELF validation and relocation writer as the CMS reference,
 * with separately selected MVS AMODE31/RMODEANY object attributes.
 * The initial 2 MiB image/card limits are deliberately retained.
 */
#define MVS_OUTPUT
#include "elf_to_cms.c"
