/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: tracking.h                               Date: 28 Mar. 2008    **
** Author: Dylan A. Shell/Muhammad Emad                                 **
** Notes:                                                               **
*************************************************************************/
#include "tracking_common.h"

#ifndef _TRACKING_H_
#define _TRACKING_H_

#define NUM_PARTICLES 300
#define INIT_PARTICLE_DEV 0.2 /* How far spread particales are intially w.r.t mean value, 0.05 + 0.05= 0.1 will be the max distance between right and left foot partcles */
#define PERIOD_RANDOMNESS 0.01

typedef struct _particle_t particle_t;
//Data for a single particle
struct _particle_t
{
    double x[2]; // 0 index points to right foot, 1 points to left foot
    double y[2];
    double weight;
    double phase;
    double period;
    double stride;
    double dir;
};

typedef struct _person_t person_t;
//Details kept for a single person
struct _person_t
{
   double X;
   double Y;
   double est_X;
   double est_Y;
   particle_t particles[NUM_PARTICLES];
   int life;
   int updated;
   int first_update; 
   int to_be_deleted;
   int initialized;
};

typedef struct _tracking_t tracking_t;
// Tracking 
struct _tracking_t
{ 
   person_t people[MAX_PED_VAL];
   int num_ppl;
   int start_peds_index;
};


tracking_t *tracking_create();
//int check_reliability(tracking_t * tr,oldx,oldy, index);
void tracking_destroy(tracking_t *tr);
void tracking_process(tracking_t *tr, ped_set_t *peds);
void tracking_display_peds(tracking_t * tr, ped_set_t * peds);
void tracking_get_peds(tracking_t *tr, double *X, double *Y, int *N, int max_output_size);


#endif
