/*
 * sn_multiple.c
 *
 *  Created on: Mar 25, 2012
 *      Author: Costin Lupu
 */

#include "sn_multiple.h"


/*
 * Operations on sn_info lists.
 */

int read_sn_list_from_file( const char *filename, struct sn_info **list )
{
    return 0;
}

int write_sn_list_to_file( const char *filename, struct sn_info *list )
{
    return 0;
}

void sn_list_add( struct sn_info **list, struct sn_info  *new )
{

}

size_t sn_list_size( const struct sn_info *list )
{
    return 0;
}

size_t clear_sn_list( struct sn_info **sn_list )
{
    return 0;
}



/* Operations on comm_info lists. */
int read_comm_list_from_file( const char *filename, struct comm_info **list )
{
    return 0;
}

int write_comm_list_to_file( const char *filename, struct comm_info *list )
{
    return 0;
}

size_t clear_comm_list( struct comm_info **comm_list )
{
    return 0;
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

/** Add new to the head of list. If list is NULL; create it.
 *
 *  The item new is added to the head of the list. New is modified during
 *  insertion. list takes ownership of new.
 */
void comm_list_add( struct comm_info **list, struct comm_info  *new )
{
    new->next = *list;
    *list = new;
}

int update_communities( struct comm_info **list, const n2n_community_t community )
{
    return 0;
}

int process_sn_msg( /*n2n_sn_t * sss,*/
                    const struct sockaddr_in *sender_sock,
                    const uint8_t *udp_buf,
                    size_t udp_size/*
                    time_t now*/)
{
    return 0;
}