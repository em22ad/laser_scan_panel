/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: interval.c                               Date: 27 Jul. 2007    **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "interval.h"
#include "utils.h"


void _int_list_fix(int_list_type *lst);

int_list_type *int_list_create()
{
    int_list_type *ret;   

    ret = (int_list_type *)malloc(sizeof(int_list_type));

    ret->head = NULL;
    ret->tail = NULL;
    ret->num = 0;
    ret->total_count = 0;

    return ret;
}

void int_list_destroy(int_list_type *lst)
{
    assert(lst != NULL);

    int_list_empty(lst);

    free(lst);
}

void int_list_empty(int_list_type *lst)
{
    assert(lst != NULL);

    while (lst->head != NULL)
        int_list_remove_item(lst, lst->head);

}

void int_list_remove_item(int_list_type *lst, int_item_type *item)
{
    assert(lst != NULL);
    assert(item != NULL);

    if (lst->head == item)
        lst->head = item->next;
    if (lst->tail == item)
        lst->tail = item->prev;
    
    if (item->prev != NULL)
        item->prev->next = item->next;
    if (item->next != NULL)
        item->next->prev = item->prev;

    lst->total_count -= item->count;
    lst->num--;

    free(item);
}

void int_check(int_list_type *lst)
{
    int c, t;
    int_item_type *it;

    assert(lst != NULL);
    c = 0;
    t = 0;

    it = lst->head;
    while (it != NULL)
    {
        t++;
        c += it->count;
        it = it->next;
    }

    assert( c == lst->total_count );
    assert( t == lst->num );

}

void int_list_add_item(int_list_type *lst, double st_value, double end_value)
{
    int end;
    int_item_type *prior_it, *next_it;
    int_item_type *new_item;
    assert(lst != NULL);

    end = 1;
    next_it = lst->head;
    // Find the item to add after
    while (next_it != NULL)
    {
        if ((next_it->start_value == st_value) && (next_it->end_value == end_value))
        { // Duplicate of item in the list.
            next_it->count++;
            lst->total_count++;
            return;
        }

        if (next_it->start_value >= st_value)
        {
            end = 0;
            break;
        };

        next_it = next_it->next;
    }

    new_item = (int_item_type *)malloc(sizeof(int_item_type));
    new_item->start_value = st_value;
    new_item->end_value = end_value;
    new_item->data = NULL;
    new_item->count = 0;


    if (next_it != NULL)
        prior_it = next_it->prev;
    else
    {
        if (end)
            prior_it = lst->tail;
        else
            prior_it = NULL;
        
    }
        
    if (next_it != NULL)
    {
        new_item->next = next_it;
        next_it->prev = new_item;
    }
    else
    {
        new_item->next = NULL;
        lst->tail = new_item;
    }

    if (prior_it != NULL)
    {
        new_item->prev = prior_it;
        prior_it->next = new_item;
    }
    else
    {
        new_item->prev = NULL;
        lst->head = new_item;
    }

    int_item_count_incr(lst, new_item);
    lst->num++; // This would get done in _fix below, but I like to update it asap.


    _int_list_fix(lst);
}

