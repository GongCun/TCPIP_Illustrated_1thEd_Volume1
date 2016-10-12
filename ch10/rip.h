#ifndef _RIP_H
#define _RIP_H

struct rip_data {
        uint16_t rip_af;
        uint16_t rip_rt;
        uint32_t rip_addr;
        uint32_t rip_mask;
        uint32_t rip_next_hop;
        uint32_t rip_metric;
};

#endif
