/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: utils.h                               Date: 14 Jul. 2007       **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#ifndef _UTILS_H_
#define _UTILS_H_

#include <gdk/gdk.h>

int wordwrap(char *string, int width);

void get_multi_line_text_extents(GdkFont * font, const gchar * text, gint
                                 text_length, gint * width, gint * height);


char *utils_print_message(const char *fmt, ...);
char *utils_vprint_message(const char *fmt,va_list ap); 

float utils_median5(float x[5]);
float utils_max5(float x[5]);

inline double uniform_rand();
inline double normal_rand();
inline double normal_rand_fast();

double get_current_time();

int file_exist(char *name);

inline double rad_to_deg(double rad);
inline double deg_to_rad(double deg);

#define DONT_INTERSECT 0
#define COLLINEAR 1
#define DO_INTERSECT 2
int line_intersection(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, double *outx, double *outy);

 
#ifndef FLOATSIGN
#define FLOATSIGN(a, b) ((b) >= 0.0 ? fabs(a) : -fabs(a))
#endif 

#ifndef MIN
#define MIN(x,y) ( (x) < (y) ? (x) : (y) )
#endif

#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif

/** Return the sign of the argument */
#ifndef SGN
#define SGN(x) (((x) == 0) ? 0 : (((x) > (0)) ? (+1) : (-1)))
#endif
 
#ifndef SQR
#define SQR(x) ((x) * (x))
#endif
 



#endif 
