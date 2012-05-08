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

int read_sn_list_from_file( const char *filename, struct sn_info **list )
{
    struct sn_info *sni = NULL, *tmp_list = NULL, *p = NULL;
    n2n_sock_str_t sock_str;

    FILE *fd = fopen(filename, "r");
    if (!fd)
    {
        traceEvent(TRACE_ERROR, "couldn't open supernodes file");
        return -1;
    }

    while (fscanf(fd, "%s\n", sock_str) > 0)
    {
        sni = calloc(1, sizeof(struct sn_info));
        if (!sni)
        {
            traceEvent(TRACE_ERROR, "couldn't allocate a new supernode entry");
            goto out_err;
        }

        sock_from_cstr(&sni->sn, sock_str);

        p = *list;
        while (p)
        {
            if (memcpy(&p->sn, &sni->sn, sizeof(n2n_sock_t)) == 0)
            {
                free(sni);
                sni = NULL;
                break;
            }
            p = p->next;
        }

        if (sni)
        {
            sn_list_add(&tmp_list, sni);
            traceEvent(TRACE_DEBUG, "Added supernode %s", sock_to_cstr(sock_str, &sni->sn));
        }
    }

    sn_list_add(list, tmp_list);
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
        if (!memcmp(&list->sn, sn, sizeof(n2n_sock_t)))
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

    memcpy(&new->sn, sn, sizeof(n2n_sock_t));
    sn_list_add(list, new);

    return new;
}

int update_supernodes(sn_list_t *supernodes, n2n_sock_t *sn)
{
    struct sn_info *item = sn_find(supernodes->list_head, sn);
    if (item)
    {
        item->last_seen = time(NULL);
        return 0;
    }

    //TODO remove
    int item_size = sizeof(sn->family) + sizeof(sn->port);
    if (sn->family == AF_INET)
    {
        item_size += IPV4_SIZE;
    }
    else if (sn->family == AF_INET6)
    {
        item_size += IPV6_SIZE;
    }
    else
    {
        traceEvent(TRACE_ERROR, "unsupported family type for supernode address");
        return -1;
    }

    struct sn_info *new = calloc(1, sizeof(struct sn_info));
    if (!new)
    {
        traceEvent(TRACE_ERROR, "not enough memory for new SN info");
        return -1;
    }

    new->last_seen = time(NULL);
    memcpy(&new->sn, sn, sizeof(n2n_sock_t));
    sn_list_add(&supernodes->list_head, new);
    supernodes->bin_size += item_size;

    return 1;
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

    struct sn_info *x = *list;
    for(i = 0; i < middle; i++)
    {
        sn_list_add(&left, x);
        x = x->next;
    }
    sn_reverse_list(&left);

    for(; i < size; i++)
    {
        sn_list_add(&right, x);
        x = x->next;
    }
    sn_reverse_list(&right);

    left  = merge_sort(&left,  middle, func);
    right = merge_sort(&right, size - middle, func);

    return merge(&left, middle, &right, size - middle, func);
}

struct sn_info *merge(struct sn_info **left, size_t left_size, struct sn_info **right, size_t right_size, sn_cmp_func func)
{
    struct sn_info *result = NULL;

#define first(sni)   *sni

