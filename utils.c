/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: utils.c                               Date: 14 Jul. 2007       **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <math.h>

/** word wrap string (borrowed from the PPR Print Spooler project 
 *
 * This function will re-line-wrap a string so that it has lines whose
 * length do not exceed a certain limit.
 */
int wordwrap(char *string, int width)
{
    const char *si;
    char *di;
    int curlen;                 /* length of the current line */
    int wordlen;
    int spacelen;               /* number of spaces at end of line */
    int last_char_was_newline;

    if (width <= 0)
        return 0;

    last_char_was_newline = 0;

    for (si = di = string, spacelen = curlen = 0; *si; spacelen = 0)
    {

        while (*si == ' ' || *si == '\n')
        {
            if (*si == '\n')
            {
                if (last_char_was_newline)
                {
                    // Double new line, start a new line
                    di -= spacelen;
                    *(di++) = '\n';
                    curlen = spacelen = 0;
                    si++;
                    continue;
                }
                last_char_was_newline = 1;
            }
            else
                last_char_was_newline = 0;
            /* Copy spaces to the destination */
            /* if they are not at the */
            if (curlen > 0)     /* begining of the line. */
            {
                *(di++) = ' ';
                curlen++;
                spacelen++;
            }
            si++;
        }

        wordlen = strcspn(si, " \n");   /* length of next word */


        /* If it won't fit, take back the space(s) and replace 
         * it/them with a linefeed.
         */
        if (curlen > 0 && (curlen + wordlen) > width)
        {
            di -= spacelen;
            *(di++) = '\n';
            curlen = spacelen = 0;
        }

        /* copy the word */
        strncpy(di, si, wordlen);
        si += wordlen;
        di += wordlen;
        curlen += wordlen;
        last_char_was_newline = 0;
    }

    if (di > string && *(di - 1) == ' ')
        *(di - 1) = '\n';

    *di = '\0';
    return (di - string);
}


void get_multi_line_text_extents(GdkFont * font, const gchar * text, gint
                                 text_length, gint * width, gint * height)
{
    int i, len, baseline;
    int wid, ascent, descent;

    int max_wid; 
    int start_y = 0;
    int end_y = 0;

    if (text_length == 0)
    {
        if (width != NULL)
            (*width) = 0;
        if (height != NULL)
            (*height) = 0;
        return;
    }

    max_wid = -1;
    baseline = 0;
    i = 0;
    while (i < text_length)
    {
        len = strcspn(text + i, "\n");

        // Compute the bounding box for the text
        gdk_text_extents(font, text + i, len,
                         NULL, NULL, &wid, &ascent, &descent);

        if (max_wid < wid)
            max_wid = wid;

        if (i == 0)
        {
            start_y = baseline - ascent;
        }

        end_y = baseline + descent;


        // Compute the baseline for the next line of text
        baseline += 14 * (ascent + descent) / 10;
        i += len + 1;
    }


    if (width != NULL)
        (*width) = max_wid;
    if (height != NULL)
        (*height) = end_y - start_y;

}

/** 
 * Utility command. Given a printf style string, it'll return and freshly allocated string with the arguments properly written to it.
 * @param fmt The printf style format string, plus additional arguments as necessary.
 * @return A newly allocated string containing the output
 * @note This is GNU code from the "Linux Programmer's Manual" printf command man page, section 3.
 */
char *utils_print_message(const char *fmt, ...)
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
 * Utility command. Given a printf style string, it'll return and freshly allocated string with the arguments properly written to it. (Same as @ref utils_print_message, except this version uses the strarg variable argument list
 * @param fmt The printf style format string, plus additional arguments as necessary.
 * @param ap A variable parameter list
 * @return A newly allocated string containing the output
 */
