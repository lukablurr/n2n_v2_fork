/*
 * sn_multiple.c
 *
 *  Created on: Mar 25, 2012
 *      Author: Costin Lupu
 */

#include "n2n.h"
#include "sn_multiple.h"

/*******************************************************************
 *                Operations on sn_info lists.                     *
 *******************************************************************/

static FILE *open_list_file_for_read( const char *filename )
{
    int fd = open(filename, O_CREAT | O_RDONLY, 0666);
    if (fd < 0)
        return NULL;

    return fdopen(fd, "r");
}

int read_sn_list_from_file( const char *filename, struct sn_info **list )
{
    n2n_sock_t      sn;
    n2n_sock_str_t  sock_str;

    traceEvent(TRACE_INFO, "opening supernodes file %s", filename);

    FILE *fd = open_list_file_for_read(filename);
    if (!fd)
    {
        traceEvent(TRACE_ERROR, "couldn't open supernodes file");
        return -1;
    }

    while (fscanf(fd, "%s\n", sock_str) > 0)
    {
        sock_from_cstr(&sn, sock_str);

        if (sn_list_add_new(list, &sn, NULL) < 0)
        {
            traceEvent(TRACE_ERROR, "couldn't add read supernode");
            goto out_err;
        }

        traceEvent(TRACE_DEBUG, "Added supernode %s", sock_str);
    }

    sn_reverse_list(list);

    fclose(fd);
    return 0;

out_err:
    clear_sn_list(list);
    fclose(fd);
    return -1;
}

int write_sn_list_to_file( const char *filename, struct sn_info *list )
{
    n2n_sock_str_t sock_str;

    FILE *fd = fopen(filename, "w");
    if (!fd)
    {
        traceEvent(TRACE_ERROR, "couldn't open supernodes file");
        return -1;
    }

    while (list)
    {
        if (fprintf(fd, "%s\n", sock_to_cstr(sock_str, &list->sn)) < 0)
        {
            traceEvent(TRACE_ERROR, "couldn't write supernode entry to file");
            goto out_err;
        }

        list = list->next;
    }

    fclose(fd);
    return 0;

out_err:
    fclose(fd);
    return -1;
}

void sn_list_add( struct sn_info **list, struct sn_info *new )
{
    new->next = *list;
    *list = new;
}

size_t clear_sn_list( struct sn_info **sn_list )
{
    size_t retval = 0;
    struct sn_info *scan = *sn_list;

    while (scan)
    {
        struct sn_info *crt = scan;
        scan = scan->next;
        free(crt);
        retval++;
    }
    *sn_list = NULL;

    return retval;
}

size_t sn_list_size( const struct sn_info *list )
{
    size_t retval = 0;
    while (list)
    {
        ++retval;
        list = list->next;
    }
    return retval;
}

void sn_reverse_list( struct sn_info **list )
{
    struct sn_info *aux = *list, *next = NULL;
    *list = NULL;

    while (aux)
    {
        next = aux->next;
        sn_list_add(list, aux);
        aux = next;
    }
}

struct sn_info *sn_find( struct sn_info *list, const n2n_sock_t *sn )
{
    while (list)
    {
        if (sn_cmp(&list->sn, sn) == 0)
        {
            return list;
        }
        list = list->next;
    }
    return NULL;
}

struct sn_info *sn_list_add_create(struct sn_info **list, n2n_sock_t *sn)
{
    struct sn_info *new = calloc(1, sizeof(struct sn_info));
    if (!new)
    {
        traceEvent(TRACE_ERROR, "not enough memory for new SN info");
        return NULL;
    }

    sn_cpy(&new->sn, sn);
    sn_list_add(list, new);

    return new;
}

int sn_list_add_new(struct sn_info **list, n2n_sock_t *sn_sock, struct sn_info **out_item)
{
    int retval = 0;

    struct sn_info *sni = sn_find(*list, sn_sock);
    if (!sni)
    {
        if ((sni = sn_list_add_create(list, sn_sock)) == NULL)
            return -1;

        retval = 1;
    }

    if (out_item)
        *out_item = sni;

    return retval;
}

void sn_list_sort(struct sn_info **list, sn_cmp_func cmp_func)
{
    size_t list_size = sn_list_size(*list);
    *list = merge_sort(list, list_size, cmp_func);
}

