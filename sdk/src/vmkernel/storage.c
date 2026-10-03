/* Read-only CMS allocator statistics. No allocator, I/O, hosted runtime,
 * writable statics, recursion or unbounded traversal. The assembler caller
 * owns serialization, input/output ownership and stack/workspace lifetime. */
#include "cms_storage.h"

uint32_t FSTCALC(const cms_free_chain *chains, cms_chain_stat *out,
                 uint32_t ceiling)
{
    unsigned chain;
    if (ceiling < 2048 || ceiling > 0x01000000u) return 12;
    for (chain = 0; chain != 4; ++chain) {
        const cms_free_chain *h = &chains[chain];
        cms_chain_stat *s = &out[chain];
        uint32_t remaining = h->count, ptr = h->first, previous = ceiling;
        uint32_t total = 0, largest = 0, low = 0, high = 0;
        if ((h->flags & 0x40u) || remaining > ceiling / 8u) return 12;
        while (remaining != 0) {
            uint32_t size, end;
            if (ptr < 2048 || (ptr & 0xff000007u) ||
                ptr > ceiling - sizeof(cms_free_block)) return 12;
            size = FSTRD(ptr + offsetof(cms_free_block, size));
            if (!size || (size & 0xff000007u)) return 12;
            /* Subtraction bounds avoid C unsigned wrap and prevent a read
             * of an unchecked next header. Descending chains cannot overlap. */
            if (ptr > previous || size > previous - ptr) return 12;
            end = ptr + size;
            previous = ptr;
            low = ptr;
            if (end > high) high = end;
            total += size; /* disjoint blocks below ceiling bound the sum */
            if (size > largest) largest = size;
            ptr = FSTRD(ptr + offsetof(cms_free_block, next));
            --remaining;
        }
        if (ptr) return 12;
        s->count = h->count;
        s->total = total;
        s->largest = largest;
        s->low = low;
        s->high = high;
        s->flags = h->flags;
    }
    return 0;
}
