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

static int test_SNM_MSG(size_t           struct_size,
                        const snm_hdr_t *hdr,
                        const void      *msg,
                        size_t           size,
                        enc_func         enc,
                        dec_func         dec)
{
    uint8_t buf[4096];
    snm_hdr_t new_hdr;
    void *new_msg = NULL;
    size_t idx = 0, rem = size;

    if (enc(buf, &idx, hdr, msg))
    {
        traceEvent(TRACE_ERROR, "Error encoding message");
        return -1;
    }

    new_msg = calloc(1, struct_size);

    if (decode_snm_hdr(&new_hdr, buf, &rem, &idx) < 0)
    {
        traceEvent(TRACE_ERROR, "Failed to decode header");
        goto test_SNM_MSG_err;
    }
    if (dec(&new_msg, &new_hdr, buf, &rem, &idx))
    {
        traceEvent(TRACE_ERROR, "Error decoding message");
        goto test_SNM_MSG_err;
    }
    if (memcmp(msg, new_msg, struct_size))
    {
        traceEvent(TRACE_ERROR, "Mismatched messages");
        goto test_SNM_MSG_err;
    }
    return 0;

test_SNM_MSG_err:
    free(new_msg);
    return -1;
}

static void test_REQ_LIST()
{
    snm_hdr_t      hdr;
    n2n_SNM_REQ_t  req;
    size_t         size = 0;

    if (test_SNM_MSG(sizeof(n2n_SNM_REQ_t),
                     &hdr, &req, size,
                     (enc_func) encode_SNM_REQ,
                     (dec_func) decode_SNM_REQ))
    {
        traceEvent(TRACE_ERROR, "Error testing n2n_REQ_LIST_t");
    }
}
static void test_RSP_LIST()
{
    snm_hdr_t      hdr;
    n2n_SNM_RSP_t  rsp;
    size_t         size = 0;

    if (test_SNM_MSG(sizeof(n2n_SNM_RSP_t),
                     &hdr, &rsp, size,
                     (enc_func) encode_SNM_RSP,
                     (dec_func) decode_SNM_RSP))
    {
        traceEvent(TRACE_ERROR, "Error testing n2n_RSP_LIST_t");
    }
}
static void test_ADVERTISE_ME()
{
    snm_hdr_t          hdr;
    n2n_SNM_ADV_ME_t   adv;
    size_t             size = 0;

    if (test_SNM_MSG(sizeof(n2n_SNM_ADV_ME_t),
                     &hdr, &adv, size,
                     (enc_func) encode_ADVERTISE_ME,
                     (dec_func) decode_ADVERTISE_ME))
    {
        traceEvent(TRACE_ERROR, "Error testing n2n_ADVERTISE_ME_t");
    }
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
    void **new_list = NULL;

    if (wr(filename, list))
    {
        traceEvent(TRACE_ERROR, "Error writing list");
        goto out_err;
    }
    if (rd(filename, new_list))
    {
        traceEvent(TRACE_ERROR, "Error reading list");
        goto out_err;
    }
    if (sz(list) != sz(new_list))
    {
        traceEvent(TRACE_ERROR, "Mismatched list sizes");
        goto out_err;
    }

    void *item = list, *new_item = new_list;
    int i = 0;

    while (item && new_item)
    {
        if (memcmp(item + 1, new_item + 1, item_size))
        {
            traceEvent(TRACE_ERROR, "Mismatched item %d", i);
            goto out_err;
        }
        item = (void *) *((int *) item);
        new_item = (void *) *((int *) new_item);
        i++;
    }
    if (cl(new_list))
    {
        traceEvent(TRACE_ERROR, "Error clearing list");
        goto out_err;
    }

    return 0;

out_err:
    unlink(filename);
    return -1;
}

typedef void (*rand_func) (void *item);
typedef void (*add_func) (void **list, void *new);

static void test_list(size_t           item_size,
                      rand_func        rand_item,
                      add_func         add,
                      write_list_func  wr,
                      read_list_func   rd,
                      size_list_func   sz,
                      clear_list_func  cl)
{
    int i = 0, size = random() % 100;
    fprintf(stdout, "Generating %d list items", size);

    void *list = NULL;

    for (i = 0; i < size; i++)
    {
        void *new = calloc(1, item_size);
        rand_item(new);
        add(&list, new);
    }
    if (sz(list) != size)
    {
        traceEvent(TRACE_ERROR, "Mismatched list size");
        return;
    }
    if (test_write_read_list(list, item_size, wr, rd, sz, cl))
    {
        traceEvent(TRACE_ERROR, "Error write/read test");
    }
    if (cl(list))
    {
        traceEvent(TRACE_ERROR, "Error clearing list");
    }
}

static void rand_sn(struct sn_info *si)
{
    si->next = NULL;
    si->sn.family = AF_INET;
    unsigned int ipv4 = random() % (~0);
    memcpy(si->sn.addr.v4, &ipv4, IPV4_SIZE);
    si->sn.port = random() % ((1 << 16) - 1);
}

static void test_sn()
{
    test_list(sizeof(struct sn_info),
              (rand_func) rand_sn,
              (add_func) sn_list_add,
              (write_list_func) write_sn_list_to_file,
              (read_list_func) read_sn_list_from_file,
              (size_list_func) sn_list_size,
              (clear_list_func) clear_sn_list);
}

static void rand_comm(struct comm_info *ci)
{
    ci->next = NULL;
    ci->sn_num = random() * 10;

    int i = 0, name_size = random() % N2N_COMMUNITY_SIZE;
    for (; i < name_size - 1; i++)
    {
        ci->community_name[i] = random() % ('z' - 'a') + 'a';
    }
    ci->community_name[i] = 0;
}

static void test_comm()
{
    test_list(sizeof(struct comm_info),
              (rand_func) rand_comm,
              (add_func) comm_list_add,
              (write_list_func) write_comm_list_to_file,
              (read_list_func) read_comm_list_from_file,
              (size_list_func) comm_list_size,
              (clear_list_func) clear_comm_list);
}

int main()
{
    srandom(time(NULL));
    test_REQ_LIST();
    test_RSP_LIST();
    test_ADVERTISE_ME();
    test_sn();
    test_comm();
    return 0;
}
