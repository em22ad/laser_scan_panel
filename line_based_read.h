/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: line_based_read.h                     Date: 7 Jul. 2007        **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#ifndef _LINE_BASED_READ_H_
#define _LINE_BASED_READ_H_

#include <stdio.h>


typedef struct _lbr_state_type lbr_state_type; /**< Private structure that wraps hierarchical config context */

lbr_state_type *lbr_fopen(const char* fname);
char * lbr_readline(lbr_state_type *lbr);
void lbr_fclose(lbr_state_type *lbr);

void lbr_seek_step(lbr_state_type *lbr);

void lbr_seek_size_set(lbr_state_type *lbr, int size);
int lbr_seek_size_get(lbr_state_type *lbr);

int lbr_at_end_of_file(lbr_state_type *lbr);



#endif