char *utils_vprint_message(const char *fmt, va_list ap)
{
    /* Guess we need no more than 100 bytes. */
    int n, size = 100;
    char *p, *np;

    if ((p = malloc(size)) == NULL)
        return NULL;

    while (1)
    {
        /* Try to print in the allocated space. */
        n = vsnprintf(p, size, fmt, ap);
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

float utils_median5(float x[5])
{
    float mmin, mmax;
    int imin, imax;
    float y[3];
    float t;
    int i, j;

    mmin = x[0];
    imin = 0;
    mmax = x[0];
    imax = 0;

    for (i = 1; i < 5; i++)
    {
        if (x[i] < mmin)
        {
            mmin = x[i];
            imin = i;
        }
        if (x[i] > mmax)
        {
            mmax = x[i];
            imax = i;
        }
    }


    j = 0;
    for (i = 0; i < 5; i++)
    {
        if ((i != imin) && (i != imax))
        {
            y[j++] = x[i];
        }
    }

    if (y[0] > y[2])
    {
        t = y[0];
        y[0] = y[2];
        y[2] = t;
    }

    if (y[0] > y[1])
    {
        t = y[0];
        y[0] = y[1];
        y[1] = t;
    }

    if (y[1] > y[2])
    {
        t = y[1];
        y[1] = y[2];
        y[2] = t;
    }

    //printf("<%f %f %f %f %f> = <%f %f %f> = %f\n", x[0], x[1], x[2], x[3], x[4], y[0], y[1], y[2], y[1]);

    return y[1];
}

float utils_max5(float x[5])
{
    int i;
    float m;

    m = x[0];
    for (i = 1; i < 5; i++)
        m = MAX(m, x[i]);

    return m;
}

inline double uniform_rand()
    // Number between 0 and 1
{
    return ((double)rand()) / ((double)RAND_MAX);
}

/**
 * Calculate a normally distributed random number with mean zero and unit std_dev 
 * @return A random number
 * @note Modified algorithm by Dr. Everett (Skip) Carter, Jr. from http://www.taygeta.com/random/gaussian.html
 */
inline double normal_rand()
{
    static int even = 0;
    static float next = 0.0;
    float x1, x2, w, y1, y2;

    if (even == 0)
    {
        do
        {
            x1 = 2.0 * uniform_rand() - 1.0;
            x2 = 2.0 * uniform_rand() - 1.0;
            w = x1 * x1 + x2 * x2;
        }
        while (w >= 1.0);

        w = sqrt((-2.0 * logf(w)) / w);
        y1 = x1 * w;
        y2 = x2 * w;
        even = 1;
        next = y2;
        return y1;
    }
    else
    {
        even = 0;
        return next;
    }
}


double get_current_time()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return (double)tv.tv_sec + ((double)tv.tv_usec)/1000000.0;
}

int file_exist(char *name)
{
    struct stat buf;
    int i = stat(name, &buf);
    /* File found */
    if (i == 0)
    {
        return 1;
    }
    return 0;
}

#define ONE_DEG_IN_RAD 0.0174532925 /**< Conversion factor for degrees to radians */

/**
 * Converts a degree argument into the radians equivalent
 * @param deg The angle in degrees
 * @return The same angle in radians
 */
inline double deg_to_rad(double deg)
{
    return (deg * ONE_DEG_IN_RAD);
}

/**
 * Converts a radian argument into the degrees equivalent
 * @param rad The angle in radians
 * @return The same angle in degrees
 */
inline double rad_to_deg(double rad)
{
    return (rad / ONE_DEG_IN_RAD);
}


#define SAME_SIGN(a, b) (((a) * (b)) >= 0)

int line_intersection(double x1, double y1, double x2, double y2, double x3, double y3,
              double x4, double y4, double *outx, double *outy)
{

    double a1, a2, b1, b2, c1, c2;
    double r1, r2, r3, r4;
    double denom, offset, num;
    double o_x = 0.0;
    double o_y = 0.0;

    // Compute a1, b1, c1, where line joining points 1 and 2
    // is \u201ca1 x + b1 y + c1 = 0\u2033.
    a1 = y2 - y1;
    b1 = x1 - x2;
    c1 = (x2 * y1) - (x1 * y2);

    // Compute r3 and r4.
    r3 = ((a1 * x3) + (b1 * y3) + c1);
    r4 = ((a1 * x4) + (b1 * y4) + c1);

    // Check signs of r3 and r4. If both point 3 and point 4 lie on
    // same side of line 1, the line segments do not intersect.
    if ((r3 != 0) && (r4 != 0) && SAME_SIGN(r3, r4))
    {
        return DONT_INTERSECT;
    }

    // Compute a2, b2, c2
    a2 = y4 - y3;
    b2 = x3 - x4;
    c2 = (x4 * y3) - (x3 * y4);

    // Compute r1 and r2
    r1 = (a2 * x1) + (b2 * y1) + c2;
    r2 = (a2 * x2) + (b2 * y2) + c2;

    // Check signs of r1 and r2. If both point 1 and point 2 lie
    // on same side of second line segment, the line segments do
    // not intersect.
    if ((r1 != 0) && (r2 != 0) && (SAME_SIGN(r1, r2)))
    {
        return DONT_INTERSECT;
    }

    //Line segments intersect: compute intersection point.
    denom = (a1 * b2) - (a2 * b1);

    if (denom == 0)
    {
        o_x = (x1 + x2) * 0.5;
        o_y = (y1 + y2) * 0.5;

        if (outx != NULL)
            (*outx) = o_x;
        if (outy != NULL)
            (*outy) = o_y;

        return COLLINEAR;
    }

    /* this gives a rounded 
    if (denom < 0)
    {
        offset = -denom / 2;
    }
    else
    {
        offset = denom / 2;
    }*/

    offset = 0.0;

    // The denom/2 is to get rounding instead of truncating. It
    // is added or subtracted to the numerator, depending upon the
    // sign of the numerator.
    num = (b1 * c2) - (b2 * c1);
    if (num < 0)
    {
        o_x = (num - offset) / denom;
    }
    else
    {
        o_x = (num + offset) / denom;
    }

    num = (a2 * c1) - (a1 * c2);
    if (num < 0)
    {
        o_y = (num - offset) / denom;
    }
    else
    {
        o_y = (num + offset) / denom;
    }

    if (outx != NULL)
        (*outx) = o_x;
    if (outy != NULL)
        (*outy) = o_y;


    // lines_intersect
    return DO_INTERSECT;
}



