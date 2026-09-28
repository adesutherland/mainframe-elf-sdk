/* Mainframe Lab CMS nucleus interface; C99, no hosted library dependency.
 * Layout authority: maintained DMSFRT MACRO and DMSFRES STAT v1.
 * Integer addresses are checked before FSTRD dereferences them. Host fixtures
 * implement that leaf accessor over a synthetic byte-addressed guest image. */
#ifndef VMCE_CMS_STORAGE_H
#define VMCE_CMS_STORAGE_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint32_t first, count, cached_max;
    uint8_t flags, key, type, unused;
} cms_free_chain;
typedef struct { uint32_t next, size; } cms_free_block;
typedef struct {
    uint32_t count, total, largest, low, high, flags;
} cms_chain_stat;
#define CMS_LAYOUT_ASSERT(n,e) typedef char cms_layout_##n[(e)?1:-1]
CMS_LAYOUT_ASSERT(word,sizeof(uint32_t)==4);
CMS_LAYOUT_ASSERT(chain,sizeof(cms_free_chain)==16);
CMS_LAYOUT_ASSERT(chain_count,offsetof(cms_free_chain,count)==4);
CMS_LAYOUT_ASSERT(chain_max,offsetof(cms_free_chain,cached_max)==8);
CMS_LAYOUT_ASSERT(chain_flags,offsetof(cms_free_chain,flags)==12);
CMS_LAYOUT_ASSERT(chain_key,offsetof(cms_free_chain,key)==13);
CMS_LAYOUT_ASSERT(chain_type,offsetof(cms_free_chain,type)==14);
CMS_LAYOUT_ASSERT(block,sizeof(cms_free_block)==8);
CMS_LAYOUT_ASSERT(block_size,offsetof(cms_free_block,size)==4);
CMS_LAYOUT_ASSERT(stat,sizeof(cms_chain_stat)==24);
CMS_LAYOUT_ASSERT(stat_total,offsetof(cms_chain_stat,total)==4);
CMS_LAYOUT_ASSERT(stat_largest,offsetof(cms_chain_stat,largest)==8);
CMS_LAYOUT_ASSERT(stat_low,offsetof(cms_chain_stat,low)==12);
CMS_LAYOUT_ASSERT(stat_high,offsetof(cms_chain_stat,high)==16);
CMS_LAYOUT_ASSERT(stat_flags,offsetof(cms_chain_stat,flags)==20);
/* Internal C ABI: R2-R6 arguments, R2 result, R15 stack. Native service
 * conventions are implemented by assembler glue, not these declarations. */
uint32_t FSTRD(uint32_t address);
uint32_t FSTCALC(const cms_free_chain *chains, cms_chain_stat *out,
                 uint32_t ceiling);
#endif
