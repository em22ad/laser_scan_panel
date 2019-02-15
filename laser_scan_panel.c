/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: laser_scan_panel.c                    Date: 11 Jul. 2007       **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <libxml/parser.h>
#include <libxml/xmlwriter.h>
#include "log_configuration.h"
#include "main_wnd.h"
#include "utils.h"

#define XML_ENCODING "ISO-8859-1"


static int quit = 0;
static char *filename = NULL;
static FILE *outputfile = NULL;
static FILE *fluxoutputfile = NULL;
static int auto_proc = 0;
static int communicate_with_matlab = 0;

void sig_quit(int signum)
{
    quit = 1;
}


void parse_args(int argc, char **argv)
{
    //We pull off the -m flag off the back of the stack.
    if (argc > 1)
    {
        if (strcmp(argv[argc-1], "-m") == 0)
        {
            communicate_with_matlab = 1;
            argc--; 
        }
    }


    if (argc == 2)
    {
        filename = strdup(argv[1]);
        return;
    }
    else if (argc == 4)
    {
        if (strcmp(argv[2], "-o") == 0)
        {
            filename = strdup(argv[1]);
            outputfile = fopen(strdup(argv[3]), "w");
            return;
        }
        else
        if (strcmp(argv[2], "-f") == 0)
        {
            filename = strdup(argv[1]);
            fluxoutputfile = fopen(strdup(argv[3]), "w");
            return;
        }

    }
    else if (argc == 5)
    {
        if ((strcmp(argv[2], "-o") == 0) && (strcmp(argv[4], "-a") == 0))
        {
            filename = strdup(argv[1]);
            outputfile = fopen(strdup(argv[3]), "w");
            auto_proc = 1;
            return;
        }
        else if ((strcmp(argv[2], "-a") == 0) && (strcmp(argv[3], "-o") == 0))
        {
            filename = strdup(argv[1]);
            outputfile = fopen(strdup(argv[4]), "w");
            auto_proc = 1;
            return;
        }
        else if ((strcmp(argv[2], "-f") == 0) && (strcmp(argv[4], "-a") == 0))
        {
            filename = strdup(argv[1]);
            fluxoutputfile = fopen(strdup(argv[3]), "w");
            auto_proc = 1;
            return;
        }
        else if ((strcmp(argv[2], "-a") == 0) && (strcmp(argv[3], "-f") == 0))
        {
            filename = strdup(argv[1]);
            fluxoutputfile = fopen(strdup(argv[4]), "w");
            auto_proc = 1;
            return;
        }
    }
    else if (argc == 6)
    {
        if ((strcmp(argv[2], "-o") == 0) && (strcmp(argv[4], "-f") == 0))
        {
            filename = strdup(argv[1]);
            outputfile = fopen(strdup(argv[3]), "w");
            fluxoutputfile = fopen(strdup(argv[5]), "w");
            return;
        }
        else if ((strcmp(argv[2], "-f") == 0) && (strcmp(argv[4], "-o") == 0))
        {
            filename = strdup(argv[1]);
            outputfile = fopen(strdup(argv[5]), "w");
            fluxoutputfile = fopen(strdup(argv[3]), "w");
            return;
        }
    }
    else if (argc == 7)
    {
        if (strcmp(argv[2], "-a") == 0) 
        {
            auto_proc = 1;

            if ((strcmp(argv[3], "-o") == 0) && (strcmp(argv[5], "-f") == 0))
            {
                filename = strdup(argv[1]);
                outputfile = fopen(strdup(argv[4]), "w");
                fluxoutputfile = fopen(strdup(argv[6]), "w");
                return;
            }
            else if ((strcmp(argv[3], "-f") == 0) && (strcmp(argv[5], "-o") == 0))
            {
                filename = strdup(argv[1]);
                outputfile = fopen(strdup(argv[6]), "w");
                fluxoutputfile = fopen(strdup(argv[4]), "w");
                return;
            }
        }
        else if (strcmp(argv[4], "-a") == 0) 
        {
            auto_proc = 1;

            if ((strcmp(argv[2], "-o") == 0) && (strcmp(argv[5], "-f") == 0))
            {
                filename = strdup(argv[1]);
                outputfile = fopen(strdup(argv[3]), "w");
                fluxoutputfile = fopen(strdup(argv[6]), "w");
                return;
            }
            else if ((strcmp(argv[2], "-f") == 0) && (strcmp(argv[5], "-o") == 0))
            {
                filename = strdup(argv[1]);
                outputfile = fopen(strdup(argv[6]), "w");
                fluxoutputfile = fopen(strdup(argv[3]), "w");
                return;
            }

        }
        else if (strcmp(argv[6], "-a") == 0) 
        {
            auto_proc = 1;

            if ((strcmp(argv[2], "-o") == 0) && (strcmp(argv[4], "-f") == 0))
            {
                filename = strdup(argv[1]);
                outputfile = fopen(strdup(argv[3]), "w");
                fluxoutputfile = fopen(strdup(argv[5]), "w");
                return;
            }
            else if ((strcmp(argv[2], "-f") == 0) && (strcmp(argv[4], "-o") == 0))
            {
                filename = strdup(argv[1]);
                outputfile = fopen(strdup(argv[5]), "w");
                fluxoutputfile = fopen(strdup(argv[3]), "w");
                return;
            }

        }


    }

    fprintf(stderr,"Usage: %s <config file> [-a] [-o <output file>] [-f <flux output file>] -m\n", argv[0]);
    fprintf(stderr,"\t\t<config file> : The configuration XML file.\n");
    fprintf(stderr,"\t\t-o : Write position output file.\n");
    fprintf(stderr,"\t\t-f : Write flux line output file.\n");
    fprintf(stderr,"\t\t-a : Auto process.\n");
    fprintf(stderr,"\t\t-m : Send MATLAB updates of the time variable over a fifo.\n\n");

    exit(1);

}

