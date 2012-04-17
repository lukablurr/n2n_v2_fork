/*
 * sn_multiple_wire.c
 *
 *  Created on: Mar 25, 2012
 *      Author: Costin Lupu
 */

#include "n2n.h"
#include "sn_multiple_wire.h"



static int alloc_array( void **array_ptr, unsigned int num, unsigned int item_size )
{
    if (num > 0)
    {
        *array_ptr = calloc(num, item_size);
        if (!*array_ptr)
        {
            traceEvent(TRACE_ERROR, "not enough memory for allocating array");
            return -1;
        }
    }
    else
    {
        *array_ptr = NULL;
    }

    return 0;
}
static void free_array( void **array_ptr )
{
    if(array_ptr && *array_ptr)
    {
        free(*array_ptr);
        *array_ptr = NULL;
    }
}

int alloc_communities( snm_comm_name_t **comm_ptr, unsigned int comm_num )
{
    return alloc_array((void **) comm_ptr, comm_num, sizeof(snm_comm_name_t));
}
void free_communities( snm_comm_name_t **comm_ptr )
{
    free_array((void **) comm_ptr);
}

int alloc_supernodes( n2n_sock_t **sn_ptr, unsigned int sn_num )
{
    return alloc_array((void **) sn_ptr, sn_num, sizeof(n2n_sock_t));
}
void free_supernodes( n2n_sock_t **sn_ptr )
{
    free_array((void **) sn_ptr);
}

int encode_SNM_comm( uint8_t *base,
                     size_t  *idx,
                     const snm_comm_name_t *comm_name )
{
    int retval = 0;
    retval += encode_uint8(base, idx, comm_name->size);
    retval += encode_buf(base, idx, comm_name->name, comm_name->size);
    return retval;
}

int decode_SNM_comm( snm_comm_name_t *comm_name,
                     const uint8_t *base,
                     size_t *rem,
                     size_t *idx )
{
    int retval = 0;
    memset(comm_name, 0, N2N_COMMUNITY_SIZE);
    retval += decode_uint8(&comm_name->size, base, rem, idx);
    retval += decode_buf(comm_name->name, comm_name->size, base, rem, idx);
    return retval;
}

int encode_SNM_hdr( uint8_t *base,
                    size_t  *idx,
                    const snm_hdr_t *hdr )
{
    int retval = 0;
    retval += encode_uint8(base, idx, hdr->type);
    retval += encode_uint8(base, idx, hdr->flags);
    retval += encode_uint16(base, idx, hdr->seq_num);
    return retval;
}

int decode_SNM_hdr( snm_hdr_t     *hdr,
                    const uint8_t *base,
                    size_t *rem,
                    size_t *idx )
{
    int retval = 0;
    retval += decode_uint8(&hdr->type, base, rem, idx);
    retval += decode_uint8(&hdr->flags, base, rem, idx);
    retval += decode_uint16(&hdr->seq_num, base, rem, idx);
    return retval;
}

int encode_SNM_REQ( uint8_t *base,
                    size_t  *idx,
                    const snm_hdr_t     *hdr,
                    const n2n_SNM_REQ_t *req )
{
    int retval = 0;
    retval += encode_SNM_hdr(base, idx, hdr);
    if (GET_N(hdr->flags))
    {
        retval += encode_uint16(base, idx, req->comm_num);

        int i;
        for (i = 0; i < req->comm_num; i++)
        {
            retval += encode_SNM_comm(base, idx, &req->comm_ptr[i]);
        }
    }
    return retval;
}

int decode_SNM_REQ( n2n_SNM_REQ_t    *pkt,
                    const snm_hdr_t  *hdr,
                    const uint8_t    *base,
                    size_t *rem,
                    size_t *idx )
{
    int retval = 0;

    if (GET_N(hdr->flags))
    {
        retval += decode_uint16(&pkt->comm_num, base, rem, idx);

        if (alloc_communities(&pkt->comm_ptr, pkt->comm_num))
        {
            return -1;
        }

        int i;
        for (i = 0; i < pkt->comm_num; i++)
        {
            retval += decode_SNM_comm(&pkt->comm_ptr[i], base, rem, idx);
        }
    }
    return retval;
}

int encode_SNM_INFO( uint8_t *base,
                     size_t  *idx,
                     const snm_hdr_t      *hdr,
                     const n2n_SNM_INFO_t *info )
{
    int retval = 0;
    retval += encode_SNM_hdr(base, idx, hdr);
    retval += encode_uint16(base, idx, info->sn_num);
    retval += encode_uint16(base, idx, info->comm_num);

    int i;
    for (i = 0; i < info->sn_num; i++)
    {
        retval += encode_sock(base, idx, &info->sn_ptr[i]);
    }

    for(i = 0; i < info->comm_num; i++)
    {
        retval += encode_SNM_comm(base, idx, &info->comm_ptr[i]);
    }
    return retval;
}

int decode_SNM_INFO( n2n_SNM_INFO_t   *pkt,
                     const snm_hdr_t  *hdr,
                     const uint8_t    *base,
                     size_t * rem,
                     size_t * idx )
{
    int retval = 0;
    retval += decode_uint16(&pkt->sn_num, base, rem, idx);
    retval += decode_uint16(&pkt->comm_num, base, rem, idx);

    if (alloc_supernodes(&pkt->sn_ptr, pkt->sn_num))
    {
        return -1;
    }
    if (alloc_communities(&pkt->comm_ptr, pkt->comm_num))
    {
        free_supernodes(&pkt->sn_ptr);
        return -1;
    }

    int i;
    for (i = 0; i < pkt->sn_num; i++)
    {
        retval += decode_sock(&pkt->sn_ptr[i], base, rem, idx);
    }
    for (i = 0; i < pkt->comm_num; i++)
    {
        retval += decode_SNM_comm(&pkt->comm_ptr[i], base, rem, idx);
    }

    return retval;
}
/*

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
*/
void log_SNM_hdr( const snm_hdr_t *hdr )
{
    traceEvent( TRACE_DEBUG, "HEADER type=%d S=%d C=%d N=%d A=%d Seq=%d", hdr->type,
                GET_S(hdr->flags), GET_C(hdr->flags), GET_N(hdr->flags), GET_A(hdr->flags),
                hdr->seq_num );
}
void log_SNM_REQ( const n2n_SNM_REQ_t *req )
{
    int i;
    traceEvent( TRACE_DEBUG, "REQ Communities=%d", req->comm_num );
    for(i = 0; i < req->comm_num; i++)
    {
        traceEvent( TRACE_DEBUG, "\t[%d] len=%d name=%s", i,
                    req->comm_ptr[i].size, req->comm_ptr[i].name );
    }
}
void log_SNM_INFO( const n2n_SNM_INFO_t *info )
{
    int i;
    n2n_sock_str_t sockbuf;;

    traceEvent( TRACE_DEBUG, "INFO Supernodes=%d Communities=%d",
                info->sn_num, info->comm_num );

    for(i = 0; i < info->sn_num; i++)
    {
        traceEvent( TRACE_DEBUG, "\t[S%d] %s", i, sock_to_cstr(sockbuf, &info->sn_ptr[i]) );
    }
    for(i = 0; i < info->comm_num; i++)
    {
        traceEvent( TRACE_DEBUG, "\t[C%d] len=%d name=%s", i, info->comm_ptr[i].size,
                    (info->comm_ptr[i].size > 0) ? (const char *) info->comm_ptr[i].name : "" );
    }
}
