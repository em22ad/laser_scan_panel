/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: wnd_annos.h                              Date: 11 Jul. 2007    **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#ifndef _WND_ANNOS_H_
#define _WND_ANNOS_H_

#include "main_wnd.h"
#include "rtk2/rtk.h"

void wnd_anno_add_annotation_figures(mainwnd_t *wnd);
void wnd_anno_edit_dlg_create(mainwnd_t * wnd, rtk_fig_t * annot_fig);
void wnd_anno_del_annotation_figures(mainwnd_t * wnd);

#endif
