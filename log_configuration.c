/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: log_configuration.c                      Date: 11 Jul. 2007    **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#include "log_configuration.h"
#include "utils.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include "wnd_prog.h"


void _log_cfg_free_anno(log_cfg_anno_type *anno);
void _log_cfg_free_log_file(log_cfg_logfile_type *logfile);
void _log_cfg_free_elements(log_cfg_type *lc);
void _log_cfg_free_period(log_cfg_logfile_period_type *period);
void _log_cfg_update_period_bg(log_cfg_logfile_type *lf, log_cfg_logfile_period_type * period, lr_state_type * lr);
void _log_cfg_update_scan_data(lr_state_type * lr, float *scan_data, log_cfg_logfile_period_type *period);

/** Return the minimum of the two arguments */
#ifndef MIN 
#define MIN(x, y) (((x) > (y)) ? (y) : (x))
#endif
                                           
/** Return the maximum of the two arguments */
#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif
 
/** Return the sign of the argument */
#ifndef SGN
#define SGN(x) (((x) == 0) ? 0 : (((x) > (0)) ? (+1) : (-1)))
#endif
 


log_cfg_type *log_cfg_create()
{
    log_cfg_type *ret;

    ret = (log_cfg_type *)malloc(sizeof(log_cfg_type));

    ret->config_notes = NULL;
    ret->logfiles = NULL;
    ret->num_logfiles = 0;
    ret->anno = NULL;
    ret->num_annos = 0;
    ret->flux_lines = NULL;
    ret->num_flux_lines = 0;
    ret->user_data = NULL;


    return ret;
}

void log_cfg_destroy(log_cfg_type *lc)
{
    assert(lc != NULL);

    _log_cfg_free_elements(lc);

    free(lc);
}

void _log_cfg_free_elements(log_cfg_type *lc)
{
    assert(lc != NULL);

    if (lc->config_notes != NULL)
        free(lc->config_notes);
    lc->config_notes = NULL;

    if (lc->logfiles != NULL)
    {
        while (lc->num_logfiles > 0)
        {
            _log_cfg_free_log_file(lc->logfiles[lc->num_logfiles-1]);
            lc->num_logfiles--;
        }
        
        free(lc->logfiles);
        lc->logfiles = NULL;
    }

    if (lc->anno != NULL)
    {
        while (lc->num_annos > 0)
        {
            _log_cfg_free_anno(lc->anno[lc->num_annos-1]);
            lc->num_annos--;
        }
        
        free(lc->anno);
        lc->anno = NULL;
    }

}

void _log_cfg_free_log_file(log_cfg_logfile_type *logfile)
{
    assert(logfile != NULL);

    if (logfile->name != NULL)
        free(logfile->name);
    logfile->name = NULL;

    if (logfile->filename != NULL)
        free(logfile->filename);
    logfile->filename = NULL;

    if (logfile->notes != NULL)
        free(logfile->notes);
    logfile->notes = NULL;

    if (logfile->lr != NULL)
        lr_fclose(logfile->lr);
    logfile->lr = NULL;

    if (logfile->periods != NULL)
    {
        while (logfile->num_periods > 0)
        {
            _log_cfg_free_period(logfile->periods[logfile->num_periods-1]);
            logfile->num_periods--;
        }
        
        free(logfile->periods);
        logfile->periods = NULL;
    }

    free(logfile);
}

void _log_cfg_free_anno(log_cfg_anno_type *anno)
{
    assert(anno != NULL);

    if (anno->text != NULL)
        free(anno->text); 
    anno->text = NULL;

    free(anno);
}

void _log_cfg_free_period(log_cfg_logfile_period_type *period)
{
    int i;
    assert(period != NULL);

    for (i = 0; i < LASER_ARRAY_LEN; i++)
        int_list_destroy(period->background_lists[i]);

    period->note = NULL;

    free(period);
}

/*
 * @return 1 on success, 0 on failure
 */
