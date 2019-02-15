/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: wnd_prog.h                               Date: 29 Jul. 2007    **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#ifndef _WND_PROG_H_
#define _WND_PROG_H_

#include <gtk/gtk.h>

GtkWindow *wnd_progress_dialog_create(char *title, char *label);
void wnd_progress_dialog_destroy(GtkWindow *win);
void wnd_progress_dialog_set_progress(GtkWindow *win, float fract);


#endif