int update_supernodes(sn_list_t *supernodes, n2n_sock_t *sn)
{
    struct sn_info *si = NULL;
    n2n_sock_str_t sock_str;

    int new_one = sn_list_add_new(&supernodes->list_head, sn, &si);

    if (!new_one)
    {
        /* existing supernode */
        si->timestamp = time(NULL);
    }
    else
        traceEvent(TRACE_DEBUG, "Added supernode %s", sock_to_cstr(sock_str, sn));

    return new_one;
}

int update_and_save_supernodes(sn_list_t *supernodes, n2n_sock_t *sn_array, int sn_num)
{
    int need_write = 0, i = 0;

    for (; i < sn_num; i++)
    {
        need_write += update_supernodes(supernodes, &sn_array[i]);
    }

    if (need_write)
    {
        /* elements added */
        write_sn_list_to_file(supernodes->filename, supernodes->list_head);
    }

    return need_write;
}

struct sn_info *merge(struct sn_info **left, size_t left_size, struct sn_info **right, size_t right_size, sn_cmp_func func);

struct sn_info *merge_sort(struct sn_info **list, size_t size, sn_cmp_func func)
{
    if (size == 1)
    {
        return *list;
    }

    // else list size is > 1, so split the list into two sublists
    struct sn_info *left = NULL, *right = NULL;
    int i, middle = size / 2;

    struct sn_info *x = *list, *next = NULL;
    for(i = 0; i < middle; i++)
    {
        next = x->next;
        sn_list_add(&left, x);
        x = next;
    }
    sn_reverse_list(&left);

    for(; i < size; i++)
    {
        next = x->next;
        sn_list_add(&right, x);
        x = next;
    }
    sn_reverse_list(&right);

    left  = merge_sort(&left,  middle, func);
    right = merge_sort(&right, size - middle, func);

    return merge(&left, middle, &right, size - middle, func);
}

struct sn_info *merge(struct sn_info **left, size_t left_size, struct sn_info **right, size_t right_size, sn_cmp_func func)
{
    struct sn_info *result = NULL, *next = NULL;

#define first(sni)   *sni

    while( left_size > 0 || right_size > 0)
    {
        if (left_size > 0 && right_size > 0)
        {
            if (func(first(left), first(right)) <= 0)
            {
                next = (*left)->next;
                sn_list_add(&result, first(left));
                *left = next;
                left_size--;
            }
            else
            {
                next = (*right)->next;
                sn_list_add(&result, first(right));
                *right = next;
                right_size--;
            }
        }
        else if (left_size > 0)
        {
            next = (*left)->next;
            sn_list_add(&result, first(left));
            *left = next;
            left_size--;
        }
        else if (right_size > 0)
        {
            next = (*right)->next;
            sn_list_add(&result, first(right));
            *right = next;
            right_size--;
        }
    }

    sn_reverse_list(&result);
    return result;
}

/*******************************************************************
 *                 Operations on comm_info lists.                  *
 *******************************************************************/
int read_comm_list_from_file( const char *filename, struct comm_info **list )
{
    n2n_sock_str_t sock_str;
    struct comm_info *ci = NULL;

    traceEvent(TRACE_INFO, "opening communities file %s", filename);

    FILE *fd = open_list_file_for_read(filename);
    if (!fd)
    {
        traceEvent(TRACE_ERROR, "couldn't open community file. %s", strerror(errno));
        return -1;
    }

    unsigned int comm_num = 0;
    if (fscanf(fd, "comm_num=%d\n", &comm_num) < 0)
    {
        //traceEvent(TRACE_ERROR, "couldn't read communities number from file");
        goto out_empty;
    }

    int i, j;
    for (i = 0; i < comm_num; i++)
    {
        ci = calloc(1, sizeof(struct comm_info));
        if (!ci)
        {
            traceEvent(TRACE_ERROR, "couldn't allocate a new community entry");
            goto out_err;
        }

        fscanf(fd, "sn_num=%d name=%s\n", &ci->sn_num, ci->community_name);

        for (j = 0; j < ci->sn_num; j++)
        {
            fscanf(fd, "\t%s\n", sock_str);

            sock_from_cstr(&ci->sn_sock[j], sock_str);
        }

        comm_list_add(list, ci);
    }

    comm_reverse_list(list);

out_empty:
    fclose(fd);
    return 0;

out_err:
    clear_comm_list(list);
    fclose(fd);
    return -1;
}

