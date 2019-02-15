/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: tracking_mod.h                           Date: 19 Sep. 2007    **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#ifndef _TRACKING_MOD_H_
#define _TRACKING_MOD_H_

#include "tracking.h"
#include "clustering.h"
#include "temp_corr_calc.h"

typedef struct _tracking_mod_t tracking_mod_t;
// Tracking Module
struct _tracking_mod_t
{ 
    double *points_X;
    double *points_Y; 
    int num_points;

    tracking_t *tracking;
    temp_corr_calc_t *temp_corr_calc;
    clustering_t *clustering;
};


tracking_mod_t *tracking_mod_create();
void tracking_mod_destroy(tracking_mod_t *mod);
void tracking_mod_process_new_data(double current_time, tracking_mod_t *mod, double *X, double *Y, int N_points);
void tracking_mod_process_get_peds(tracking_mod_t *mod, double *X, double *Y, int *N, int max_output_size);

#endif
