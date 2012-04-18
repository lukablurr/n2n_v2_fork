/*
 * sn_multiple_test.c
 *
 *  Created on: Mar 25, 2012
 *      Author: Costin Lupu
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "n2n.h"
#include "sn_multiple.h"
#include "sn_multiple_wire.h"


typedef int (*enc_func) ( uint8_t         *base,
                          size_t          *idx,
                          const snm_hdr_t *hdr,
                          const void      *msg );

typedef int (*dec_func) ( void            *msg,
                          const snm_hdr_t *hdr,
                          const uint8_t   *base,
                          size_t          *rem,
                          size_t          *idx );

typedef int (*cmp_func) ( const void *a, const void *b );

typedef void (*add_func) (void **list, void *new);
typedef void (*rand_func) (void *item);


/*
 * RANDOM functions
 */
static void rand_sn(struct sn_info *si)
{
    si->next = NULL;
    si->sn.family = AF_INET;
    unsigned int ipv4 = random() % 0xFFFFFFFF;
    memcpy(si->sn.addr.v4, &ipv4, IPV4_SIZE);
    si->sn.port = random() % ((1 << 16) - 1);
}

static void rand_comm(struct comm_info *ci)
{
    ci->next = NULL;
    ci->sn_num = random() * 10;

    int i = 0, name_size = random() % (N2N_COMMUNITY_SIZE - 1) + 1;
    for (; i < name_size; i++)
    {
        ci->community_name[i] = random() % ('z' - 'a') + 'a';
    }
    ci->community_name[i] = 0;
}

static size_t generate_random_list(void          **list,
                                   size_t          item_size,
                                   rand_func       rand_item,
                                   add_func        add)
{
    int i = 0, size = random() % 100;
    traceEvent( TRACE_NORMAL, "Generating %d list items\n", size);

    for (i = 0; i < size; i++)
    {
        void *new = calloc(1, item_size);
        rand_item(new);
        add(list, new);
    }

    return size;
}

/*
 * COMPARE functions
 */
static int cmp_SNM_REQ( const void *a, const void *b )
{
    const n2n_SNM_REQ_t *reqA = (const n2n_SNM_REQ_t *) a;
    const n2n_SNM_REQ_t *reqB = (const n2n_SNM_REQ_t *) b;

    int i, diff = reqA->comm_num - reqB->comm_num;
    if (diff)
    {
        return diff;
    }
    for(i = 0; i < reqA->comm_num; i++)
    {
        diff = memcmp(reqA->comm_ptr[i].name,
                      reqB->comm_ptr[i].name,
                      reqA->comm_ptr[i].size);
        if (diff)
        {
            return diff;
        }
    }

    return 0;
}

static int cmp_SNM_INFO( const void *a, const void *b )
{
    const n2n_SNM_INFO_t *infoA = (const n2n_SNM_INFO_t *) a;
    const n2n_SNM_INFO_t *infoB = (const n2n_SNM_INFO_t *) b;

    int i, diff = infoA->sn_num - infoB->sn_num;
    if (diff)
    {
        return diff;
    }
    diff = infoA->comm_num - infoB->comm_num;
    if (diff)
    {
        return diff;
    }

    for(i = 0; i < infoA->sn_num; i++)
    {
        diff = sock_equal(infoA->sn_ptr + i, infoB->sn_ptr + i);
        if (diff)
        {
            return diff;
        }
    }

    for(i = 0; i < infoA->comm_num; i++)
    {
        diff = memcmp(infoA->comm_ptr[i].name,
                      infoB->comm_ptr[i].name,
                      infoA->comm_ptr[i].size);
        if (diff)
        {
            return diff;
        }
    }

    return 0;
}

static int cmp_SNM_ADV( const void *a, const void *b )
{
    const n2n_SNM_ADV_t *advA = (const n2n_SNM_ADV_t *) a;
    const n2n_SNM_ADV_t *advB = (const n2n_SNM_ADV_t *) b;

    int i, diff = memcmp(&advA->sn, &advB->sn, sizeof(n2n_sock_t));
    if (diff)
    {
        return diff;
    }
    diff = advA->comm_num - advB->comm_num;
    if (diff)
    {
        return diff;
    }

    for(i = 0; i < advA->comm_num; i++)
    {
        diff = memcmp(advA->comm_ptr[i].name,
                      advB->comm_ptr[i].name,
                      advA->comm_ptr[i].size);
        if (diff)
        {
            return diff;
        }
    }

    return 0;
}

/*
 * TESTING functions for MESSAGES
 */