int write_comm_list_to_file( const char *filename, struct comm_info *list )
{
    n2n_sock_str_t sock_str;

    FILE *fd = fopen(filename, "w");
    if (!fd)
    {
        traceEvent(TRACE_ERROR, "couldn't open community file");
        return -1;
    }

    unsigned int comm_num = comm_list_size(list);
    if (fprintf(fd, "comm_num=%d\n", comm_num) < 0)
    {
        traceEvent(TRACE_ERROR, "couldn't write community number to file");
        goto out_err;
    }

    unsigned int i;

    while (list)
    {
        fprintf(fd, "sn_num=%d name=%s\n", list->sn_num, list->community_name);

        for(i = 0; i < list->sn_num; i++)
        {
            sock_to_cstr(sock_str, list->sn_sock + i);

            fprintf(fd, "\t%s\n", sock_str);
        }

        list = list->next;
    }

    fclose(fd);
    return 0;

out_err:
    fclose(fd);
    return -1;
}

/** Add new to the head of list. If list is NULL; create it.
 *
 *  The item new is added to the head of the list. New is modified during
 *  insertion. list takes ownership of new.
 */
void comm_list_add( struct comm_info **list, struct comm_info *new )
{
    new->next = *list;
    *list = new;
}

size_t clear_comm_list( struct comm_info **comm_list )
{
    size_t retval = 0;
    struct comm_info *scan = *comm_list;

    while (scan)
    {
        struct comm_info *crt = scan;
        scan = scan->next;
        free(crt);
        retval++;
    }
    *comm_list = NULL;

    return retval;
}

size_t comm_list_size( const struct comm_info *list )
{
    size_t retval = 0;
    while (list)
    {
        ++retval;
        list = list->next;
    }
    return retval;
}

void comm_reverse_list( struct comm_info **list )
{
    struct comm_info *aux = *list, *next = NULL;
    *list = NULL;

    while (aux)
    {
        next = aux->next;
        comm_list_add(list, aux);
        aux = next;
    }
}

struct comm_info *comm_find( struct comm_info *list,
                             n2n_community_t   comm_name,
                             size_t            comm_name_len )
{
    while (list)
    {
        if (!memcmp(list->community_name, comm_name, comm_name_len))
        {
            return list;
        }
        list = list->next;
    }
    return NULL;
}

int add_new_community(comm_list_t        *communities,
                      n2n_community_t     comm_name,
                      struct comm_info  **comm)
{
    int retval = 0;

    struct comm_info *ci = comm_find(communities->list_head,
                                     comm_name,
                                     strlen((const char *) comm_name));
    if (!ci)
    {
        ci = calloc(1, sizeof(struct comm_info));
        if (!ci)
        {
            traceEvent(TRACE_ERROR, "not enough memory for new community info");
            return -1;
        }

        memcpy(&ci->community_name, comm_name, sizeof(n2n_community_t));
        comm_list_add(&communities->list_head, ci);

        retval = 1;
    }

    if (comm)
        *comm = ci;

    return retval;
}

static int add_new_supernode_to_community(struct comm_info *comm, n2n_sock_t *supernode)
{
    int i;

    if (comm->sn_num == N2N_MAX_SN_PER_COMM)
        return -1;

    for (i = 0; i < comm->sn_num; i++)
    {
        if (!sn_cmp(supernode, &comm->sn_sock[i]))
            return -1; /* dupe */
    }

    sn_cpy(&comm->sn_sock[comm->sn_num++], supernode);
    return 0;
}

int update_communities( comm_list_t       *communities,
                        snm_comm_name_t   *community_name,
                        n2n_sock_t        *supernode )
{
    struct comm_info *ci = NULL;

    add_new_community(communities, community_name->name, &ci);
    if (!ci)
        return -1;

    if (supernode)
    {
        add_new_supernode_to_community(ci, supernode);
    }

    return 0;
}

static int communities_to_array( uint16_t          *out_size,
                                 snm_comm_name_t  **out_array,
                                 struct comm_info  *communities )
{
    *out_size = comm_list_size(communities);
    if (alloc_communities(out_array, *out_size))
    {
        traceEvent(TRACE_ERROR, "could not allocate communities array");
        return -1;
    }

    snm_comm_name_t *cni = *out_array;

    while (communities)
    {
        cni->size = strlen((char *) communities->community_name);
        memcpy(cni->name, communities->community_name, sizeof(n2n_community_t));
        communities = communities->next;
        cni++;
    }
    return 0;
}

