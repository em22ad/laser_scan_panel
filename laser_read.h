/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: laser_read.h                          Date: 11 Jul. 2007       **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#ifndef _LASER_READ_H_
#define _LASER_READ_H_

#include "parse_log_file.h"

typedef struct _lr_state_type lr_state_type; /**< Private structure */

struct _lr_state_type 
{
    int fp;

    double time_stamp;
    long num_data_points;
    float *data_points;

    laser_data_type *file_data;
    long file_size;

    int seek_size;
    long curr_line;
    long num_lines;
};

typedef long lr_position_state_type; 


lr_state_type *lr_fopen(const char* fname);
void lr_fclose(lr_state_type *lr);

void lr_seek_size_set(lr_state_type *lr, int size);
int lr_seek_size_get(lr_state_type *lr);


inline void lr_step(lr_state_type *lr);
inline long lr_num_data_points_get(lr_state_type *lr);
inline float *lr_data_points_get(lr_state_type *lr);
inline double lr_time_stamp_get(lr_state_type *lr);

void lr_read_to_time(lr_state_type *lr, double time);

double lr_get_first_time(lr_state_type *lr);
double lr_get_last_time(lr_state_type *lr);

lr_position_state_type lr_save_position_state(lr_state_type *lr);
void lr_load_position_state(lr_state_type *lr, lr_position_state_type state);


#endif
