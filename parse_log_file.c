/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: parse_log_file.c                      Date: 11 Jul. 2007       **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#include "parse_log_file.h"
#include "line_based_read.h"
#include <wctype.h>

char *filename;
int line_step_size = 1;
FILE *outputfp;

#define LASER_LOG_HEADER 9

//Fwd decls.
void parse_args(int argc, char **argv);

void print_data(laser_data_type *data)
{
    int i;
    printf("%lf [ ",data->timestamp);

    for (i = 0; i < LASER_ARRAY_LEN; i++)
        printf("%f ", data->data[i]);

    printf("] \n");
}

void write_data(laser_data_type *data)
{
    fwrite(data, sizeof(laser_data_type), 1, outputfp);
}

void parse_line(laser_data_type *data, char *line)
{
    char *l, *p;
    int i, j;

    l = line;

    while (iswspace(l[0])) l = &(l[1]);
    while (!iswspace(l[0])) l = &(l[1]);

    l[0] = '\0';
    data->timestamp = strtod(line, NULL);
    // Skip some fields:
    p = &(l[1]);
    for (i = 0; i < 12; i++)
    {
       while (iswspace(*l)) l = &(l[1]);
       while (!iswspace(*l)) l = &(l[1]);
    }

    for (j = 0; j < LASER_ARRAY_LEN; j++)
    {
        p = l;
        // Seek to the end of the field
        while (iswspace(p[0])) p = &(p[1]);
        while (!iswspace(p[0])) p = &(p[1]);
        p[0] = '\0';
        data->data[j] = strtod(l, NULL);
        // Now, skip the next field (the intensity)
        p = &(p[1]);
        while (iswspace(p[0])) p = &(p[1]);
        while (!iswspace(p[0])) p = &(p[1]);
        l = p;
    }

}


int main(int argc, char **argv)
{
    int i;
    char *l;
    long count;
    laser_data_type laser_data;
    lbr_state_type *lbr;

    parse_args(argc, argv);

    lbr = lbr_fopen(filename);

    if (lbr == NULL)
    {
        fprintf(stderr,"Unable to process input\n");
        exit(1);
    }
    else
    {
        l = (char *)malloc(sizeof(char) * (5 + strlen(filename)));
        sprintf(l,"%s.dat", filename);
        outputfp = fopen(l,"wb");
        free(l);
    }

    lbr_seek_size_set(lbr, 1);
    for (i = 0; i < LASER_LOG_HEADER; i++)
        lbr_seek_step(lbr);

    count = 0;
    lbr_seek_size_set(lbr, line_step_size);

    while (!lbr_at_end_of_file(lbr))
    {
        l = lbr_readline(lbr);

        if (lbr_at_end_of_file(lbr))
        {
            free(l);
            break;
        }

        /////////////////////////////////////

        parse_line(&laser_data, l);
        //print_data(&laser_data);
        write_data(&laser_data);
        count++;
        

        /////////////////////////////////////

        free(l);
    }

    fprintf(stderr,"%ld records written\n", count);

    lbr_fclose(lbr);

    fclose(outputfp);

    free(filename);
    return 0;
}



void parse_args(int argc, char **argv)
{
    if (argc == 2)
    {
        filename = strdup(argv[1]);
        return;
    }


    if (argc == 4)
    {
        if (strcmp(argv[1],"-n") == 0)
        {
            line_step_size = atoi(argv[2]);
            if (line_step_size > 0)
            {
                filename = strdup(argv[3]);
                return;
            }
        }
    }

    fprintf(stderr,"Usage: %s [-n step_size] <input_log_file>\n", argv[0]);
    fprintf(stderr,"\t\t-n step_size     : Parse log file with lines \"step_size\" apart.\n");
    fprintf(stderr,"\t\t<input_log_file> : The log file name.\n\n");

    exit(1);
}