/*******************************************************************
 *                   SNM INFO related functions                    *
 *******************************************************************/

int snm_info_add_sn( n2n_SNM_INFO_t *info, struct sn_info *supernodes )
{
    info->sn_num = sn_list_size(supernodes);
    if (alloc_supernodes(&info->sn_ptr, info->sn_num))
    {
        traceEvent(TRACE_ERROR, "could not allocate supernodes array");
        return -1;
    }

    n2n_sock_t *sn = info->sn_ptr;

    while (supernodes)
    {
        sn_cpy(sn, &supernodes->sn);
        supernodes = supernodes->next;
        sn++;
    }
    return 0;
}

static int snm_info_add_comm( n2n_SNM_INFO_t *info, struct comm_info *communities )
{
    return communities_to_array(&info->comm_num, &info->comm_ptr, communities);
}

int build_snm_info( int              sock,         /* for ADV */
                    sn_list_t       *supernodes,
                    comm_list_t     *communities,
                    snm_hdr_t       *req_hdr,
                    n2n_SNM_REQ_t   *req,
                    snm_hdr_t       *info_hdr,
                    n2n_SNM_INFO_t  *info )
{
    int retval = 0;

    info_hdr->type    = SNM_TYPE_RSP_LIST_MSG;
    info_hdr->seq_num = req_hdr->seq_num;
    info_hdr->flags   = 0;

    memset(info, 0, sizeof(n2n_SNM_INFO_t));

    if (GET_E(req_hdr->flags))
    {
        /* INFO for edge */

        if (GET_N(req_hdr->flags))
        {
            if (req->comm_num != 1)
            {
                traceEvent(TRACE_ERROR, "Invalid edge request: Community number=%d",
                           req->comm_num);
                return -1;
            }

            snm_comm_name_t *comm = &req->comm_ptr[0];
            struct comm_info *ci = comm_find(communities->list_head, comm->name, comm->size);

            if (ci)
            {
                CLR_S(req_hdr->flags);
                SET_N(info_hdr->flags);
                SET_A(info_hdr->flags);

                /* set community supernodes ADV addresses */
                info->sn_num = ci->sn_num + 1;
                if (alloc_supernodes(&info->sn_ptr, info->sn_num))
                {
                    traceEvent(TRACE_ERROR, "could not allocate supernodes array");
                    return -1;
                }

                memcpy(info->sn_ptr, ci->sn_sock, ci->sn_num * sizeof(n2n_sock_t));
                sn_local_addr(sock, &info->sn_ptr[ci->sn_num]);

                /* set community name */
                info->comm_num = 1;
                if (alloc_communities(&info->comm_ptr, info->comm_num))
                {
                    traceEvent(TRACE_ERROR, "could not allocate community array");
                    return -1;
                }
                info->comm_ptr[0] = req->comm_ptr[0];
            }
            else
            {
                info->comm_num = comm_list_size(communities->list_head);
            }
        }
    }
    else
    {
        /* INFO for supernode */

        if (GET_C(req_hdr->flags))
        {
            SET_C(info_hdr->flags);

            /* Set communities list */
            retval += snm_info_add_comm(info, communities->list_head);
        }
        else if (GET_N(req_hdr->flags))
        {
            /* Set supernodes???TODO */
        }
    }

    if (GET_S(req_hdr->flags))
    {
        SET_S(info_hdr->flags);

        /* Set supernodes list */
        retval += snm_info_add_sn(info, supernodes->list_head);
    }

    return retval;
}

void clear_snm_info( n2n_SNM_INFO_t *info )
{
    info->sn_num = 0;
    free_supernodes(&info->sn_ptr);
    info->comm_num = 0;
    free_communities(&info->comm_ptr);
}

/*
 * Process response
 */
int  process_snm_rsp( sn_list_t       *supernodes,
                      comm_list_t     *communities,
                      n2n_sock_t      *sender,
                      snm_hdr_t       *hdr,
                      n2n_SNM_INFO_t  *rsp )
{
    int i;

    int new_sn = 0;

    /* Update list of supernodes */
    if (GET_S(hdr->flags))
    {
        new_sn = update_and_save_supernodes(supernodes, rsp->sn_ptr, rsp->sn_num);
    }

    /* Update list of communities */
    if (GET_C(hdr->flags))
    {
        for (i = 0; i < rsp->comm_num; i++)
        {
            update_communities(communities, &rsp->comm_ptr[i], sender);
        }
    }

    return new_sn;
}

