#include "os.h"

int get_idx_in_table_i(uint64_t vpn, int i) { /* obtain the relevant 9 bits of VPN */
    uint64_t shifted_vpn = vpn;
    if (i == 0) { shifted_vpn = vpn >> 36; }
    if (i == 1) { shifted_vpn = vpn >> 27; }
    if (i == 2) { shifted_vpn = vpn >> 18; }
    if (i == 3) { shifted_vpn = vpn >> 9; }
    if (i == 4) { shifted_vpn = vpn >> 0; }
    return shifted_vpn & 511; /* 511 in decimal = 111,111,111 in binary */
}

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn) {
    if (ppn == NO_MAPPING) { /* need to destroy the VPN mapping */
        if (page_table_query(pt, vpn) == NO_MAPPING) { /* there's no mapping already */
            return;
        }
        /* need to destroy existing mapping */
        uint64_t* level_i_table_ptr = phys_to_virt(pt << 12); /* obtain a pointer to the level_0 table */
        for (int i = 0; i < 5; i++) {
            int idx_in_table = get_idx_in_table_i(vpn, i); /* obtain the relevant 9 bits of VPN */
            if (i == 4) { /* last level (leaf) */
                level_i_table_ptr[idx_in_table]--; /* turn off valid bit of the relevant PTE (destroy mapping) */
                return;
            }
            level_i_table_ptr = phys_to_virt(level_i_table_ptr[idx_in_table] - 1); /* change pointer to next page */
        }
    }
    else { /* need to create a mapping to PPN */
        uint64_t* level_i_table_ptr = phys_to_virt(pt << 12); /* obtain a pointer to the level_0 table */
        for (int i = 0; i < 5; i++) {
            int idx_in_table = get_idx_in_table_i(vpn, i); /* obtain the relevant 9 bits of VPN */
            if (level_i_table_ptr[idx_in_table] % 2 == 0) { /* valid bit of current PTE is 0, need to allocate new frame */
                if (i == 4) { /* last level (leaf) */
                    level_i_table_ptr[idx_in_table] = (ppn << 12) + 1; /* update relevant PTE with the requested mapping and turn on valid bit */
                    return;
                }
                level_i_table_ptr[idx_in_table] = (alloc_page_frame() << 12) + 1; /* update PTE with: alloc + turn on valid bit */
                level_i_table_ptr = phys_to_virt(level_i_table_ptr[idx_in_table] - 1); /* change pointer to the allocated page */
            }
            else { /* valid bit of current PTE is 1 --> continue to the next level */
                if (i == 4) { /* last level (leaf) */
                    level_i_table_ptr[idx_in_table] = (ppn << 12) + 1; /* update relevant PTE with the requested mapping and turn on valid bit */
                    return;
                }
                level_i_table_ptr = phys_to_virt(level_i_table_ptr[idx_in_table] - 1); /* change pointer to next page */
            }
        }
    }
}

uint64_t page_table_query(uint64_t pt, uint64_t vpn) {
    uint64_t* level_i_table_ptr = phys_to_virt(pt << 12); /* obtain a pointer to the level_0 table */
    for (int i = 0; i < 5; i++) {
        int idx_in_table = get_idx_in_table_i(vpn, i); /* obtain the relevant 9 bits of VPN */
        if (level_i_table_ptr[idx_in_table] % 2 == 0) { /* valid bit of current PTE is 0, no mapping for this VPN */
            return NO_MAPPING;
        }
        /* valid bit of current PTE is 1 --> continue to the next level */
        if (i == 4) { /* last level (leaf) */
            return level_i_table_ptr[idx_in_table] >> 12; /* return just the PPN */
        }
        level_i_table_ptr = phys_to_virt(level_i_table_ptr[idx_in_table] - 1); /* change pointer to next page */
    }
    return 0; /* should never get here.. */
}


