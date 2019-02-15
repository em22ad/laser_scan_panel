/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: clustering.h                             Date: 19 Sep. 2007    **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#ifndef _CLUSTERING_H_
#define _CLUSTERING_H_

#include "tracking_common.h"

typedef struct _clustering_t clustering_t;
// Clustering
struct _clustering_t
{ 
    cluster_set_t *cluster_set[2];
    int current_cluster_set;
};

clustering_t *clustering_create();
void clustering_destroy(clustering_t *cl);
void clustering_destroy_part(clustering_t *cl);
void clustering_cluster_points(clustering_t *cl, double *X, double *Y, int N);
cluster_set_t *clustering_clusters_old_get(clustering_t *cl);
cluster_set_t *clustering_clusters_new_get(clustering_t *cl);
int clustering_sets_populated(clustering_t *cl);

void clustering_print_debug_point(point_t *pt);
void clustering_print_debug_cluster(cluster_t *cl);
void clustering_print_debug_set(cluster_set_t *out);

void clustering_delete_cluster_list(GList *cl);

#endif
