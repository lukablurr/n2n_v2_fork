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

int encode_SNM_REQ( uint8_t *base,
                    size_t  *idx,
                    const snm_hdr_t     *hdr,
                    const n2n_SNM_REQ_t *req )
{
    return 0;
}

int decode_SNM_REQ( n2n_SNM_REQ_t    *pkt,
                    const snm_hdr_t  *hdr,
                    const uint8_t    *base,
                    size_t *rem,
                     size_t *idx )
{
    return 0;
}

int encode_SNM_RSP( uint8_t *base,
                    size_t  *idx,
                    const snm_hdr_t     *hdr,
                    const n2n_SNM_RSP_t *rsp )
{
    return 0;
}

int decode_SNM_RSP( n2n_SNM_RSP_t   *pkt,
                    const snm_hdr_t *hdr,
                    const uint8_t   *base,
                    size_t * rem,
                    size_t * idx )
{
    return 0;
}

int encode_ADVERTISE_ME( uint8_t *base,
                         size_t  *idx,
                         const snm_hdr_t        *hdr,
                         const n2n_SNM_ADV_ME_t *adv )
{
    return 0;
}

int decode_ADVERTISE_ME( n2n_SNM_ADV_ME_t  *pkt,
                         const snm_hdr_t   *hdr,
                         const uint8_t     *base,
                         size_t *rem,
                         size_t *idx )
{
    return 0;
}
