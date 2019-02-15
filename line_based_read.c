/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: line_based_read.c                     Date: 7 Jul. 2007        **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#include "line_based_read.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "utils.h"

struct _lbr_state_type 
{
    int seek_step;
    int wrapped;
    FILE *fp;
};

char * _lbr_print_message(const char *fmt, ...);
char * _lbr_readline_helper(lbr_state_type *lbr);
void _lbr_seeker(lbr_state_type *lbr, int num);


/* This is a large string, but is based on the laser logs that I looked at. It is an estimate, all the routines are supposed to work when the line is longer */
#define LBR_LINE_LENGTH 3500

/**
 * Returns a newly allocated lbr context block associated with the file, or NULL on failure 
 */
lbr_state_type *lbr_fopen(const char* fname)
{
    lbr_state_type *ret;

    ret = (lbr_state_type *)malloc(sizeof(lbr_state_type));
    if (ret == NULL)
        return NULL;

    ret->seek_step = 1;
    ret->wrapped = 0;
    ret->fp = fopen(fname, "r");


    if (ret->fp == NULL)
    {
        free(ret);
        ret = NULL;
    }

    return ret;
}



/**
 * Reads one line from the given file
 * @param lbr The state block associated with the file
 * @return A newly allocated string with the line, or NULL if no line was available 
 */
char * lbr_readline(lbr_state_type *lbr)
{
    char * ret;

    assert(lbr != NULL);

    ret = _lbr_readline_helper(lbr);

    if (ret != NULL)
    {
        _lbr_seeker(lbr, lbr->seek_step-1);

    }
    else
    { // read feof
        if (feof(lbr->fp))
        {

            //printf("1) at = %ld\n", ftell(lbr->fp));
            fseek(lbr->fp, 0, SEEK_END);
            //printf("2) at = %ld\n", ftell(lbr->fp));

            //Seek back 1 line:
            _lbr_seeker(lbr, -1);
            //printf("3) at = %ld\n", ftell(lbr->fp));

            ret = lbr_readline(lbr);
            lbr->wrapped = 1;
            return ret;

        }
    }

    return ret;
}


/**
 * Take a single step
 */
void lbr_seek_step(lbr_state_type *lbr)
{
    assert(lbr != NULL);

    _lbr_seeker(lbr, lbr->seek_step);
}

void _lbr_print_line(char *buffer)
{
    char *t;
    char *l;
    t = strdup(buffer);
    l = t;

    while (*l != '\0')
    {
        if (*l == '\n') *l = '_';
        *l++;
    }

    printf("%s\n", t);

    free(t);
}

/**
 * Seek
 */
void _lbr_seeker(lbr_state_type *lbr, int num)
{
    int i, n;
    long init_pos;
    long curr_pos;
    long first_backward;
    static char buffer[LBR_LINE_LENGTH+1];

    assert(lbr != NULL);

//printf("at %ld\n", ftell(lbr->fp));

    if (num == 0) 
    {
        lbr->wrapped = 0;
        return;
    }

    init_pos = ftell(lbr->fp);

//printf("asked to seek %d\n", num);

    if (num < 0)
    {
        curr_pos  = init_pos;
        first_backward = -2;

        while (num < 0)
        {
            fseek(lbr->fp, MAX(0, curr_pos - LBR_LINE_LENGTH), SEEK_SET);

            // Now, read string
            i = fread(buffer, sizeof(char), LBR_LINE_LENGTH, lbr->fp);
            buffer[i] = '\0';

//_lbr_print_line(buffer);

            // Search backward until we find an \n
            n = MIN(strlen(buffer), curr_pos);
            for (i = n-1; (i >= 0) && (curr_pos > 0); i--)
            {
                //printf("[%d] Checking buffer[%d] = '%c'\n", curr_pos, i, buffer[i]);

                if (buffer[i] == '\n')
                {
                    if (first_backward == -2) 
                        first_backward++;
                    else if (first_backward == -1)
                        first_backward  = curr_pos;
                    if (num == 0)
                    {
                        fseek(lbr->fp, curr_pos, SEEK_SET);
                        //printf("moved to %ld\n", ftell(lbr->fp));
                        lbr->wrapped = 0;
                        return;
                    }
                    num++;
                }

                curr_pos--;

            }

            if (curr_pos < 0)
            {
                fseek(lbr->fp, MAX(0, first_backward), SEEK_SET);
                lbr->wrapped = 1;
                //printf("fb = %ld\n", first_backward);
                //printf("homed to %ld (homed)\n", ftell(lbr->fp));
                return;
            }

            if (curr_pos == 0)
            {
                if ((first_backward < 0) && (num < 0))
                    lbr->wrapped = 1;
                fseek(lbr->fp, 0, SEEK_SET);
                return;
            }



            // We've not read back enough, so must keep going.
        }

    }
    else
    {
        curr_pos  = init_pos;

        while (num > 0)
        {
            // Read string
            i = fread(buffer, sizeof(char), LBR_LINE_LENGTH, lbr->fp);
            buffer[i] = '\0';

            if ((i == 0) && (feof(lbr->fp)))
            {
                fseek(lbr->fp, init_pos, SEEK_SET);
                //Seek back 1 line:
                _lbr_seeker(lbr, -1);
                lbr->wrapped = 1;
                return; //We've seeked to the end of the file.
            }

            for (i = 0; (i < strlen(buffer)) && (num > 0); i++)
            {
                if (buffer[i] == '\n')
                {
                    num--;
                }
                curr_pos++;
            }

        }

        fseek(lbr->fp, curr_pos, SEEK_SET);
    }

    lbr->wrapped = 0;
}

