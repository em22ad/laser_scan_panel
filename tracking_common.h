/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: tracking_common.h                        Date: 29 Mar. 2008    **
** Author: Dylan A. Shell/Muhammad Emad                                 **
** Notes:                                                               **
*************************************************************************/

#include <glib.h>

#ifndef _TRACKING_COMMON_H_
#define _TRACKING_COMMON_H_
#define INTERVAL_PROC

#define MAX_PED_VAL  8500 /*maximum allowed pedestrians over any given period of time*/
#define MAX_CLUST_VAL 10 /*maximum clusters per ped */
#define MAX_CLUSTERS_VAL 150/*maximum number of clusters that can be detected at A time*/
#define PED_LIFE 70 /*maximum number of clusters that can be detected at A time	*/
#define MAX_CLUST_PER_PED 10 /*maximum number of clusters that can assigned to a ped	*/
#define MAX_PED_PER_IT 100 //maximum peds that can be detected in each iteration

typedef struct _point_t point_t;
struct _point_t
{
    double X;
    double Y;
};

typedef struct _cluster_t cluster_t;
struct _cluster_t 
{ 
    double X;
    double Y;
    double mean_X;
    double mean_Y;
    double dim_X;	// cluster's major axis
    double dim_Y;	// cluster's minor axis

    int num_pts; //number of points belonging to this cluster
    GList *points_list;
};

typedef struct _pedestrian_t pedestrian_t;
struct _pedestrian_t 
{ 
    int clust_card; //number of clusters that in cluster_list
    int deleted;		
    GList *cluster_list;
};

typedef struct _cluster_set_t cluster_set_t;
//Collection of Clusters
struct _cluster_set_t
{
    GList *cluster_list;  //get checked
    int numClust;	//number of detected clusters
    int populated; // 1 for yes, 0 for No
};

typedef struct _ped_set_t ped_set_t;
//Collection of Pedestrian's
struct _ped_set_t
{
    GList *ped_list;
    int updated[MAX_PED_VAL];
    int num_ped;
};


#endif