int log_cfg_parse_node(log_cfg_type *lc, xmlDocPtr doc, xmlNodePtr cur)
{
    xmlNodePtr c;
    int num_lfs, num_as, num_fl;
    assert(lc != NULL);

    _log_cfg_free_elements(lc);

    if (xmlStrcmp(cur->name, (const xmlChar *)"logconfiguration") != 0)
        return 0;

    // Count the number logfiles
    num_lfs = 0;
    for (c = cur->xmlChildrenNode; c != NULL; c = c->next) 
    {
        if (xmlStrcmp(c->name, (const xmlChar *)"logfile") == 0) num_lfs++;
    }
    lc->logfiles = (log_cfg_logfile_type **)calloc(num_lfs, sizeof(log_cfg_logfile_type *));
    
    // Count the number annots
    num_as = 0;
    for (c = cur->xmlChildrenNode; c != NULL; c = c->next) 
    {
        if (xmlStrcmp(c->name, (const xmlChar *)"annotation") == 0) num_as++;
    }
    lc->anno = (log_cfg_anno_type **)calloc(num_as, sizeof(log_cfg_anno_type *));

    // Count the number flux_lines
    num_fl = 0;
    for (c = cur->xmlChildrenNode; c != NULL; c = c->next) 
    {
        if (xmlStrcmp(c->name, (const xmlChar *)"flux_line") == 0) num_fl++;
    }
    lc->flux_lines = (log_cfg_flux_line_type **)calloc(num_fl, sizeof(log_cfg_flux_line_type *));


    cur = cur->xmlChildrenNode;
    while (cur != NULL) 
    {
        if (xmlStrcmp(cur->name, (const xmlChar *)"config") == 0)
        {
            lc->config_notes = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
        }

        if (xmlStrcmp(cur->name, (const xmlChar *)"logfile") == 0)
        {
            lc->logfiles[lc->num_logfiles] = (log_cfg_logfile_type *)malloc(sizeof(log_cfg_logfile_type));
            if (!log_cfg_parse_logfile(lc->logfiles[lc->num_logfiles++], doc, cur->xmlChildrenNode)) 
                return 0;
         }

        if (xmlStrcmp(cur->name, (const xmlChar *)"annotation") == 0)
        {
            lc->anno[lc->num_annos] = (log_cfg_anno_type *)malloc(sizeof(log_cfg_anno_type));
            if (!log_cfg_parse_annotation(lc->anno[lc->num_annos++], doc, cur->xmlChildrenNode)) 
                return 0;
        }

        if (xmlStrcmp(cur->name, (const xmlChar *)"flux_line") == 0)
        {
            lc->flux_lines[lc->num_flux_lines] = (log_cfg_flux_line_type *)malloc(sizeof(log_cfg_flux_line_type));
            if (!log_cfg_parse_fluxline(lc->flux_lines[lc->num_flux_lines++], doc, cur)) 
                return 0;
        }

        cur = cur->next;
    }

    if (!log_cfg_check_valid(lc))
        return 0;

    return 1;
}

/* Check that the data structures actually have the right semantics. 
 * Returns zero on a failure.
 */
int log_cfg_check_valid(log_cfg_type *lc)
{
    int i;
    //TODO check for validity.
    // For example: All periods should be in a sorted order and non-overlapping.
    
    // We allocate the lr for each of the log files, if these aren't valid log files, when we'll
    // hear about it here.
    for (i = 0; i < lc->num_logfiles; i++)
    {
        if (lc->logfiles[i]->filename != NULL)
        {
            lc->logfiles[i]->lr = lr_fopen(lc->logfiles[i]->filename);

            if (lc->logfiles[i]->lr == NULL)
                return 0;
        }
        else 
            return 0;
    }
        
    return 1;
}

int log_cfg_parse_logfile(log_cfg_logfile_type *lf, xmlDocPtr doc, xmlNodePtr cur)
{
    xmlNodePtr c;
    int num_p;
 
    assert(lf != NULL);

    lf->name = (char *)xmlGetProp(cur->parent, "name");
    lf->filename = (char *)xmlGetProp(cur->parent, "file");
    lf->notes = NULL;
    lf->num_periods = 0;
    lf->periods = NULL;
    lf->user_data = NULL;
    lf->lr = NULL;

    // Count the number periods
    num_p = 0;
    for (c = cur; c != NULL; c = c->next) 
    {
        if (xmlStrcmp(c->name, (const xmlChar *)"period") == 0) num_p++;
    }
    lf->periods = (log_cfg_logfile_period_type **)calloc(num_p, sizeof(log_cfg_logfile_period_type *));

    while (cur != NULL) 
    {
        if (xmlStrcmp(cur->name, (const xmlChar *)"notes") == 0)
            lf->notes = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

        if (xmlStrcmp(cur->name, (const xmlChar *)"period") == 0)
        {
            lf->periods[lf->num_periods] = (log_cfg_logfile_period_type *)malloc(sizeof(log_cfg_logfile_period_type));
            if (!log_cfg_parse_period(lf->periods[lf->num_periods++], doc, cur->xmlChildrenNode)) 
                return 0;
        }

        cur = cur->next;
    }

    return 1;

}

int log_cfg_parse_fluxline(log_cfg_flux_line_type *l, xmlDocPtr doc, xmlNodePtr cur)
{
    assert(l != NULL);

    l->name = (char *)xmlGetProp(cur, "name");
    l->x1 = atof(xmlGetProp(cur, "x1"));
    l->y1 = atof(xmlGetProp(cur, "y1"));
    l->x2 = atof(xmlGetProp(cur, "x2"));
    l->y2 = atof(xmlGetProp(cur, "y2"));
    l->length = hypot( l->x1 - l->x2, l->y1 - l->y2);
    l->user_data = NULL;

    return 1;
}

