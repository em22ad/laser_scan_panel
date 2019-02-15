/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: int_.h                               Date: 27 Jul. 2007    **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/


#ifndef _INTERVAL__H_
#define _INTERVAL__H_

typedef struct _int_list_type int_list_type;
typedef struct _int_item_type int_item_type;

struct _int_list_type 
{
    int_item_type *head, *tail;

    int num;
    int total_count;
};

struct _int_item_type 
{
    int_item_type *prev, *next;

    double start_value;
    double end_value;

    int count;
    void *data;
};

int_list_type *int_list_create();
void int_list_destroy(int_list_type *lst);
void int_list_empty(int_list_type *lst);
void int_list_remove_item(int_list_type *lst, int_item_type *item);
void int_list_add_item(int_list_type *lst, double st_value, double end_value);
void int_list_cull_lowcount_item(int_list_type *lst, int cull_threshold);
int_item_type *int_list_find_item(int_list_type *lst, double x);
int_item_type *int_list_get_item_from_head(int_list_type *lst, int k);
int_item_type *int_list_get_item_from_tail(int_list_type *lst, int k);

int int_list_total_count(int_list_type *lst);
int int_item_get_count(int_item_type *item);
void int_item_count_incr(int_list_type *lst, int_item_type *item);
void int_item_count_add(int_list_type *lst, int_item_type *item, int num);

void int_list_print(int_list_type *lst);
void int_item_print(int_item_type *item);

void int_check(int_list_type *lst);

#endif

