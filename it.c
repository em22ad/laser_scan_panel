/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: it.c                                  Date: 27 Jul. 2007       **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "interval.h"

int main()
{
    int i;
    float r; 
    int_item_type *item;
    int_list_type *n = int_list_create();

    int_list_add_item(n, 20.0, 100.0);
    int_item_count_incr(n, int_list_find_item(n, 25.0));

    int_list_add_item(n, 12.0, 14.0);
    int_item_count_incr(n, int_list_find_item(n, 13.0));

    int_list_add_item(n, 10.0, 12.0);
    int_item_count_incr(n, int_list_find_item(n, 11.0));

    int_list_add_item(n, 13.0, 19.0);
    int_item_count_incr(n, int_list_find_item(n, 18.0));

    int_list_add_item(n, 0.0, 100.0);

    for (i = 0; i < 1000; i++)
    {
        r = (100.0*rand())/RAND_MAX;
        item = int_list_find_item(n, r);
        if (item != NULL)
            int_item_count_incr(n, item);
    }

    int_list_print(n);

    printf("-------------------------\n");
    int_list_cull_lowcount_item(n, 1.0);
    int_list_print(n);

    int_list_destroy(n);
    return 0;
}