int log_cfg_parse_annotation(log_cfg_anno_type  *la, xmlDocPtr doc, xmlNodePtr cur)
{
    assert(la != NULL);
    char *s;

    la->time = atof(xmlGetProp(cur->parent, "time"));
    s = xmlGetProp(cur->parent, "short");
    la->shortnote =  ((strcasecmp(s,"true") == 0) || (strcasecmp(s,"yes") == 0));
    la->text = NULL;
    la->user_data = NULL;

    while (cur != NULL) 
    {
        la->text = (char *)xmlNodeListGetString(doc, cur, 1);

        cur = cur->next;
    }

    return 1;
}

int log_cfg_parse_period(log_cfg_logfile_period_type *p, xmlDocPtr doc, xmlNodePtr cur)
{
    char *s;
    int i;
    assert(p != NULL);
    
    p->start_time = atof(xmlGetProp(cur->parent, "start_time"));
    p->end_time = atof(xmlGetProp(cur->parent, "end_time"));
    s = xmlGetProp(cur->parent, "active");
    p->active = ((strcasecmp(s,"true") == 0) || (strcasecmp(s,"yes") == 0));
    p->note = NULL;
    p->position_x = 0.0;
    p->position_y = 0.0;
    p->position_t = 0.0;
    p->user_data = NULL;
    for (i = 0; i < LASER_ARRAY_LEN; i++)
        p->background_lists[i] = int_list_create();


    while (cur != NULL) 
    {
        if (xmlStrcmp(cur->name, (const xmlChar *)"note") == 0)
            p->note = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

        if (xmlStrcmp(cur->name, (const xmlChar *)"location") == 0)
        {
           p->position_x = atof(xmlGetProp(cur, "x"));
           p->position_y = atof(xmlGetProp(cur, "y"));
           p->position_t = atof(xmlGetProp(cur, "t"));
        }

        cur = cur->next;
    }

    return 1;
}


/*
 * Write the output to file 
 */
void log_cfg_xml_write(log_cfg_type *lc, xmlTextWriterPtr writer)
{
    int i, j;
    char s[200];
    assert(lc != NULL);
    assert(writer != NULL);

    xmlTextWriterStartElement(writer, "logconfiguration");

    xmlTextWriterStartElement(writer, "config");
    xmlTextWriterWriteFormatString(writer, lc->config_notes);
    xmlTextWriterEndElement(writer);

    for (i = 0; i < lc->num_logfiles; i++)
    {
        xmlTextWriterStartElement(writer, "logfile");
        xmlTextWriterWriteAttribute(writer, "name", lc->logfiles[i]->name);
        xmlTextWriterWriteAttribute(writer, "file", lc->logfiles[i]->filename);

        xmlTextWriterStartElement(writer, "notes");
        xmlTextWriterWriteFormatString(writer, lc->logfiles[i]->notes);
        xmlTextWriterEndElement(writer);

        for (j = 0; j < lc->logfiles[i]->num_periods; j++)
        {
            xmlTextWriterStartElement(writer, "period");

            sprintf(s,"%6.6f", lc->logfiles[i]->periods[j]->start_time);
            xmlTextWriterWriteAttribute(writer, "start_time", s);
            sprintf(s,"%6.6f", lc->logfiles[i]->periods[j]->end_time);
            xmlTextWriterWriteAttribute(writer, "end_time", s);
            xmlTextWriterWriteAttribute(writer, "active", (lc->logfiles[i]->periods[j]->active?"true":"false"));

            xmlTextWriterStartElement(writer, "note");
            xmlTextWriterWriteFormatString(writer, lc->logfiles[i]->periods[j]->note);
            xmlTextWriterEndElement(writer);

            xmlTextWriterStartElement(writer, "location");
            sprintf(s,"%6.6f", lc->logfiles[i]->periods[j]->position_x);
            xmlTextWriterWriteAttribute(writer, "x", s);
            sprintf(s,"%6.6f", lc->logfiles[i]->periods[j]->position_y);
            xmlTextWriterWriteAttribute(writer, "y", s);
            sprintf(s,"%6.6f", lc->logfiles[i]->periods[j]->position_t);
            xmlTextWriterWriteAttribute(writer, "t", s);
            xmlTextWriterEndElement(writer);

            xmlTextWriterEndElement(writer);
        }


        xmlTextWriterEndElement(writer);
    }

    for (i = 0; i < lc->num_annos; i++)
    {

        xmlTextWriterStartElement(writer, "annotation");
        sprintf(s,"%6.6f", lc->anno[i]->time);
        xmlTextWriterWriteAttribute(writer, "time", s);

        xmlTextWriterWriteAttribute(writer, "short", (lc->anno[i]->shortnote?"true":"false"));

        xmlTextWriterWriteFormatString(writer, lc->anno[i]->text);
        xmlTextWriterEndElement(writer);

    }

    for (i = 0; i < lc->num_flux_lines; i++)
    {
        xmlTextWriterStartElement(writer, "flux_line");
        xmlTextWriterWriteAttribute(writer, "name", lc->flux_lines[i]->name);

        sprintf(s,"%6.6f", lc->flux_lines[i]->x1);
        xmlTextWriterWriteAttribute(writer, "x1", s);
        sprintf(s,"%6.6f", lc->flux_lines[i]->y1);
        xmlTextWriterWriteAttribute(writer, "y1", s);
        sprintf(s,"%6.6f", lc->flux_lines[i]->x2);
        xmlTextWriterWriteAttribute(writer, "x2", s);
        sprintf(s,"%6.6f", lc->flux_lines[i]->y2);
        xmlTextWriterWriteAttribute(writer, "y2", s);

        xmlTextWriterEndElement(writer);
    }

    xmlTextWriterEndElement(writer);
}