static int test_SNM_MSG(size_t           struct_size,
                        const snm_hdr_t *hdr,
                        const void      *msg,
                        size_t           size,
                        enc_func         enc,
                        dec_func         dec,
                        cmp_func         cmp)
{
    uint8_t buf[1024 * 1024];
    snm_hdr_t new_hdr;
    void *new_msg = NULL;
    size_t idx = 0, rem = size;

    if (enc(buf, &idx, hdr, msg) != idx)
    {
        traceEvent(TRACE_ERROR, "Error encoding message");
        return -1;
    }

    new_msg = calloc(1, struct_size);

    rem = idx;
    idx = 0;
    if (decode_SNM_hdr(&new_hdr, buf, &rem, &idx) < 0)
    {
        traceEvent(TRACE_ERROR, "Failed to decode header");
        goto test_SNM_MSG_err;
    }

    log_SNM_hdr(&new_hdr);

    if (dec(new_msg, &new_hdr, buf, &rem, &idx) < 0)
    {
        traceEvent(TRACE_ERROR, "Error decoding message");
        goto test_SNM_MSG_err;
    }
    if (cmp(msg, new_msg))
    {
        traceEvent(TRACE_ERROR, "Mismatched messages");
        goto test_SNM_MSG_err;
    }

    if (hdr->type == SNM_TYPE_REQ_LIST_MSG)
    {
        log_SNM_REQ(new_msg);
        free_communities(&((n2n_SNM_REQ_t *) new_msg)->comm_ptr);
    }
    else if (hdr->type == SNM_TYPE_RSP_LIST_MSG)
    {
        log_SNM_INFO(new_msg);
        free_supernodes(&((n2n_SNM_INFO_t *) new_msg)->sn_ptr);
        free_communities(&((n2n_SNM_INFO_t *) new_msg)->comm_ptr);
    }
    else if(hdr->type == SNM_TYPE_ADV_MSG)
    {
        log_SNM_ADV(new_msg);
        free_communities(&((n2n_SNM_ADV_t *) new_msg)->comm_ptr);
    }

    free(new_msg);
    return 0;

test_SNM_MSG_err:
    free(new_msg);
    return -1;
}

static void test_REQ_LIST()
{
    snm_hdr_t      hdr = {SNM_TYPE_REQ_LIST_MSG, 0, 3134};
    n2n_SNM_REQ_t  req;
    size_t         size = 0;
    size_t         list_size = 0;
    struct comm_info *communities = NULL;

    traceEvent( TRACE_NORMAL, "---- Testing SNM REQUEST message" );

    SET_N(hdr.flags);

    list_size = generate_random_list((void **) &communities,
                                     sizeof(struct comm_info),
                                     (rand_func) rand_comm,
                                     (add_func) comm_list_add);

    req.comm_num = comm_list_size(communities);
    alloc_communities(&req.comm_ptr, req.comm_num);

    struct comm_info *ci = communities;

    int i;
    for (i = 0; i < req.comm_num; i++)
    {
        req.comm_ptr[i].size = strlen((char *) ci->community_name);
        memcpy(&req.comm_ptr[i].name, ci->community_name, sizeof(n2n_community_t));
        ci = ci->next;
    }

    log_SNM_hdr(&hdr);
    log_SNM_REQ(&req);

    if (test_SNM_MSG(sizeof(n2n_SNM_REQ_t),
                     &hdr, &req, size,
                     (enc_func) encode_SNM_REQ,
                     (dec_func) decode_SNM_REQ,
                     cmp_SNM_REQ))
    {
        traceEvent(TRACE_ERROR, "Error testing n2n_SNM_REQ_t");
    }

    free_communities(&req.comm_ptr);
    clear_comm_list(&communities);

    traceEvent( TRACE_NORMAL, "---- End testing SNM REQUEST message" );
}

static void test_INFO()
{
    snm_hdr_t      hdr = {SNM_TYPE_REQ_LIST_MSG, 0, 3134};
    n2n_SNM_REQ_t  req;
    n2n_SNM_INFO_t rsp;
    size_t         size = 0;

    traceEvent( TRACE_NORMAL, "---- Testing SNM INFO message" );

    SET_S(hdr.flags);
    SET_C(hdr.flags);

    sn_list_t supernodes = { NULL, 0 };
    generate_random_list((void **) &supernodes.list_head, sizeof(struct sn_info),
                         (rand_func) rand_sn, (add_func) sn_list_add);

    comm_list_t communities = { NULL, 0 };
    generate_random_list((void **) &communities.list_head, sizeof(struct comm_info),
                         (rand_func) rand_comm, (add_func) comm_list_add);

    build_snm_info(&supernodes, &communities, &hdr, &req, &rsp);
    clear_sn_list(&supernodes.list_head);
    clear_comm_list(&communities.list_head);
    hdr.type = SNM_TYPE_RSP_LIST_MSG;

    log_SNM_hdr(&hdr);
    log_SNM_INFO(&rsp);

    if (test_SNM_MSG(sizeof(n2n_SNM_INFO_t),
                     &hdr, &rsp, size,
                     (enc_func) encode_SNM_INFO,
                     (dec_func) decode_SNM_INFO,
                     cmp_SNM_INFO))
    {
        traceEvent(TRACE_ERROR, "Error testing n2n_SNM_INFO_t");
    }

    clear_snm_info(&rsp);

    traceEvent( TRACE_NORMAL, "---- End testing SNM INFO message" );
}

