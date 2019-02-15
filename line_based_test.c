/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: line_based_test.c                     Date: 7 Jul. 2007        **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "line_based_read.h"

#define INPUT_FILE "logs/tester.log"


char *stripnewlines(char *t)
{
    char *l;
    l = t;
    if ( l == NULL) return NULL;
    while (*l != '\0')
    {
        if (*l == '\n') *l = '_';
        *l++;
    }
    return t;
}

int main()
{
    char *l;
    lbr_state_type *lbr;
    
    lbr = lbr_fopen(INPUT_FILE);
    assert(lbr != NULL);

    lbr_seek_size_set(lbr, 3);


    free(lbr_readline(lbr));
    free(lbr_readline(lbr));
    free(lbr_readline(lbr));
    free(lbr_readline(lbr));
    free(lbr_readline(lbr));
    free(lbr_readline(lbr));

    lbr_seek_size_set(lbr, 1);

    while (1)
    {
        l = lbr_readline(lbr);

        printf("[%d] %s", lbr_at_end_of_file(lbr), l);
        free(l);

        if (lbr_at_end_of_file(lbr))
        {
            break;
        }
    }

    lbr_fclose(lbr);

    return 0;
}


/* Conditions to test
 *  Read past end of file,
 *      Keep reading past eof
 *      Read after reading past eof
 *  Read past beginning of file,
 *      Keep reading past bof
 *      Read after reading past bof
 *  Operations on an empty file
 *  Empty lines
 *    first line empty
 *    last line empty
 *  Lines same length as buffers
 */