void  _int_list_fix(int_list_type *lst)
{
    int_item_type *it;
    int i;
    //float ratio;
    assert(lst != NULL);

    /*
    int j;
    int_item_type *i2;
    printf("List = [%d;%d|", lst->num, lst->total_count);
    i2 = lst->head; 
    while (i2 != NULL)
    {
        printf("(%f %f #%d) ", i2->start_value, i2->end_value, i2->count);
        i2 = i2->next;
    }
    printf("]\n");
    */

    it = lst->head;
    i = 0;
    // Move through an merge intersecting item.
    while (it != NULL)
    {
        if (it->next != NULL)
        {
            if (it->end_value >= it->next->start_value)
            {
                // We only merge if they have "similar" counts.
                // That is to say, we don't extend a tried and true
                // interval if we get one little superious reading on
                // the outside.
                
                double ratio;
                if (it->count < it->next->count)
                    ratio = ((float)it->count) / ((float)it->next->count + 1.0);
                else
                    ratio = ((float)it->next->count) / ((float)it->count + 1.0);

                if ((ratio > 0.25) && (ratio < 0.75))
                {
                    it->count += it->next->count;
                    lst->total_count += it->next->count;
                    it->end_value = MAX(it->end_value, it->next->end_value);
                    int_list_remove_item(lst, it->next);
                    continue;
                }
                /*
                }
                else
                {
                    // Now, ensure we have no overlap.
                    it->start_value = MIN(it->start_value, it->next->start_value);
                    it->next->end_value = MAX(it->end_value, it->next->end_value);

                    if (it->count > it->next->count)
                    {
                        it->end_value = it->end_value;
                        it->next->start_value = it->end_value + 0.01;
                    }
                    else
                    {
                        it->next->start_value = it->next->start_value;
                        it->end_value = it->next->start_value - 0.01;
                    }
                }
                */
            }
        }

        it = it->next;
        i++;
    }

    lst->num = i;

    /*
    printf("List'= [%d;%d|", lst->num, lst->total_count);
    i2 = lst->head; 
    while (i2 != NULL)
    {
        printf("(%f %f #%d) ", i2->start_value, i2->end_value, i2->count);
        i2 = i2->next;
    }
    printf("]\n");
    */

}

void int_item_print(int_item_type *item)
{
    assert(item != NULL);

    printf(" [item=0x%p][prev=0x%p, next=0x%p]\n", item, item->prev, item->next);
    printf(" %f   %f  (%d) (0x%p)\n", item->start_value, item->end_value, item->count, item->data);
}

void int_list_print(int_list_type *lst)
{
    int_item_type *it;
    assert(lst != NULL);

    printf("List of %d items (total count=%d)\n", lst->num, lst->total_count);
    it = lst->head; 
    while (it != NULL)
    {
        int_item_print(it);  
        it = it->next; 
    }

}

int_item_type *int_list_find_item(int_list_type *lst, double x)
{
    int_item_type *it;
    assert(lst != NULL);

    it = lst->head;
    while (it != NULL)
    {
        if ((it->start_value <= x) && (it->end_value >= x))
        {
            break;
        }
        it = it->next;
    }
    
    return it;
}

int int_item_get_count(int_item_type *item)
{
    assert(item != NULL);
    return item->count;
}


void int_item_count_incr(int_list_type *lst, int_item_type *item)
{
    int_item_count_add(lst, item, 1);
}

void int_item_count_add(int_list_type *lst, int_item_type *item, int num)
{
    assert(lst != NULL);
    assert(item != NULL);

    lst->total_count+= num;
    item->count+= num;
}


void int_list_cull_lowcount_item(int_list_type *lst, int cull_threshold)
{
    int_item_type *item, *nitem;
    assert(lst != NULL);

    item = lst->head;
    while (item != NULL)
    {
        if (item->count < cull_threshold)
        {
            nitem = item->next;
            int_list_remove_item(lst, item);
            item = nitem;
        }
        else
            item = item->next;
    }
}

int int_list_total_count(int_list_type *lst)
{
    assert(lst != NULL);

    return lst->total_count;
}


int_item_type *int_list_get_item_from_head(int_list_type *lst, int k)
{
    int_item_type *item;
    int i;
    assert(lst != NULL);

    item = lst->head;
    for (i = 0; i<k; i++)
    {
        if (item == NULL)
            return NULL;
        item = item->next;
    }
    return item;
}

int_item_type *int_list_get_item_from_tail(int_list_type *lst, int k)
{
    int_item_type *item;
    int i;
    assert(lst != NULL);

    item = lst->tail;
    for (i = 0; i<k; i++)
    {
        if (item == NULL)
            return NULL;
        item = item->prev;
    }
    return item;
}
