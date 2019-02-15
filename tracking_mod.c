/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: tracking_mod.c                           Date: 02 Apr. 2008    **
** Author: Dylan A. Shell, Muhammad Emad                                **
** Notes:                                                               **
*************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "tracking_mod.h"
#include "tracking_common.h"

// Forward decls.
void _tracking_mod_init(tracking_mod_t *mod);
void _tracking_mod_shutdown(tracking_mod_t *mod);

tracking_mod_t *tracking_mod_create()
{
    tracking_mod_t *ret;

    ret = (tracking_mod_t *)malloc(sizeof(tracking_mod_t));

    // Set members
    ret->points_X = NULL;
    ret->points_Y = NULL;
    ret->num_points = 0;
    ret->tracking = NULL;
    ret->clustering  = NULL;
    ret->temp_corr_calc  = NULL;

    _tracking_mod_init(ret);

    return ret;
}


void tracking_mod_destroy(tracking_mod_t *mod)
{
    assert(mod != NULL);
    _tracking_mod_shutdown(mod);

    free(mod);
}

void tracking_mod_process_new_data(double current_time, tracking_mod_t *mod, double *X, double *Y, int N_points)
{
    assert(mod != NULL);

    /* The following compile time switch is introduced for turning on/off the feature which lets
    us process line by line at 0.5 sec interval*/
    
    #ifdef INTERVAL_PROC
	static double time=-1.0;
	double interval;
	if (time == -1.0)
	{
		time=current_time;
	}
	
	interval= (current_time-time);
	    
	if (interval < 0.050)
	{
		// NO OP
	}else
	{
		time=current_time;
		mod->points_X = X;
		mod->points_Y = Y;
		mod->num_points = N_points;
		clustering_cluster_points(mod->clustering, X, Y, N_points);

		if (clustering_sets_populated(mod->clustering))
		{
		temp_corr_calculate(mod->tracking,mod->temp_corr_calc, clustering_clusters_old_get(mod->clustering), clustering_clusters_new_get(mod->clustering));
		tracking_process(mod->tracking, temp_corr_calc_peds_get(mod->temp_corr_calc));
		}
	}
    #else    
	    mod->points_X = X;
	    mod->points_Y = Y;
	    mod->num_points = N_points;
	    clustering_cluster_points(mod->clustering, X, Y, N_points);

	    if (clustering_sets_populated(mod->clustering))
	    {
	    	temp_corr_calculate(mod->tracking,mod->temp_corr_calc, clustering_clusters_old_get(mod->clustering), clustering_clusters_new_get(mod->clustering));
		tracking_process(mod->tracking, temp_corr_calc_peds_get(mod->temp_corr_calc));
	    }
    #endif
}

void tracking_mod_process_get_peds(tracking_mod_t *mod, double *X, double *Y, int *N, int max_output_size)
{
    assert(mod != NULL);

    // Pass the request on to the tracking bit
    tracking_get_peds(mod->tracking, X, Y, N, max_output_size);
}

void _tracking_mod_init(tracking_mod_t *mod)
{
    assert(mod != NULL);
    printf("[Tracking Module] Init\n");

    mod->clustering = clustering_create();
    mod->temp_corr_calc = temp_corr_calc_create();
    mod->tracking = tracking_create(); 
}


void _tracking_mod_shutdown(tracking_mod_t *mod)
{
    assert(mod != NULL);
    printf("[Tracking Module] Shutdown\n");

    tracking_destroy(mod->tracking); 
    temp_corr_calc_destroy(mod->temp_corr_calc); 
    clustering_destroy(mod->clustering); 
}