void log_cfg_print(log_cfg_type *lc)
{
    int i, j;
    assert(lc != NULL);

    printf("num_logfiles = %d\n", lc->num_logfiles);
    for (i = 0; i < lc->num_logfiles; i++)
    {
        printf("logfile[%d] = 0x%p\n", i, lc->logfiles[i]);
        printf("logfile[%d]->name = '%s'\n", i, lc->logfiles[i]->name);
        printf("logfile[%d]->filename = '%s'\n", i, lc->logfiles[i]->filename);
        printf("logfile[%d]->notes = '%s'\n", i, lc->logfiles[i]->notes);
        printf("logfile[%d]->num_periods = %d\n", i, lc->logfiles[i]->num_periods);

        for (j = 0; j < lc->logfiles[i]->num_periods; j++)
        {
            printf("\tlogfile[%d]->periods[%d] = 0x%p\n", i, j, lc->logfiles[i]->periods[j]);
            printf("\tlogfile[%d]->periods[%d] = <%f, %f, %d, %s, (%f %f %f) 0x%p>\n", i, j, 
                lc->logfiles[i]->periods[j]->start_time,
                lc->logfiles[i]->periods[j]->end_time,
                lc->logfiles[i]->periods[j]->active,
                lc->logfiles[i]->periods[j]->note,
                lc->logfiles[i]->periods[j]->position_x,
                lc->logfiles[i]->periods[j]->position_y,
                lc->logfiles[i]->periods[j]->position_t,
                lc->logfiles[i]->periods[j]->user_data);

        }

    }

}

void log_cfg_times_get(log_cfg_type *lc, double *earliest, double *latest)
{
    double first, last;
    int i, j;

    assert(lc != NULL);

    first = -2.0;
    last = -1.0;

    for (i = 0; i < lc->num_logfiles; i++)
    {
        for (j = 0; j < lc->logfiles[i]->num_periods; j++)
        {
            if (first < 0)
                first = lc->logfiles[i]->periods[j]->start_time;
            else
                first = MIN(first, lc->logfiles[i]->periods[j]->start_time);

            if (last < 0)
                last = lc->logfiles[i]->periods[j]->end_time;
            else
                last = MAX(last, lc->logfiles[i]->periods[j]->end_time);
            
        }
    }

    for (i = 0; i < lc->num_annos; i++)
    {
        if (first < 0)
            first = lc->anno[i]->time;
        else
            first = MIN(first, lc->anno[i]->time);

        if (last < 0)
            last = lc->anno[i]->time;
        else
            last = MAX(last, lc->anno[i]->time);
    }

    if (earliest != NULL)
        (*earliest) = first;
    if (latest != NULL)
        (*latest) = last;
}


void log_cfg_delete_annotation(log_cfg_type *lc, int n)
{

    log_cfg_anno_type **newlist;
    int i, j;

    assert(lc != NULL);

    assert(lc->num_annos > 0);

    if (lc->num_annos == 1)
    {
        assert(n == 0);

        _log_cfg_free_anno(lc->anno[0]);

        free(lc->anno);
        lc->anno = NULL;

        lc->num_annos = 0;
    }
    else
    {
        newlist = (log_cfg_anno_type **)calloc(lc->num_annos-1, sizeof(log_cfg_anno_type *));

        j = 0;
        for (i = 0; i < lc->num_annos-1; i++)
        {
            if (i == n)
            {
                j = 1;
                _log_cfg_free_anno(lc->anno[n]);
            }

            newlist[i] = lc->anno[i + j];
        }


        lc->num_annos--;
        free(lc->anno);
        lc->anno = newlist;
    }

}


