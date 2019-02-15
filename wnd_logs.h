/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: wnd_logs.h                               Date: 11 Jul. 2007    **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#ifndef _WND_LOGS_H_
#define _WND_LOGS_H_

#include "main_wnd.h"
#include "rtk2/rtk.h"

void wnd_log_add_log_figures(mainwnd_t * wnd);
void wnd_log_del_log_figures(mainwnd_t * wnd);
void wnd_log_edit_dlg_create(mainwnd_t * wnd, rtk_fig_t *log_fig, int num_log);
void wnd_log_edit_period_dlg_create(mainwnd_t * wnd, rtk_fig_t *log_fig, int num_log, int num_period);

#endif
