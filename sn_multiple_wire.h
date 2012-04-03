/*
 * sn_multiple_wire.h
 *
 *  Created on: Mar 25, 2012
 *      Author: Costin Lupu
 */

#ifndef SN_MULTIPLE_WIRE_H_
#define SN_MULTIPLE_WIRE_H_

#include <stdint.h>//TODO win
#include "n2n_wire.h"

#define SNM_TYPE_REQ_LIST_MSG    0x01
#define SNM_TYPE_RSP_LIST_MSG    0x02
#define SNM_TYPE_ADV_ME_MSG      0x03

typedef struct snm_hdr
{
    uint8_t    type;
    uint8_t    flags;
    uint16_t   seq_num;
} snm_hdr_t;

#define IS_REQ_LIST(pMsg)        ((pMsg)->hdr.type == SNM_TYPE_REQ_LIST_MSG)
#define IS_RSP_LIST(pMsg)        ((pMsg)->hdr.type == SNM_TYPE_RSP_LIST_MSG)
#define IS_ADV_ME(pMsg)          ((pMsg)->hdr.type == SNM_TYPE_ADV_ME_MSG)

typedef struct snm_comm_name
{
    uint8_t           size;
    uint8_t          *name;
} snm_comm_name_t;

typedef struct n2n_REQ_LIST
{
    /*snm_hdr_t         hdr;*/
    uint16_t          comm_num;
    snm_comm_name_t  *comm_ptr;
} n2n_SNM_REQ_t;

typedef struct n2n_RSP_LIST
{
    /*snm_hdr_t         hdr;*/
    uint16_t          sn_num;
    uint16_t          comm_num;
    n2n_sock_t       *sn_ptr;
    snm_comm_name_t  *comm_ptr;
} n2n_SNM_RSP_t;

typedef struct n2n_ADVERTISE_ME
{
    /*snm_hdr_t         hdr;*/
    n2n_sock_t        sn;
    uint16_t          comm_num;
    snm_comm_name_t  *comm_ptr;
} n2n_SNM_ADV_ME_t;


int encode_snm_hdr( uint8_t *base,
                    size_t  *idx,
                    const snm_hdr_t *hdr );

int decode_snm_hdr( snm_hdr_t     *hdr,
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

int encode_SNM_RSP( uint8_t *base,
                    size_t  *idx,
                    const snm_hdr_t      *hdr,
                    const n2n_SNM_RSP_t  *rsp );

int decode_SNM_RSP( n2n_SNM_RSP_t   *pkt,
                    const snm_hdr_t *hdr,
                    const uint8_t   *base,
                    size_t * rem,
                    size_t * idx );

int encode_ADVERTISE_ME( uint8_t *base,
                         size_t  *idx,
                         const snm_hdr_t        *hdr,
                         const n2n_SNM_ADV_ME_t *adv );

int decode_ADVERTISE_ME( n2n_SNM_ADV_ME_t *pkt,
                         const snm_hdr_t  *hdr,
                         const uint8_t    *base,
                         size_t *rem,
                         size_t *idx );

#endif /* SN_MULTIPLE_WIRE_H_ */