log_cfg_anno_type *log_cfg_add_annotation(log_cfg_type *lc)
{
    log_cfg_anno_type  *newanno;
    log_cfg_anno_type **newlist;
    int i;

    assert(lc != NULL);

    newanno = (log_cfg_anno_type *)malloc(sizeof(log_cfg_anno_type));
    newanno->time = 0.0;
    newanno->text = NULL;
    newanno->user_data = NULL;
    newanno->shortnote = 0;

    newlist = (log_cfg_anno_type **)calloc(lc->num_annos+1, sizeof(log_cfg_anno_type *));
    for (i = 0; i < lc->num_annos; i++)
        newlist[i] = lc->anno[i];

    newlist[lc->num_annos] = newanno;
    lc->num_annos++;
    free(lc->anno);
    lc->anno = newlist;

    return newanno;
}

void log_cfg_delete_logfile(log_cfg_type *lc, int n)
{
    log_cfg_logfile_type **newlist;
    int i, j;

    assert(lc != NULL);
    assert(n < lc->num_logfiles);

    /*printf("<%d>\n", n);
    printf("[ ");
    for (i = 0; i < lc->num_logfiles; i++)
        printf("0x%p ", lc->logfiles[i]);
    printf("]\n");*/

    if (lc->num_logfiles == 1)
    {
        assert(n == 0);

        _log_cfg_free_log_file(lc->logfiles[0]);

        free(lc->logfiles);
        lc->logfiles = NULL;

        lc->num_logfiles = 0;
    }
    else
    {
        newlist = (log_cfg_logfile_type **)calloc(lc->num_logfiles-1, sizeof(log_cfg_logfile_type *));

        j = 0;
        for (i = 0; i < lc->num_logfiles-1; i++)
        {
            if (i == n)
            {
                j = 1;
                _log_cfg_free_log_file(lc->logfiles[n]);
            }

            newlist[i] = lc->logfiles[i + j];
        }


        lc->num_logfiles--;
        free(lc->logfiles);
        lc->logfiles  = newlist;


    }

    /*printf("[ ");
    for (i = 0; i < lc->num_logfiles; i++)
        printf("0x%p ", lc->logfiles[i]);
    printf("]\n");*/
}

void log_cfg_logfile_update_filename(log_cfg_type *lc, int n, char *filename)
{
    static char empty_str[1] = "";
    char *a, *b;
    int i;
    log_cfg_logfile_type *logfile;

    assert(lc != NULL);
    assert(n < lc->num_logfiles);
    logfile = lc->logfiles[n];

    // Now check, does the filename change?
    // Make it safe for NULL values;
    a = ((filename == NULL)?empty_str:filename);
    b = ((logfile->filename == NULL)?empty_str:logfile->filename);
    
    if (strcmp(a, b) != 0)
    {
        // There is a change in the name

        // Delete all existing periods
        if (logfile->periods != NULL)
        {
            while (logfile->num_periods > 0)
            {
                _log_cfg_free_period(logfile->periods[logfile->num_periods-1]);
                logfile->num_periods--;
            }

            free(logfile->periods);
            logfile->periods = NULL;
        }

        if (logfile->filename != NULL) free(logfile->filename);
        logfile->filename = strdup(filename);

        // Open the logfile        
        logfile->lr = lr_fopen(logfile->filename);

        if (logfile->lr == NULL)
            return;

        // We allocate a single period
        logfile->periods = (log_cfg_logfile_period_type **)calloc(1, sizeof(log_cfg_logfile_period_type *));
        logfile->num_periods = 1;
        logfile->periods[0] = (log_cfg_logfile_period_type *)calloc(1, sizeof(log_cfg_logfile_period_type));
        logfile->periods[0]->start_time = lr_get_first_time(logfile->lr);
        logfile->periods[0]->end_time = lr_get_last_time(logfile->lr);
        logfile->periods[0]->active = 0;
        logfile->periods[0]->note = NULL;
        logfile->periods[0]->position_x = 0.0;
        logfile->periods[0]->position_y = 0.0;
        logfile->periods[0]->position_t = 0.0;
        logfile->periods[0]->user_data = NULL;
        for (i = 0; i < LASER_ARRAY_LEN; i++)
            logfile->periods[0]->background_lists[i] = int_list_create();

        printf("'%s' ranges from %6.2fs to %6.2fs\n",
            logfile->filename,
            logfile->periods[0]->start_time,
            logfile->periods[0]->end_time);
    }
}

log_cfg_logfile_type *log_cfg_add_logfile(log_cfg_type *lc)
{
    log_cfg_logfile_type *newlog;
    log_cfg_logfile_type **newlist;
    int i;

    assert(lc != NULL);

    newlog = (log_cfg_logfile_type *)malloc(sizeof(log_cfg_logfile_type));
    newlog->name = NULL;
    newlog->filename = NULL;
    newlog->notes = NULL;
    newlog->num_periods = 0;
    newlog->periods = NULL;
    newlog->user_data = NULL;
    newlog->lr = NULL;

    newlist = (log_cfg_logfile_type **)calloc(lc->num_logfiles+1, sizeof(log_cfg_anno_type *));
    for (i = 0; i < lc->num_logfiles; i++)
        newlist[i] = lc->logfiles[i];

    newlist[lc->num_logfiles] = newlog;
    lc->num_logfiles++;
    free(lc->logfiles);
    lc->logfiles = newlist;

    return newlog;
}

