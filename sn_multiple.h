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
#define N2N_PERSIST_FILENAME_LEN        64

struct sn_info
{
    struct sn_info     *next;
    n2n_sock_t          sn;
    //TODO set listening port also
};

typedef struct sn_list
{
    struct sn_info     *list_head;
    size_t              bin_size;
    char                filename[N2N_PERSIST_FILENAME_LEN];
} sn_list_t;

/* Operations on sn_info lists. */
int read_sn_list_from_file( const char *filename, struct sn_info **list );
int write_sn_list_to_file( const char *filename, struct sn_info *list );
void sn_list_add( struct sn_info **list, struct sn_info *new );
size_t clear_sn_list( struct sn_info **sn_list );
size_t sn_list_size( const struct sn_info *list );
void sn_reverse_list( struct sn_info **list );
struct sn_info *sn_find( struct sn_info *list, n2n_sock_t *sn );

int update_sn_list( sn_list_t *list, n2n_sock_t *sn );

struct comm_info
{
    struct comm_info   *next;
    size_t              sn_num;
    n2n_community_t     community_name;
   /*time_t              last_seen;*/
};

typedef struct comm_list
{
    struct comm_info   *list_head;
    size_t              bin_size;
    char                filename[N2N_PERSIST_FILENAME_LEN];
} comm_list_t;

/* Operations on comm_info lists. */
int read_comm_list_from_file( const char *filename, struct comm_info **list );
int write_comm_list_to_file( const char *filename, struct comm_info *list );
void comm_list_add( struct comm_info **list, struct comm_info *new );
size_t clear_comm_list( struct comm_info **comm_list );
size_t comm_list_size( const struct comm_info *list );
void comm_reverse_list( struct comm_info **list );
int update_comm_list( comm_list_t       *comm_list,
                      size_t             sn_num,
                      snm_comm_name_t   *community_name );

int update_communities( comm_list_t *communities, n2n_community_t *comm_name );

/*******************************************************************
 *                   SNM INFO related functions                    *
 *******************************************************************/
int build_snm_info(sn_list_t       *supernodes,
                   comm_list_t     *communities,
                   snm_hdr_t       *req_hdr,
                   n2n_SNM_REQ_t   *req,
                   n2n_SNM_INFO_t  *info);
void clear_snm_info(n2n_SNM_INFO_t *info);

void process_snm_rsp(sn_list_t        *supernodes,
                     comm_list_t      *communities,
                     n2n_SNM_INFO_t   *snm_info);

/*******************************************************************
 *                    SNM ADV related functions                    *
 *******************************************************************/
int build_snm_adv(int sock, comm_list_t *communities, n2n_SNM_ADV_t *adv);//TODO maybe add hdr also
void clear_snm_adv(n2n_SNM_ADV_t *adv);
void process_snm_adv(sn_list_t *supernodes, n2n_SNM_ADV_t *adv);

/*void send_snm_req(int sock, n2n_sock_t *sn);*/

#endif /*SN_MULTIPLE_H_ */
