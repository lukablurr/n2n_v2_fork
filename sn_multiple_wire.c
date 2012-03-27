/*
 * sn_multiple_wire.c
 *
 *  Created on: Mar 25, 2012
 *      Author: Costin Lupu
 */

#include "sn_multiple_wire.h"


int encode_snm_hdr( uint8_t *base,
                    size_t  *idx,
                    const snm_hdr_t *hdr )
{
    return 0;
}

int decode_snm_hdr( snm_hdr_t     *hdr,
                    const uint8_t *base,
                    size_t *rem,
                    size_t *idx )
{
    return 0;
}

int encode_REQ_LIST( uint8_t *base,
                     size_t  *idx,
                     const snm_hdr_t      *hdr,
                     const n2n_REQ_LIST_t *req )
{
    return 0;
}

int decode_REQ_LIST( n2n_REQ_LIST_t   *pkt,
                     const snm_hdr_t  *hdr,
                     const uint8_t    *base,
                     size_t *rem,
                     size_t *idx )
{
    return 0;
}

int encode_RSP_LIST( uint8_t *base,
                     size_t  *idx,
                     const snm_hdr_t      *hdr,
                     const n2n_RSP_LIST_t *rsp )
{
    return 0;
}

int decode_RSP_LIST( n2n_RSP_LIST_t  *pkt,
                     const snm_hdr_t *hdr,
                     const uint8_t   *base,
                     size_t * rem,
                     size_t * idx )
{
    return 0;
}

int encode_ADVERTISE_ME( uint8_t *base,
                         size_t  *idx,
                         const snm_hdr_t          *hdr,
                         const n2n_ADVERTISE_ME_t *adv )
{
    return 0;
}

int decode_ADVERTISE_ME( n2n_ADVERTISE_ME_t *pkt,
                         const snm_hdr_t    *hdr,
                         const uint8_t      *base,
                         size_t *rem,
                         size_t *idx )
{
    return 0;
}