void log_cfg_merge_period_with_next(log_cfg_type *lc, int log_num, int period_num)
{
    log_cfg_logfile_period_type **newlist;
    int i, j;
    char *str;

    assert(lc != NULL);

    assert(period_num >= 0);
    assert(log_num < lc->num_logfiles);
    assert(period_num < lc->logfiles[log_num]->num_periods-1);

    newlist = (log_cfg_logfile_period_type **)calloc(lc->logfiles[log_num]->num_periods-1, sizeof(log_cfg_logfile_period_type *));

    j = 0;
    for (i = 0; i < lc->logfiles[log_num]->num_periods-1; i++)
    {
        if (i == period_num)
        {
            j = 1;
            //So we're going to merge [i] and [i+1]
            
            lc->logfiles[log_num]->periods[i+1]->start_time = 
                    MIN(lc->logfiles[log_num]->periods[i]->start_time, lc->logfiles[log_num]->periods[i+1]->start_time);
            lc->logfiles[log_num]->periods[i+1]->end_time = 
                    MAX(lc->logfiles[log_num]->periods[i]->end_time, lc->logfiles[log_num]->periods[i+1]->end_time);

            str = (char *)malloc(sizeof(char) * (strlen(lc->logfiles[log_num]->periods[i]->note) + strlen(lc->logfiles[log_num]->periods[i+1]->note) + 3));
            sprintf(str,"%s\n%s", lc->logfiles[log_num]->periods[i]->note, lc->logfiles[log_num]->periods[i+1]->note);
            free(lc->logfiles[log_num]->periods[i+1]->note);
            lc->logfiles[log_num]->periods[i+1]->note = str;

            _log_cfg_free_period(lc->logfiles[log_num]->periods[i]);

            // Now let's rebuild the period's background
            _log_cfg_update_period_bg(lc->logfiles[log_num], lc->logfiles[log_num]->periods[i+1], lc->logfiles[log_num]->lr);
        }

        newlist[i] = lc->logfiles[log_num]->periods[i + j];
    }

    lc->logfiles[log_num]->num_periods--;
    free(lc->logfiles[log_num]->periods);
    lc->logfiles[log_num]->periods = newlist;

    log_cfg_period_update_backgrounds(lc);
}

void log_cfg_split_period(log_cfg_type *lc, int log_num, int period_num, double split_time)
{
    log_cfg_logfile_period_type **newlist;
    log_cfg_logfile_period_type *newperiod;
    int i, j;

    assert(lc != NULL);
    
    assert(log_num >= 0);
    assert(period_num >= 0);
    assert(log_num < lc->num_logfiles);
    assert(period_num < lc->logfiles[log_num]->num_periods);

    assert( lc->logfiles[log_num]->periods[period_num]->start_time < split_time );
    assert( lc->logfiles[log_num]->periods[period_num]->end_time > split_time );

    //log_cfg_print(lc);

    newlist = (log_cfg_logfile_period_type **)calloc(lc->logfiles[log_num]->num_periods+1, sizeof(log_cfg_logfile_period_type *));
    newperiod = (log_cfg_logfile_period_type *)calloc(1, sizeof(log_cfg_logfile_period_type));

    newperiod->start_time = split_time;
    newperiod->end_time = lc->logfiles[log_num]->periods[period_num]->end_time;
    newperiod->active = (lc->logfiles[log_num]->periods[period_num]->active);
    newperiod->position_x = lc->logfiles[log_num]->periods[period_num]->position_x;
    newperiod->position_y = lc->logfiles[log_num]->periods[period_num]->position_y;
    newperiod->position_t = lc->logfiles[log_num]->periods[period_num]->position_t;
    newperiod->user_data = NULL;
    for (i = 0; i < LASER_ARRAY_LEN; i++)
        newperiod->background_lists[i] = int_list_create();

    if (lc->logfiles[log_num]->periods[period_num]->note != NULL)
    {
        newperiod->note = (char *)calloc(strlen(lc->logfiles[log_num]->periods[period_num]->note) + 10,sizeof(char));
        sprintf(newperiod->note,"(DUP) %s", lc->logfiles[log_num]->periods[period_num]->note);
    }
    else 
    {
        newperiod->note = strdup("(DUP)");
    }

    lc->logfiles[log_num]->periods[period_num]->end_time = split_time;

    _log_cfg_update_period_bg(lc->logfiles[log_num], lc->logfiles[log_num]->periods[period_num], lc->logfiles[log_num]->lr);
    _log_cfg_update_period_bg(lc->logfiles[log_num], newperiod, lc->logfiles[log_num]->lr);

    j = 0;
    for (i = 0; i < lc->logfiles[log_num]->num_periods; i++)
    {
        newlist[i + j] = lc->logfiles[log_num]->periods[i];

        if (i == period_num)
        {
            j = 1; 

            newlist[i + j] = newperiod;
        }


    }

    lc->logfiles[log_num]->num_periods++;
    free(lc->logfiles[log_num]->periods);
    lc->logfiles[log_num]->periods = newlist;

    log_cfg_period_update_backgrounds(lc);
}