void parse_config(log_cfg_type *cfg)
{
    xmlParserCtxtPtr ctxt;      /* the parser context */
    xmlDocPtr xmldoc;           /* the resulting document tree */
    xmlNodePtr cur;

    assert(cfg != NULL);


    ctxt = xmlNewParserCtxt();
    assert(ctxt != NULL);

    xmldoc = xmlCtxtReadFile(ctxt, filename, XML_ENCODING, XML_PARSE_DTDVALID);
    assert(xmldoc != NULL);

    cur = xmlDocGetRootElement(xmldoc);
    assert(cur != NULL);
    
    if (!log_cfg_parse_node(cfg, xmldoc, cur))
    {
        xmlFreeParserCtxt(ctxt);
        exit(1);
    }

    xmlFreeParserCtxt(ctxt);

    log_cfg_period_update_backgrounds(cfg);
}

void write_config_output(log_cfg_type *cfg)
{
    xmlTextWriterPtr writer;

    assert(cfg != NULL);

    writer = xmlNewTextWriterFilename(filename, 0);
    if (writer == NULL)
    {
        fprintf(stderr,"Unable to write to file \"%s\"\n", filename);
        return;
    }

    xmlTextWriterSetIndent(writer, 1);

    if (xmlTextWriterStartDocument(writer, NULL, "ISO-8859-1", NULL) < 0)
        return;

    if (xmlTextWriterStartDTD(writer, "logconfiguration", NULL, "dtds/log_cfg.dtd") < 0)
        return;

    if (xmlTextWriterEndDTD(writer) < 0)
        return;

    log_cfg_xml_write(cfg, writer);

    if (xmlTextWriterEndDocument(writer) < 0)
        return;

    xmlFreeTextWriter(writer);
}


int main(int argc, char **argv)
{
    log_cfg_type *cfg;
    rtk_app_t *app;
    mainwnd_t *mainwnd;

    rtk_init(&argc, &argv);
    parse_args(argc, argv);

    signal(SIGINT, sig_quit);
    signal(SIGQUIT, sig_quit);

    app = rtk_app_create();

    cfg = log_cfg_create();
    parse_config(cfg);

    mainwnd = create_main_wnd(app, cfg, outputfile, fluxoutputfile, auto_proc, communicate_with_matlab);


    rtk_app_main_init(app);

    while (!quit)
    {
        rtk_app_main_loop(app);

        if (mainwnd_update(mainwnd) != 0)
            quit = 1;
        usleep(100); // This must be enough to do all the rendering updates or 
                       // we run into issues with growing event lists.
    }

    rtk_app_main_term(app);

    log_cfg_destroy(cfg);

    if (outputfile != NULL)
        fclose(outputfile);

    if (fluxoutputfile != NULL)
        fclose(fluxoutputfile);

    return 0;
}