/*******************************************************************
 *                    SNM ADV related functions                    *
 *******************************************************************/

static int snm_adv_add_comm(n2n_SNM_ADV_t *adv, struct comm_info *communities)
{
    return communities_to_array(&adv->comm_num, &adv->comm_ptr, communities);
}

int build_snm_adv(int                 sock,
                  struct comm_info   *comm_list,
                  snm_hdr_t          *hdr,
                  n2n_SNM_ADV_t      *adv)
{
    int retval = 0;

    hdr->type    = SNM_TYPE_ADV_MSG;
    hdr->flags   = 0;
    hdr->seq_num = 0;

    memset(adv, 0, sizeof(n2n_SNM_ADV_t));

    sn_local_addr(sock, &adv->sn);

    if (comm_list)
    {
        SET_N(hdr->flags);
        retval += snm_adv_add_comm(adv, comm_list);
    }

    return retval;
}

void clear_snm_adv(n2n_SNM_ADV_t *adv)
{
    adv->comm_num = 0;
    free_communities(&adv->comm_ptr);
}

int  process_snm_adv(sn_list_t         *supernodes,
                     comm_list_t       *communities,
                     n2n_sock_t        *sn,
                     n2n_SNM_ADV_t     *adv)
{
    int i;
    struct comm_info *ci = NULL;

    /* Adjust advertising address */
    if (sn_is_zero_addr(&adv->sn))
        sn_cpy_addr(&adv->sn, sn);

    /* Add senders address */
    update_and_save_supernodes(supernodes, sn, 1);

    int communities_updated = 0;

    /* Update list of communities from recvd from a supernode */
    for (i = 0; i < adv->comm_num; i++)
    {
        ci = comm_find(communities->list_head,
                       adv->comm_ptr[i].name,
                       adv->comm_ptr[i].size);

        if (!ci)
            continue;

        if (add_new_supernode_to_community(ci, &adv->sn) == 0)
            communities_updated = 1;
    }

    if (communities_updated)
    {
        /* elements added */
        write_comm_list_to_file(communities->filename, communities->list_head);
    }

    return communities_updated;
}


/*******************************************************************
 *                            Utils                                *
 *******************************************************************/

int sn_cmp(const n2n_sock_t *left, const n2n_sock_t *right)
{
    if (left->family != right->family)
        return (left->family - right->family);
    else if (left->port != right->port)
            return (left->port - right->port);
    else if (left->family == AF_INET)
        return memcmp(left->addr.v4, right->addr.v4, IPV4_SIZE);

    return memcmp(left->addr.v6, right->addr.v6, IPV6_SIZE);
}

void sn_cpy_addr(n2n_sock_t *dst, const n2n_sock_t *src)
{
    dst->family = src->family;
    if (src->family == AF_INET)
        memcpy(dst->addr.v4, src->addr.v4, IPV4_SIZE);
    else
        memcpy(dst->addr.v6, src->addr.v6, IPV6_SIZE);
}

void sn_cpy(n2n_sock_t *dst, const n2n_sock_t *src)
{
    dst->port = src->port;
    sn_cpy_addr(dst, src);
}

int sn_is_zero_addr(n2n_sock_t *sn)
{
    int i = (sn->family == AF_INET ? (IPV4_SIZE - 1) : (IPV6_SIZE - 1));

    for (; i >= 0; --i)
    {
        if (sn->addr.v6[i] != 0)
            return 0;
    }

    //return (*((unsigned int *) sn->addr.v4) == 0);
    return 1;
}

int sn_is_loopback(n2n_sock_t *sn, uint16_t local_port)
{
    if (sn->family == AF_INET  &&
        sn->port == local_port &&
        sn->addr.v4[0] == 127  &&
        sn->addr.v4[1] == 0    &&
        sn->addr.v4[2] == 0    &&
        sn->addr.v4[3] == 1)
    {
        return 1;
    }

    return 0;
}

int sn_local_addr(int sock, n2n_sock_t *sn)
{
    int retval = 0;

    struct sockaddr addr;
    unsigned int addrlen = sizeof(struct sockaddr);
    retval += getsockname(sock, &addr, &addrlen);

    struct sockaddr_in *sa = (struct sockaddr_in *) &addr;
    sa->sin_port = ntohs(sa->sin_port);

    sn_cpy(sn, (n2n_sock_t *) sa);

    return retval;
}
