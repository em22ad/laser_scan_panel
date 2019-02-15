/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: laser_reader_test.c                   Date: 11 Jul. 2007       **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "laser_read.h"

#define INPUT_FILE "logs/laser_intel1_07_02_07_MonCSC.dat"

void print_data(double ts, float *data_array, int n)
{
    int i;
    printf("%lf [ ",ts);

    for (i = 0; i < n; i++)
        printf("%f ", data_array[i]);

    printf("] \n");
}

int main()
{
    double old_ts, ts;
    float *data;
    float *history_array;
    int n, i;
    lr_state_type *lr;
    
    lr = lr_fopen(INPUT_FILE);
    assert(lr != NULL);

    lr_seek_size_set(lr, 1);

    n = lr_num_data_points_get(lr);
    history_array = (float *)malloc(sizeof(float)*n);
    for (i = 0; i < n; i++)
        history_array[i] = -1.0;

    old_ts = -1.0;
    while (1)
    {
        lr_step(lr);

        ts = lr_time_stamp_get(lr);
        data = lr_data_points_get(lr);

        if (ts == old_ts) break;

        //print_data(ts, data, n);
        
        for (i = 0; i < n; i++)
            if (data[i] > history_array[i])
                history_array[i] = data[i];

        old_ts = ts;
    }

    printf("[ ");

    for (i = 0; i < n; i++)
        printf("%f ", history_array[i]);

    printf("] \n");


    lr_fclose(lr);

    return 0;
}

