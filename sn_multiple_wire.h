/*
 * sn_multiple_wire.h
 *
 *  Created on: Mar 25, 2012
 *      Author: Costin Lupu
 */

#ifndef SN_MULTIPLE_WIRE_H_
#define SN_MULTIPLE_WIRE_H_

#if defined(WIN32)
#include "win32/n2n_win32.h"

#if defined(__MINGW32__)
#include <stdint.h>
#endif /* #ifdef __MINGW32__ */

#else /* #if defined(WIN32) */
#include <stdint.h>
#endif /* #if defined(WIN32) */

#include "n2n_wire.h"

#define SNM_TYPE_REQ_LIST_MSG             0x01
#define SNM_TYPE_RSP_LIST_MSG             0x02
#define SNM_TYPE_ADV_MSG                  0x03

typedef struct snm_hdr
{
    uint8_t    type;
    uint8_t    flags;
    uint16_t   seq_num;
} snm_hdr_t;

#define IS_REQ_LIST(pMsg)        ((pMsg)->hdr.type == SNM_TYPE_REQ_LIST_MSG)
#define IS_RSP_LIST(pMsg)        ((pMsg)->hdr.type == SNM_TYPE_RSP_LIST_MSG)
#define IS_ADV_ME(pMsg)          ((pMsg)->hdr.type == SNM_TYPE_ADV_MSG)

/*
 * Flags (MSB order):
 *  0 1 2 3 4 5 6 7
 * +-+-+-+-+-+-+-+-+
 * |S|C|N|A| rfu   |
 * +-+-+-+-+-+-+-+-+
 */
#define S_FLAG_POS       7
#define C_FLAG_POS       6
#define N_FLAG_POS       5
#define A_FLAG_POS       4
#define E_FLAG_POS       3

#define __FLAG_MASK(pos)           (1 << (pos))
#define __BIT_FLAG(flags, pos)     (((flags) >> (pos)) & 1)

#define SET_S(flags)      (flags) |= __FLAG_MASK(S_FLAG_POS)
#define SET_C(flags)      (flags) |= __FLAG_MASK(C_FLAG_POS)
#define SET_N(flags)      (flags) |= __FLAG_MASK(N_FLAG_POS)
#define SET_A(flags)      (flags) |= __FLAG_MASK(A_FLAG_POS)
#define SET_E(flags)      (flags) |= __FLAG_MASK(E_FLAG_POS)

#define GET_S(flags)      __BIT_FLAG(flags, S_FLAG_POS)
#define GET_C(flags)      __BIT_FLAG(flags, C_FLAG_POS)
#define GET_N(flags)      __BIT_FLAG(flags, N_FLAG_POS)
#define GET_A(flags)      __BIT_FLAG(flags, A_FLAG_POS)
#define GET_E(flags)      __BIT_FLAG(flags, E_FLAG_POS)

#define CLR_S(flags)      (flags) &= ~__FLAG_MASK(S_FLAG_POS)
#define CLR_C(flags)      (flags) &= ~__FLAG_MASK(C_FLAG_POS)
#define CLR_N(flags)      (flags) &= ~__FLAG_MASK(N_FLAG_POS)
#define CLR_A(flags)      (flags) &= ~__FLAG_MASK(A_FLAG_POS)
#define CLR_E(flags)      (flags) &= ~__FLAG_MASK(E_FLAG_POS)

typedef struct snm_comm_name
{
    uint8_t           size;
    n2n_community_t   name;
} snm_comm_name_t;

typedef struct n2n_SNM_REQ_INFO
{
    uint16_t          comm_num;
    snm_comm_name_t  *comm_ptr;
} n2n_SNM_REQ_t;

typedef struct n2n_SNM_INFO
{
    uint16_t          sn_num;
    uint16_t          comm_num;
    n2n_sock_t       *sn_ptr;
    snm_comm_name_t  *comm_ptr;
} n2n_SNM_INFO_t;

typedef struct n2n_ADVERTISE_ME
{
    n2n_sock_t        sn;
    uint16_t          comm_num;
    snm_comm_name_t  *comm_ptr;
} n2n_SNM_ADV_t;


int alloc_communities( snm_comm_name_t **comm_ptr, unsigned int comm_num );
void free_communities( snm_comm_name_t **comm_ptr );
int alloc_supernodes( n2n_sock_t **sn_ptr, unsigned int sn_num );
void free_supernodes( n2n_sock_t **sn_ptr );

int encode_SNM_comm( uint8_t *base,
                     size_t  *idx,
                     const snm_comm_name_t *comm_name );
int decode_SNM_comm( snm_comm_name_t *comm_name,
                     const uint8_t *base,
                     size_t *rem,
                     size_t *idx );

int encode_SNM_hdr( uint8_t *base,
                    size_t  *idx,
                    const snm_hdr_t *hdr );

int decode_SNM_hdr( snm_hdr_t     *hdr,
                    const uint8_t *base,
                    size_t *rem,
                    size_t *idx );

int encode_SNM_REQ( uint8_t *base,
                    size_t  *idx,
                    const snm_hdr_t      *hdr,
                    const n2n_SNM_REQ_t  *req );

int decode_SNM_REQ( n2n_SNM_REQ_t    *pkt,
                    const snm_hdr_t  *hdr,
                    const uint8_t    *base,
                    size_t *rem,
                    size_t *idx );

int encode_SNM_INFO( uint8_t *base,
                     size_t  *idx,
                     const snm_hdr_t      *hdr,
                     const n2n_SNM_INFO_t *rsp );

int decode_SNM_INFO( n2n_SNM_INFO_t   *pkt,
                     const snm_hdr_t  *hdr,
                     const uint8_t    *base,
                     size_t * rem,
                     size_t * idx );

int encode_SNM_ADV( uint8_t *base,
                    size_t  *idx,
                    const snm_hdr_t       *hdr,
                    const n2n_SNM_ADV_t   *adv );

int decode_SNM_ADV( n2n_SNM_ADV_t    *pkt,
                    const snm_hdr_t  *hdr,
                    const uint8_t    *base,
                    size_t *rem,
                    size_t *idx );


void log_SNM_hdr( const snm_hdr_t *hdr );
void log_SNM_REQ( const n2n_SNM_REQ_t *req );
void log_SNM_INFO( const n2n_SNM_INFO_t *info );
void log_SNM_ADV( const n2n_SNM_ADV_t *adv );


#endif /* SN_MULTIPLE_WIRE_H_ */