static void test_ADV()
{
    snm_hdr_t        hdr = {SNM_TYPE_ADV_MSG, 0, 5463};
    n2n_SNM_ADV_t    adv;
    size_t           size = 0;

    traceEvent( TRACE_NORMAL, "---- Testing SNM ADV message" );

    SET_N(hdr.flags);

    comm_list_t communities = { NULL, 0 };
    generate_random_list((void **) &communities.list_head, sizeof(struct comm_info),
                         (rand_func) rand_comm, (add_func) comm_list_add);

    int sock = open_socket(45555, 1);
    build_snm_adv(sock, &communities, &adv);
    closesocket(sock);

    if (test_SNM_MSG(sizeof(n2n_SNM_ADV_t),
                     &hdr, &adv, size,
                     (enc_func) encode_SNM_ADV,
                     (dec_func) decode_SNM_ADV,
                     cmp_SNM_ADV))
    {
        traceEvent(TRACE_ERROR, "Error testing n2n_SNM_ADV_t");
    }

    traceEvent( TRACE_NORMAL, "---- End testing SNM ADV message" );
}

/*
 * LISTS TESTS
 */
typedef int (*read_list_func) ( const char *filename, void **list );
typedef int (*write_list_func) ( const char *filename, void *list );
typedef size_t (*size_list_func) ( const void *list );
typedef size_t (*clear_list_func) ( void **sn_list );

static int test_write_read_list(void       *list,
                                size_t      item_size,
                                write_list_func  wr,
                                read_list_func   rd,
                                size_list_func   sz,
                                clear_list_func  cl)
{
    const char *filename = "tmp_list";
    void *new_list = NULL;

    if (wr(filename, list))
    {
        traceEvent(TRACE_ERROR, "Error writing list");
        goto out_err;
    }
    if (rd(filename, &new_list))
    {
        traceEvent(TRACE_ERROR, "Error reading list");
        goto out_err;
    }

    size_t size = sz(list);
    size_t new_size = sz(new_list);
    if (size != new_size)
    {
        traceEvent(TRACE_ERROR, "Mismatched list sizes: %d instead of %d", new_size, size);
        goto out_err;
    }

    void *item = list, *new_item = new_list;
    int i = 0;

    while (item && new_item)
    {
#define PTR_SIZE           sizeof(void *)
#define ITEM_DATA(item)    (void *) ((unsigned int) (item) + PTR_SIZE)

        if (memcmp(ITEM_DATA(item), ITEM_DATA(new_item), item_size - PTR_SIZE))
        {
            traceEvent(TRACE_ERROR, "Mismatched item %d", i);
            goto out_err;
        }

#define NEXT_ITEM(item)    ((void *) *((unsigned int *) item))

        item     = NEXT_ITEM(item);
        new_item = NEXT_ITEM(new_item);
        i++;
    }
    if (cl(&new_list) != size)
    {
        traceEvent(TRACE_ERROR, "Error clearing list");
        goto out_err;
    }

    unlink(filename);
    return 0;

out_err:
    unlink(filename);
    return -1;
}

static void test_list(size_t           item_size,
                      rand_func        rand_item,
                      add_func         add,
                      write_list_func  wr,
                      read_list_func   rd,
                      size_list_func   sz,
                      clear_list_func  cl)
{
    void *list = NULL;
    size_t list_size = generate_random_list(&list, item_size, rand_item, add);

    if (sz(list) != list_size)
    {
        traceEvent(TRACE_ERROR, "Mismatched list size");
        return;
    }
    if (test_write_read_list(list, item_size, wr, rd, sz, cl))
    {
        traceEvent(TRACE_ERROR, "Error write/read test");
    }
    if (cl(&list) != list_size)
    {
        traceEvent(TRACE_ERROR, "Error clearing list");
    }
}

static void test_sn()
{
    traceEvent(TRACE_NORMAL, "--- Testing supernodes lists IO");
    test_list(sizeof(struct sn_info),
              (rand_func) rand_sn,
              (add_func) sn_list_add,
              (write_list_func) write_sn_list_to_file,
              (read_list_func) read_sn_list_from_file,
              (size_list_func) sn_list_size,
              (clear_list_func) clear_sn_list);
    traceEvent(TRACE_NORMAL, "--- End testing supernodes lists IO");
}

static void test_comm()
{
    traceEvent(TRACE_NORMAL, "--- Testing community lists IO");
    test_list(sizeof(struct comm_info),
              (rand_func) rand_comm,
              (add_func) comm_list_add,
              (write_list_func) write_comm_list_to_file,
              (read_list_func) read_comm_list_from_file,
              (size_list_func) comm_list_size,
              (clear_list_func) clear_comm_list);
    traceEvent(TRACE_NORMAL, "--- End testing community lists IO");
}

int main()
{
    srandom(time(NULL));
    test_REQ_LIST();
    test_INFO();
    test_ADV();
    test_sn();
    test_comm();
    return 0;
}