/* Return the index of the log's period that contains time.
 * returns -1 if none is found */
int log_cfg_find_period(log_cfg_type *lc, int log_num, double time)
{
    int i;
    assert(lc != NULL);
    assert(log_num < lc->num_logfiles);

    for (i = 0; i < lc->logfiles[log_num]->num_periods; i++)
    {
        if ((lc->logfiles[log_num]->periods[i]->active) && 
            (lc->logfiles[log_num]->periods[i]->start_time <= time) && 
            (lc->logfiles[log_num]->periods[i]->end_time > time))
            return i;
    }

    return -1;
}

void log_cfg_period_update_backgrounds(log_cfg_type *lc)
{
    int i, j;
    assert(lc != NULL);

    for (i = 0; i < lc->num_logfiles; i++)
    {
        for (j = 0; j < lc->logfiles[i]->num_periods; j++)
        {
            _log_cfg_update_period_bg(lc->logfiles[i], lc->logfiles[i]->periods[j], lc->logfiles[i]->lr);
        }
    }
}

char *_log_cfg_get_cached_name(log_cfg_logfile_type *lf, log_cfg_logfile_period_type * period)
{
    static char file_name[250];
    char *p;

    p = strrchr(lf->filename,'/');
    if (p == NULL)
        p = &(lf->filename[0]);
    else
        p++;

    sprintf(file_name,".bg_cache_%s.%6.2f%6.2f", p, period->start_time, period->end_time);

    return file_name;
}

int _log_cfg_update_period_bg_read_cached(log_cfg_logfile_type *lf, log_cfg_logfile_period_type * period)
{ // return 1 on success, no need to re-calculate
    FILE *in;
    int i;
    double a, b;
    int c, r;
    char ch, ch2;
    int_item_type *item;

    // Does this file exist?
    if (file_exist(_log_cfg_get_cached_name(lf, period)))
    { // Read out the cached bg.
        in = fopen(_log_cfg_get_cached_name(lf, period), "r");

        i = 0;
        int_list_empty(period->background_lists[i]);

        while (!feof(in))
        {
            r = fscanf(in, "\[ %lf %lf %d]\t", &a, &b, &c);

            if (r == 0)
            {
                fscanf(in, "%c%c", &ch, &ch2); // Get X and newline.

                i++;
                if (i == LASER_ARRAY_LEN)
                    break;
                int_list_empty(period->background_lists[i]);
            }
            else
            {
                item = (int_item_type *)malloc(sizeof(int_item_type));
                item->start_value = a;
                item->end_value = b;
                item->data = NULL;
                item->count = c;
                item->prev = period->background_lists[i]->tail;
                item->next = NULL;

                if (period->background_lists[i]->tail != NULL)
                {
                    period->background_lists[i]->tail->next = item;
                    period->background_lists[i]->tail = item;
                }
                else
                {
                    period->background_lists[i]->head = item;
                    period->background_lists[i]->tail = item;
                }

                period->background_lists[i]->num++;
                period->background_lists[i]->total_count += c;


            }
        }

        fclose(in);
        return 1; // success, no need to do anything else.
    }

    return 0;
}

void _log_cfg_update_period_bg_write_cached(log_cfg_logfile_type *lf, log_cfg_logfile_period_type * period)
{
    int i;
    FILE *out;
    int_item_type *item;

    out = fopen(_log_cfg_get_cached_name(lf, period), "w");

    for (i = 0; i < LASER_ARRAY_LEN; i++)
    {
        item = period->background_lists[i]->head;
        while (item != NULL)
        {
            fprintf(out, "[%6lf %6lf %d]\t", item->start_value, item->end_value, item->count);
            item = item->next;

        }
        fprintf(out,"X\n");
    }


    fclose(out);
}