    while( left_size > 0 || right_size > 0)
    {
        if (left_size > 0 && right_size > 0)
        {
            if (func(first(left), first(right)) <= 0)
            {
                sn_list_add(&result, first(left));
                *left = (*left)->next;
                left_size--;
            }
            else
            {
                sn_list_add(&result, first(right));
                *right = (*right)->next;
                right_size--;
            }
        }
        else if (left_size > 0)
        {
            sn_list_add(&result, first(left));
            *left = (*left)->next;
            left_size--;
        }
        else if (right_size > 0)
        {
            sn_list_add(&result, first(right));
            *right = (*right)->next;
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
    struct comm_info *ci = NULL;

    FILE *fd = fopen(filename, "r");
    if (!fd)
    {
        traceEvent(TRACE_ERROR, "couldn't open community file. %s", strerror(errno));
        return -1;
    }

    unsigned int comm_num = 0;
    if (fread(&comm_num, sizeof(unsigned int), 1, fd) != 1)
    {
        traceEvent(TRACE_ERROR, "couldn't read communities number from file");
        goto out_err;
    }

    int i;
    for (i = 0; i < comm_num; i++)
    {
        ci = calloc(1, sizeof(struct comm_info));
        if (!ci)
        {
            traceEvent(TRACE_ERROR, "couldn't allocate a new community entry");
            goto out_err;
        }

        if (fread(&ci->sn_num, sizeof(size_t), 1, fd) != 1)
        {
            traceEvent(TRACE_ERROR, "couldn't read SN number from file");
            goto out_err;
        }

        if (fread(ci->sn_sock, sizeof(n2n_sock_t), N2N_MAX_SN_PER_COMM, fd) != N2N_MAX_SN_PER_COMM)
        {
            traceEvent(TRACE_ERROR, "couldn't read SN sockets from file");
            goto out_err;
        }

        if (fread(ci->community_name, sizeof(n2n_community_t), 1, fd) != 1)
        {
            traceEvent(TRACE_ERROR, "couldn't read community name from file");
            goto out_err;
        }

        comm_list_add(list, ci);
    }

    comm_reverse_list(list);

    fclose(fd);
    return 0;

    out_err: clear_comm_list(list);
    fclose(fd);
    return -1;
}

int write_comm_list_to_file( const char *filename, struct comm_info *list )
{
    FILE *fd = fopen(filename, "w");
    if (!fd)
    {
        traceEvent(TRACE_ERROR, "couldn't open community file");
        return -1;
    }

    unsigned int comm_num = comm_list_size(list);
    if (fwrite(&comm_num, sizeof(unsigned int), 1, fd) != 1)
    {
        traceEvent(TRACE_ERROR, "couldn't write community number to file");
        goto out_err;
    }

    while (list)
    {
        if (fwrite(&list->sn_num, sizeof(size_t), 1, fd) != 1)
        {
            traceEvent(TRACE_ERROR, "couldn't write SN number to file");
            goto out_err;
        }
        if (fwrite(&list->sn_sock[0], sizeof(n2n_sock_t), N2N_MAX_SN_PER_COMM, fd) != N2N_MAX_SN_PER_COMM)
        {
            traceEvent(TRACE_ERROR, "couldn't write SN sockets to file");
            goto out_err;
        }
        if (fwrite(list->community_name, sizeof(n2n_community_t), 1, fd) != 1)
        {
            traceEvent(TRACE_ERROR, "couldn't write community name to file");
            goto out_err;
        }
        list = list->next;
    }

    fclose(fd);
    return 0;

    out_err: fclose(fd);
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

struct comm_info *find_comm( struct comm_info *list,
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

int update_comm_list( comm_list_t *comm_list, size_t sn_num,
                      snm_comm_name_t *community_name )
{
    struct comm_info *info = find_comm(comm_list->list_head, community_name->name, community_name->size);
    if (info)
    {
        info->sn_num += sn_num;
        return 0;
    }

    struct comm_info *new = calloc(1, sizeof(struct comm_info));
    if (new)
    {
        traceEvent(TRACE_ERROR, "not enough memory for new community info");
        return -1;
    }

    memcpy(&new->community_name, community_name->name, community_name->size);
    new->sn_num = sn_num;

    comm_list_add(&comm_list->list_head, new);
    return 0;
}

int update_communities( comm_list_t *communities, n2n_community_t comm_name )
{
    struct comm_info *info = find_comm(communities->list_head, comm_name, strlen((char *) comm_name));
    if (info)
    {
        return 0;
    }

    struct comm_info *new = calloc(1, sizeof(struct comm_info));
    if (new)
    {
        traceEvent(TRACE_ERROR, "not enough memory for new community info");
        return -1;
    }

    memcpy(&new->community_name, comm_name, sizeof(n2n_community_t));
    new->sn_num = 1;

    comm_list_add(&communities->list_head, new);
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
        memcpy(sn, &supernodes->sn, sizeof(n2n_sock_t));
        supernodes = supernodes->next;
        sn++;
    }
    return 0;
}

static int snm_info_add_comm( n2n_SNM_INFO_t *info, struct comm_info *communities )
{
    return communities_to_array(&info->comm_num, &info->comm_ptr, communities);
}

int build_snm_info( sn_list_t *supernodes, comm_list_t *communities,
                    snm_hdr_t *req_hdr, n2n_SNM_REQ_t *req,
                    n2n_SNM_INFO_t *info )
{
    int retval = 0;

    memset(info, 0, sizeof(n2n_SNM_INFO_t));

    if (GET_S(req_hdr->flags))
    {
        /* Set supernodes list */
        retval += snm_info_add_sn(info, supernodes->list_head);
    }
    if (GET_C(req_hdr->flags))
    {
        /* Set communities list */
        retval += snm_info_add_comm(info, communities->list_head);
    }
    else if (GET_N(req_hdr->flags))
    {
        /* Set supernodes???TODO */
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
void process_snm_rsp( sn_list_t *supernodes, comm_list_t *communities,
                      n2n_SNM_INFO_t *snm_info )
{
    int i;

    int need_write = 0;

    /* Update list of supernodes */
    for (i = 0; i < snm_info->sn_num; i++)
    {
        need_write = update_supernodes(supernodes, &snm_info->sn_ptr[i]);
    }

    if (need_write)
    {
        /* elements added */
        write_sn_list_to_file(supernodes->filename, supernodes->list_head);
    }

    /* Update list of communities from recvd from a supernode */
    for (i = 0; i < snm_info->comm_num; i++)
    {
        update_comm_list(communities, 1, &snm_info->comm_ptr[i]);
    }
}

/*******************************************************************
 *                    SNM ADV related functions                    *
 *******************************************************************/

static int snm_adv_add_comm(n2n_SNM_ADV_t *adv, struct comm_info *communities)
{
    return communities_to_array(&adv->comm_num, &adv->comm_ptr, communities);
}

int build_snm_adv(int sock, comm_list_t *communities, n2n_SNM_ADV_t *adv)
{
    int retval = 0;

    memset(adv, 0, sizeof(n2n_SNM_ADV_t));

    struct sockaddr addr;
    unsigned int addrlen = sizeof(struct sockaddr);
    retval += getsockname(sock, &addr, &addrlen);

    struct sockaddr_in *sa = (struct sockaddr_in *) &addr;

    adv->sn.family = sa->sin_family;
    memcpy(adv->sn.addr.v4, &sa->sin_addr, sizeof(struct in_addr));
    adv->sn.port = sa->sin_port;

    retval += snm_adv_add_comm(adv, communities->list_head);

    return retval;
}

void clear_snm_adv(n2n_SNM_ADV_t *adv)
{
    adv->comm_num = 0;
    free_communities(&adv->comm_ptr);
}

void process_snm_adv(sn_list_t *supernodes, struct peer_info *edges, n2n_SNM_ADV_t *adv)
{
    int i;

    /* Add senders address */
    int need_write = update_supernodes(supernodes, &adv->sn);
    if (need_write)
    {
        /* elements added */
        write_sn_list_to_file(supernodes->filename, supernodes->list_head);
    }

    /* Update list of communities from recvd from a supernode */
    for (i = 0; i < adv->comm_num; i++)
    {
        /*update_comm_list(communities, 1, &adv->comm_ptr[i])*/;
    }
}
