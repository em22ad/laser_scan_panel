/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: temp_corr_calc.h                         Date: 11 May. 2007    **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/
 
#ifndef _TEMP_CORR_CALC_H_
#define _TEMP_CORR_CALC_H_

#include "tracking_common.h"
#include "tracking.h"

typedef struct _temp_corr_calc_t temp_corr_calc_t;
typedef struct _update update;
typedef struct _list list;
// Temporal Correlation Calculation
struct _temp_corr_calc_t
{ 
    ped_set_t *peds;
};

struct _list
{
	GList* row;
};

struct _update
{
	int pair_index;
	double dist_frm_ped;
	int ped_index;
};

temp_corr_calc_t *temp_corr_calc_create();

int mymax (int x, int y);

void temp_corr_calc_destroy(temp_corr_calc_t *tc);

void temp_corr_calculate(tracking_t * tr,temp_corr_calc_t *tc, cluster_set_t *cs_prev, cluster_set_t *cs_curr);

ped_set_t *temp_corr_calc_peds_get(temp_corr_calc_t *tc);


#endif
