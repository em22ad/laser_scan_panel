/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: parse_log_file.h                      Date: 11 Jul. 2007       **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#ifndef _PARSE_LOG_FILE_H_
#define _PARSE_LOG_FILE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//#define LASER_ARRAY_LEN 361
#define LASER_ARRAY_LEN 181

typedef struct _laser_data_type laser_data_type;

struct _laser_data_type
{
    double timestamp;
    float data[LASER_ARRAY_LEN];
};




#endif