char * _lbr_readline_helper(lbr_state_type *lbr)
{

    char *temp, *new;
    char line[LBR_LINE_LENGTH];
    int sl, sn;

    assert(lbr != NULL);

    if (fgets(line, LBR_LINE_LENGTH, lbr->fp))
    {
        new = strdup(line);

        sl = strlen(line);

        if (line[sl-1] == '\n')
            return new;


        while (sl == (LBR_LINE_LENGTH-1))
        { // read incremental bits


            if (fgets(line, LBR_LINE_LENGTH, lbr->fp))
            {
                sl = strlen(line);
                sn = strlen(new);
                //printf("line = [%s] sl = %d\n", line, sl);

                temp = malloc(sizeof(char) * sl * sn + 1);
                strcpy(&(temp[0]), new);
                strcpy(&(temp[sn]), line);
                free(new);
                new = temp;

                if (line[sl-1] == '\n')
                    break;

            }
            else
            {
                free(new);
                return NULL;
            }
        }

        return new;
    }
    else
        return NULL;
}


/** 
 * Utility command. Given a printf style string, it'll return and freshly allocated string with the arguments properly written to it.
 * @param fmt The printf style format string, plus additional arguments as necessary.
 * @return A newly allocated string containing the output
 * @note This is GNU code from the "Linux Programmer's Manual" printf command man page, section 3.
 */
char *_lbr_print_message(const char *fmt, ...)
{
    /* Guess we need no more than 100 bytes. */
    int n, size = 100;
    char *p, *np;
    va_list ap;

    if ((p = malloc(size)) == NULL)
        return NULL;

    while (1)
    {
        /* Try to print in the allocated space. */
        va_start(ap, fmt);
        n = vsnprintf(p, size, fmt, ap);
        va_end(ap);
        /* If that worked, return the string. */
        if (n > -1 && n < size)
            return p;
        /* Else try again with more space. */
        if (n > -1)             /* glibc 2.1 */
            size = n + 1;       /* precisely what is needed */
        else                    /* glibc 2.0 */
            size *= 2;          /* twice the old size */
        if ((np = realloc(p, size)) == NULL)
        {
            free(p);
            return NULL;
        }
        else
        {
            p = np;
        }
    }

}

/**
 * Closes the file and destroys the lbr context block 
 */
void lbr_fclose(lbr_state_type *lbr)
{
    assert(lbr != NULL);
    fclose(lbr->fp);
    free(lbr);
}


/*
 * Set the size to seek each read or step
 */
void lbr_seek_size_set(lbr_state_type *lbr, int size)
{
    assert(lbr != NULL);

    if (!lbr->wrapped)
        _lbr_seeker(lbr, - lbr->seek_step + size );
    else
        _lbr_seeker(lbr,  size );

    lbr->seek_step = size;

}

/*
 * Get the size to seek each read or step
 */
int lbr_seek_size_get(lbr_state_type *lbr)
{
    assert(lbr != NULL);
    return lbr->seek_step;
}

/*
 * Checks whether at the beginning (with negative seek step) or end (with
 * postive seek step)
 */
int lbr_at_end_of_file(lbr_state_type *lbr)
{
    assert(lbr != NULL);

    if (lbr->seek_step > 0)
        return lbr->wrapped;
    else if (lbr->seek_step < 0)
        return lbr->wrapped;
    else 
        return 0;
}