void _log_cfg_update_period_bg(log_cfg_logfile_type *lf, log_cfg_logfile_period_type * period, lr_state_type * lr)
{
    static int _sample_randomly = 1;
    static int _sample_num_samples = 1500;
#define _SAMPLE_BLOCK_SIZE 250
    int i, c, j;
    lr_position_state_type lr_state_save;
    float *new_data;
    GtkWindow *dlg;

    assert(period != NULL);

    if (_log_cfg_update_period_bg_read_cached(lf, period))
    {
        return;
    }

    for (i = 0; i < LASER_ARRAY_LEN; i++)
        int_list_empty(period->background_lists[i]);

    dlg = wnd_progress_dialog_create("Background Learning", "Processing period to extract background");

    //Save lr state
    lr_state_save = lr_save_position_state(lr);

    if (_sample_randomly)
    {
        for (j = 0; j < _sample_num_samples; j++)
        {
            double d = period->start_time + ((float)rand() / (float)RAND_MAX)*(period->end_time - period->start_time);

            lr_read_to_time(lr, d);
            for (c = 0; c < _SAMPLE_BLOCK_SIZE; c++)
            {
                new_data = lr_data_points_get(lr);
                _log_cfg_update_scan_data(lr, new_data, period);
                lr_step(lr);
                if ((lr_time_stamp_get(lr)+0.01 >= period->end_time) || (lr_time_stamp_get(lr)-0.01 <= period->start_time))
                    break;
            }

            wnd_progress_dialog_set_progress(dlg, (((float)j+2.0))/((float)_sample_num_samples));

        }

    }
    else
    {
        c = 0;
        lr_read_to_time(lr, period->start_time);
        while (1)
        {
            new_data = lr_data_points_get(lr);

            //if (c > 100000) 
            _log_cfg_update_scan_data(lr, new_data, period);

            lr_step(lr);
            if ((lr_time_stamp_get(lr)+0.01 >= period->end_time)
                    || (lr_time_stamp_get(lr)-0.01 <= period->start_time))
                break;


            if ((c % 500) == 0)
            {
                wnd_progress_dialog_set_progress(dlg, (lr_time_stamp_get(lr) - period->start_time)/(period->end_time - period->start_time));
                //if (c >= 200000) break;
            }
            c++;

        }

    }

    wnd_progress_dialog_destroy(dlg);

    // Okay, now we cull the bits from the background lists 
    for (i = 0; i < LASER_ARRAY_LEN; i++)
    {
        int thres = (int)rint(0.09*(period->background_lists[i]->total_count));
        int_list_cull_lowcount_item(period->background_lists[i], thres);
    }


    // Since we know that the cached version wasn't read, try write it.
    _log_cfg_update_period_bg_write_cached(lf, period);


    //Repair lr state
    lr_load_position_state(lr, lr_state_save);

}


void _log_cfg_update_scan_data(lr_state_type * lr, float *scan_data, log_cfg_logfile_period_type *period)
{
#define INTERVAL_SIZE 0.05
    int i;


    for (i = 0; i < LASER_ARRAY_LEN; i++)
    {
        int_list_add_item(period->background_lists[i], scan_data[i] - INTERVAL_SIZE, scan_data[i] + INTERVAL_SIZE);           
        if (period->background_lists[i]->num > 30)
        { 
            int thres;
            thres = (int)rint(0.01*(period->background_lists[i]->total_count));
            int_list_cull_lowcount_item(period->background_lists[i], thres);

        }
    }

}

int _log_cfg_is_time_valid_for_all_intervals(log_cfg_type *lc, double t)
{
    int i;

    for (i = 0; i < lc->num_logfiles; i++)
    {
        if (log_cfg_find_period(lc, i, t) == -1)
            return 0;
    }

    return 1;
}

/* Return the index of the log's period that contains time.
 * returns -1 if none is found */
int _log_cfg_find_period_wo_regard_to_activeness(log_cfg_type *lc, int log_num, double time)
{
    int i;
    assert(lc != NULL);
    assert(log_num < lc->num_logfiles);

    for (i = 0; i < lc->logfiles[log_num]->num_periods; i++)
    {
        if ((lc->logfiles[log_num]->periods[i]->start_time <= time) && 
            (lc->logfiles[log_num]->periods[i]->end_time > time))
            return i;
    }

    return -1;
}

// find a time that is the smallest that is within the periods of all the 
// lasers and greater than the given time. If none can be found, time is returned.
double _log_cfg_get_next_time(log_cfg_type *lc, double time)
{
    int i, p;

#define MAX_TIME 1893456000.0
    double sm_incr = MAX_TIME; // large time in 2030.
    double sm_value = time;

    for( i = 0; i < lc->num_logfiles; i++)
    {
        p = _log_cfg_find_period_wo_regard_to_activeness(lc, i, time);
        p++;
        if (p < lc->logfiles[i]->num_periods) // beginning of next period
        {
            if (sm_incr > lc->logfiles[i]->periods[p]->start_time - time)
            {
                sm_incr = (lc->logfiles[i]->periods[p]->start_time - time);
                sm_value = lc->logfiles[i]->periods[p]->start_time;

            }
        }

    }

    return sm_value;
}


// find the earliest time in which all lasers have active segments.
double log_cfg_all_period_starting_times(log_cfg_type *lc)
{
    double s_time = 0.0;
    double n_time;


    while (1)
    {
        if (_log_cfg_is_time_valid_for_all_intervals(lc, s_time))
            return s_time;

        n_time = _log_cfg_get_next_time(lc, s_time);

        if (n_time == s_time)
            return -1.0;

        s_time = n_time;

    }


}


