/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: log_configuration.h                      Date: 11 Jul. 2007    **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/


#ifndef _LOG_CONFIGURATION_H_
#define _LOG_CONFIGURATION_H_

#include <libxml/parser.h>
#include <libxml/xmlwriter.h>
#include "laser_read.h"
#include "interval.h"

typedef struct _log_cfg_type log_cfg_type; /**< Private structure that wraps hierarchical config context */
typedef struct _log_cfg_logfile_type log_cfg_logfile_type; 
typedef struct _log_cfg_logfile_period_type log_cfg_logfile_period_type; 
typedef struct _log_cfg_anno_type log_cfg_anno_type; 
typedef struct _log_cfg_flux_line_type log_cfg_flux_line_type;

struct _log_cfg_type 
{
    char *config_notes;

    log_cfg_logfile_type **logfiles;
    int num_logfiles;
    log_cfg_anno_type **anno;
    int num_annos;
    log_cfg_flux_line_type **flux_lines;
    int num_flux_lines;

    void *user_data;
};

struct _log_cfg_logfile_type 
{
    char *name;
    char *filename;
    char *notes;
    log_cfg_logfile_period_type **periods;
    int num_periods;

    lr_state_type *lr;

    void *user_data;
};

struct _log_cfg_anno_type 
{
    double time;
    char *text;
    int shortnote;

    void *user_data;
};

struct _log_cfg_flux_line_type 
{
    char *name;
    double x1; 
    double y1; 
    double x2; 
    double y2; 
    double length;

    void *user_data;
};

struct _log_cfg_logfile_period_type 
{
    double start_time;
    double end_time;
    int active;
    char *note;
    float position_x;
    float position_y;
    float position_t;

    int_list_type *background_lists[LASER_ARRAY_LEN];
    void *user_data;
};

log_cfg_type *log_cfg_create();
void log_cfg_destroy(log_cfg_type *lc);

/* Parsing / reading */
int log_cfg_parse_node(log_cfg_type *lc, xmlDocPtr doc, xmlNodePtr cur);
int log_cfg_check_valid(log_cfg_type *lc);

/* Writing */
void log_cfg_xml_write(log_cfg_type *lc, xmlTextWriterPtr writer);

/* Other functions */
void log_cfg_times_get(log_cfg_type *lc, double *earliest, double *latest);

int log_cfg_parse_fluxline(log_cfg_flux_line_type *l, xmlDocPtr doc, xmlNodePtr cur);
int log_cfg_parse_annotation(log_cfg_anno_type  *la, xmlDocPtr doc, xmlNodePtr cur);
int log_cfg_parse_logfile(log_cfg_logfile_type *lf, xmlDocPtr doc, xmlNodePtr cur);
int log_cfg_parse_period(log_cfg_logfile_period_type *p, xmlDocPtr doc, xmlNodePtr cur);

log_cfg_anno_type *log_cfg_add_annotation(log_cfg_type *lc);
void log_cfg_delete_annotation(log_cfg_type *lc, int n);

log_cfg_logfile_type *log_cfg_add_logfile(log_cfg_type *lc);
void log_cfg_delete_logfile(log_cfg_type *lc, int n);
void log_cfg_logfile_update_filename(log_cfg_type *lc, int n, char *filename);

void log_cfg_merge_period_with_next(log_cfg_type *lc, int log_num, int period_num);
void log_cfg_split_period(log_cfg_type *lc, int log_num, int period_num, double split_time);

void log_cfg_print(log_cfg_type *lc);

int log_cfg_find_period(log_cfg_type *lc, int log_num, double time);

void log_cfg_period_update_backgrounds(log_cfg_type *lc);

double log_cfg_all_period_starting_times(log_cfg_type *lc);

#endif
