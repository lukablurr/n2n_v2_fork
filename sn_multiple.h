/*
 *sn_multiple.h
 *
 * Created on: Mar 25, 2012
 *     Author: Costin Lupu
 */

#ifndef SN_MULTIPLE_H_
#define SN_MULTIPLE_H_

#include "n2n_wire.h"
#include "sn_multiple_wire.h"

#define N2N_SN_COMM_PORT                5646


struct sn_info
{
    struct sn_info     *next;
    n2n_sock_t          sn;
};

/* Operations on sn_info lists. */
int read_sn_list_from_file( const char *filename, struct sn_info **list );
int write_sn_list_to_file( const char *filename, struct sn_info *list );
void sn_list_add( struct sn_info **list, struct sn_info *new );
size_t sn_list_size( const struct sn_info *list );
size_t clear_sn_list( struct sn_info **sn_list );

struct comm_info
{
    struct comm_info   *next;
    size_t              sn_num;
    n2n_community_t     community_name;
   /*time_t              last_seen;*/
};

/* Operations on comm_info lists. */
int read_comm_list_from_file( const char *filename, struct comm_info **list );
int write_comm_list_to_file( const char *filename, struct comm_info *list );
void comm_list_add( struct comm_info **list, struct comm_info *new );
size_t comm_list_size( const struct comm_info *list );
size_t clear_comm_list( struct comm_info **comm_list );

int update_communities( struct comm_info **list, const n2n_community_t community );

//int process_sn_msg( /*n2n_sn_t * sss,*/
  //                  const struct sockaddr_in *sender_sock,
    //                const uint8_t *udp_buf,
      //              size_t udp_size/*
        //            time_t now*/);

void build_snm_rsp(struct sn_info   *supernodes,
                   struct comm_info *communities,
                   n2n_SNM_REQ_t    *req,
                   n2n_SNM_RSP_t    *rsp);

void update_snm_info(struct sn_info    **supernodes,
                     struct comm_info  **communities,
                     n2n_SNM_RSP_t      *rsp);

#endif /*SN_MULTIPLE_H_ */
