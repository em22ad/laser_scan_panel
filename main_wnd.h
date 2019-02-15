/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: main_wnd.h                               Date: 11 Jul. 2007    **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/


#ifndef _MAIN_WND_H_
#define _MAIN_WND_H_

#include "log_configuration.h"
#include "laser_read.h"
#include "rtk2/rtk.h"
#include "tracking_mod.h"

#define COLOUR_GRID_MAJOR         0xC0C0C0
#define COLOUR_GRID_MINOR         0xE0E0E0
#define COLOUR_GRID_BG            0xeae5fb
#define COLOUR_GRID_LINE          0x606060
#define COLOUR_BG_FILL            0xE0E0E0
#define COLOUR_BG_OUTLINE         0xA0A0A0
#define COLOUR_TL_LINE            0xC0C0C0
#define COLOUR_TL_PERIODSPLIT     0x404040
#define COLOUR_TL_BG              0xffffaf
#define COLOUR_TL_ANNO            0xdedf22
#define COLOUR_TL_SH_ANNO         0x000000
#define COLOUR_TL_CURSOR          0xdf4822
#define COLOUR_PRD_ACTIVE         0x3a19a8
#define COLOUR_PRD_INACTIVE       0xa6a2b1
#define COLOUR_ANNO_BG            0xf0f0f0
#define COLOUR_ANNO_FG            0xd2d2d2
#define COLOUR_ANNO_TXT           0x000000
#define COLOUR_LASER_RECT         0x5555ff
#define COLOUR_LASER_HIGH         0x5555ff
#define COLOUR_LASER_FORE         0xffff00
#define COLOUR_LASER_RANGES_FG    0x8b81f2
#define COLOUR_LASER_RANGES_BG    0xaba1ff
#define COLOUR_LASER_BG           0xaaaadd
#define COLOUR_LASER_FG           0x66bb66
#define COLOUR_LASER_CLUSTER      0xe78232
#define COLOUR_LASER_TR_POS       0xe732e7
#define COLOUR_LASER_TR_POS_G     0x777777
#define COLOUR_LASER_TR_PART      0xa2d5d9
#define COLOUR_LASER_TR_PART1     0xF20509
#define COLOUR_LASER_TR_PART2     0x0205F9
#define COLOUR_FLUXLINE           0xff9999

#define SPACE_UNITS 0.01
#define ANNOT_WIDTH 0.075
#define CURSOR_WIDTH 0.003
#define PERIOD_WIDTH 0.175
#define TIME_LINE_HEIGHT 160.0

#define TIME_TO_TL_SPACE(w, x) ((((x) - (w)->time_start)*SPACE_UNITS) - (0.5 * (w)->timeline_length))
#define TL_SPACE_TO_TIME(w, x)  ((((x) + (0.5 * (w)->timeline_length))/(SPACE_UNITS)) + (w)->time_start)


typedef struct _perlaserstate_t perlaserstate_t;
typedef struct _mainwnd_t mainwnd_t;


struct _perlaserstate_t
{
    rtk_fig_t *laser_fig;
    rtk_fig_t *data_fig;
    rtk_fig_t *bg_fig;
    int active_period;
    int render_count;
};

// Main window displaying sensor stuff
struct _mainwnd_t
{ 
    // The rtk canvas
    rtk_canvas_t *canvas;
    rtk_canvas_t *timeline;

    // The grid figure (fixed to cs)
    rtk_fig_t *grid_fig;
    // The horizontal lines figure 
    rtk_fig_t *timeline_bg_fig;
    rtk_fig_t **timeline_log_figs;
    rtk_fig_t **timeline_anno_figs;
    rtk_fig_t *timeline_anno_sum_fig;
    rtk_fig_t *timeline_cursor_fig;
    rtk_fig_t *timeline_cursor_pos_fig;
    double time_start, time_end;
    double timeline_length;

    // Background outline
    rtk_fig_t *outline_scan_fig;
    rtk_fig_t *foreground_fig;
    rtk_fig_t *tracking_visualization_fig;
    rtk_fig_t *flux_visualization_fig;

    // Menu containing file options
    GtkAccelGroup *accel_group;
    rtk_menu_t *file_menu;
    rtk_menuitem_t *save_item;
    rtk_menuitem_t *exit_item;
    // Menu containing annotation options
    rtk_menu_t *anno_menu;
    rtk_menuitem_t *add_anno_item;
    rtk_menuitem_t *show_anno_item;
    rtk_menuitem_t *lock_anno_item;
    // Menu containing log options
    rtk_menuitem_t *add_log_item;
    // Menu containing playback options
    rtk_menu_t *playback_menu;
    rtk_menuitem_t *ffwd_item;
    rtk_menuitem_t *proc_item;
    rtk_menuitem_t *play_item;
    rtk_menuitem_t *rew_item;
    rtk_menuitem_t *stop_item;
    rtk_menuitem_t *time_based_item;
    rtk_menuitem_t *line_based_item;
    rtk_menu_t *playback_options_menu;
    rtk_menuitem_t *timestep_001_item;
    rtk_menuitem_t *timestep_01_item;
    rtk_menuitem_t *timestep_1_item;
    rtk_menuitem_t *do_flux_item;
    // Menu containing view options
    rtk_menu_t *view_menu;
    rtk_menuitem_t *show_map_outline_item;
    rtk_menuitem_t *show_map_fill_item;
    rtk_menuitem_t *show_bg_item;
    rtk_menuitem_t *show_fg_item;
    rtk_menuitem_t *show_clusters_item;
    rtk_menuitem_t *show_fluxlines_item;
    rtk_menuitem_t *show_tracking_pos_item;
    rtk_menuitem_t *show_tracking_part_item;
    // Menu containing laser options
    rtk_menu_t *laser_menu;
    rtk_menuitem_t *show_laser_outline_item;
    rtk_menuitem_t *show_laser_fill_item;
    rtk_menuitem_t *lock_laser_item;
    rtk_menu_t *rate_sub_menu;
    rtk_menuitem_t *rate_1_item;
    rtk_menuitem_t *rate_2_item;
    rtk_menuitem_t *rate_5_item;
    rtk_menuitem_t *rate_10_item;

    // State for the simulation
    double current_time;
    int processing;

    // Playback:
    int play_stepping;
    perlaserstate_t *laser_play_data;
    int laser_play_data_n;
    int render_rate;
    double time_step_size;;

    // Tracking:
    tracking_mod_t *tr_mod;
    int auto_process;

    log_cfg_type *cfg;

    FILE *output_file;
    FILE *fluxout_file;
    
    // Should send status commands to matlab?
    int send_matlab_status;
    double last_status_message;

};


mainwnd_t *create_main_wnd(rtk_app_t *app, log_cfg_type *cfg, FILE *outfile, FILE *fluxoutfile, int auto_process, int send_matlab_status);
int mainwnd_update(mainwnd_t *wnd);
void flush_hints(mainwnd_t *wnd);
void redraw_lasers(mainwnd_t *wnd);
void draw_laser_figure(mainwnd_t *wnd, int num);
void draw_laser_data_figure(mainwnd_t *wnd, int num);



#endif
