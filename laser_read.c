/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: laser_read.c                          Date: 11 Jul. 2007       **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#include "laser_read.h"

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <math.h>
#include "parse_log_file.h"

#define NO_COPY 1

/**
 * Returns a newly allocated laser reader context block 
 */
lr_state_type *lr_fopen(const char* fname)
{
    lr_state_type *ret;
    struct stat stat;

    ret = (lr_state_type *)malloc(sizeof(lr_state_type));


    ret->fp = open(fname, O_RDONLY);

    if (fstat(ret->fp, &stat) != 0)
    {
        free(ret);
        perror("stat failed");
        return NULL;
    }

    ret->file_size = stat.st_size;

    //printf("Length = %ld\n", ret->file_size);

    ret->file_data = mmap(NULL, ret->file_size, PROT_READ, MAP_PRIVATE, ret->fp, 0);
    if (ret->file_data < 0)
    {
        free(ret);
        perror("mmap failed");
        return NULL;
    }

    ret->num_data_points = LASER_ARRAY_LEN;
    ret->data_points = (float *)calloc(LASER_ARRAY_LEN, sizeof(float));
    ret->time_stamp = -1.0;

    ret->seek_size = 1;
    ret->curr_line = -1;
    ret->num_lines  = ret->file_size/sizeof(laser_data_type);

    return ret;
}

/**
 * Finished with laser reader
 */
void lr_fclose(lr_state_type *lr)
{
    assert(lr != NULL);

    munmap(lr->file_data, lr->file_size);

    close(lr->fp);

#ifndef NO_COPY
    free(lr->data_points);
#endif    
// Otherwise we didn't do a memcpy, so the data might be free'd by the
// user.


    free(lr);
}

/**
 * Set the data skip size 
 */
void lr_seek_size_set(lr_state_type *lr, int size)
{
    assert(lr != NULL);

    lr->seek_size = size;
}

/**
 * Get the data skip size 
 */
int lr_seek_size_get(lr_state_type *lr)
{
    assert(lr != NULL);

    return lr->seek_size;
}


void _lr_fetch_line(lr_state_type *lr, long i)
{
    assert(lr != NULL);

#ifdef NO_COPY
    // This is less safe, but faster:
    lr->data_points =  &(lr->file_data[i].data[0]);
#elif
    memcpy(lr->data_points, &(lr->file_data[i].data), sizeof(float) * LASER_ARRAY_LEN);
#endif

    lr->time_stamp = lr->file_data[i].timestamp;
}

inline void lr_step(lr_state_type *lr)
{
    long old_line;
    assert(lr != NULL);

    old_line = lr->curr_line;
    lr->curr_line +=  lr->seek_size;

    //Clip to range
    if (lr->curr_line < 0)
            lr->curr_line  = 0;
    if (lr->curr_line >= lr->num_lines)
            lr->curr_line  = lr->num_lines-1;



    if (lr->curr_line != old_line)
    { // update the data.
        _lr_fetch_line(lr, lr->curr_line);
    }
}

inline long lr_num_data_points_get(lr_state_type *lr)
{
    assert(lr != NULL);

    return lr->num_data_points;
}

inline float *lr_data_points_get(lr_state_type *lr)
{
    assert(lr != NULL);

    return lr->data_points;
}


inline double lr_time_stamp_get(lr_state_type *lr)
{
    assert(lr != NULL);

    return lr->time_stamp;
}


double lr_get_first_time(lr_state_type *lr)
{
    assert(lr != NULL);

    return lr->file_data[0].timestamp;
}

double lr_get_last_time(lr_state_type *lr)
{
    assert(lr != NULL);

    return lr->file_data[lr->num_lines-1].timestamp;
}

long _lr_search_for_time(lr_state_type * lr, double needle_time)
{
    long low;
    long high;
    long mid;
    double midVal;

    assert(lr != NULL);

    low = 0;
    high = lr->num_lines-1;

    while (low <= high)
    {
        mid = low + ((high - low) / 2);
        midVal = lr->file_data[mid].timestamp;

        if (midVal < needle_time)
            low = mid + 1;
        else if (midVal > needle_time)
            high = mid - 1;
        else
        {
            return mid;
        }
    }

    // Time not found, so we want the next seek to give the
    // timestamp after needle_time, where after depends on the
    // size sign of seek_size

    if (lr->seek_size > 0)
    {
        return low;
    }
    else
    {
        return high;
    }
}



// Read so as to ensure that time_stamp is the closest one to the given time
void lr_read_to_time(lr_state_type *lr, double time)
{
    long i;
    assert(lr != NULL);

    // As an optimization we compare how close the 
    // time is to the current time. Now the decisoin should depend
    // on the log(num_lines) versus freq*fabs(time - current_time)
    // We use a cheap hack:

    if (fabs(time - lr_time_stamp_get(lr) > 1.0))
    { // Do a binary search on the whole array.

        i = _lr_search_for_time(lr, time);

        lr->curr_line = i;
        _lr_fetch_line(lr, i);
    }
    else
    { // Do a linear search from here
      // BTW We ignore the step size.
        if ((time > lr_time_stamp_get(lr)))
        {
            i = lr->curr_line;
            while (i < lr->num_lines-1)
            {
                if (lr->file_data[i].timestamp < time)
                    i++;
                else
                    break;
            }
            // i either has a >= timestamp, or i is the last entry
            lr->curr_line = i;
            _lr_fetch_line(lr, i);
        }
        else if ((time < lr_time_stamp_get(lr)))
        {
            i = lr->curr_line;
            while (i > 0)
            {
                if (lr->file_data[i].timestamp > time)
                    i--;
                else
                    break;
            }
            // i either has a <= timestamp, or i is 0
            lr->curr_line = i;
            _lr_fetch_line(lr, i);
        }
        else // Perfect as is
        {
            return;
        }
    }
}

lr_position_state_type lr_save_position_state(lr_state_type *lr)
{
    assert(lr != NULL);
    return (lr_position_state_type)lr->curr_line;
}

void lr_load_position_state(lr_state_type *lr, lr_position_state_type state)
{
    assert(lr != NULL);

    lr->curr_line = (long)state;

    //Clip to range
    if (lr->curr_line < 0)
            lr->curr_line  = 0;
    if (lr->curr_line >= lr->num_lines)
            lr->curr_line  = lr->num_lines-1;

    _lr_fetch_line(lr, (long)(lr->curr_line));
}





