/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: main_wnd.c                               Date: 02 Apr. 2008    **
** Author: Dylan A. Shell/Muhammad Emad                                 **
** Notes:                                                               **
*************************************************************************/

#include "main_wnd.h"
#include "background_img.h"
#include "utils.h"
#include <assert.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include "wnd_logs.h"
#include "wnd_annos.h"

#define TIME_RESOLUTION 0.1

#define TIME_WINDOW 0.05

#define CURSOR_DIFF 0.01

#define MATLAB_COMMS_FILE "/tmp/matlabcomms"

typedef struct _flux_line_data_type flux_line_data_type;
struct _flux_line_data_type 
{
    int n;
    int *counts;
};

int must_flush_to_disk = 0;

void sig_flush_to_disk(int signum)
{
    must_flush_to_disk = 1;
}




extern void write_config_output(log_cfg_type *cfg);

void build_timeline(mainwnd_t *wnd);
void move_cursor(mainwnd_t *wnd, double new_time, int jump);

void cursor_mouse_ev(struct _rtk_fig_t *fig, int event, int mode);
void timeline_canvas_click(void *cvs, int buttons);
void laser_mouse_ev(struct _rtk_fig_t *fig, int event, int mode);

void move_through_logs(mainwnd_t *wnd);
void draw_laser_bg_figure(mainwnd_t *wnd, int num);

void process_data(mainwnd_t *wnd, double *X, double *Y, int N);

void setup_fluxlines(mainwnd_t *wnd);

mainwnd_t *create_main_wnd(rtk_app_t *app, log_cfg_type *cfg, FILE *outfile, FILE *fluxoutfile, int auto_process, int send_matlab_status)
{
    GtkWidget *frame, *vpane, *menu_bar;
    GtkWidget *layout1, *layout2;
    GSList *group;

    int i;
    int an_locked = 1; // Start up unlocked
    int an_show = 1; // Start up showing the annotations
    int laser_locked = 1; // Start up with locked
    int laser_show_fill = 0; // Start up with show filled
    int laser_show_outl = 1; // Start up with show filled
    int time_based_play = 1; // Start playing by lines
    mainwnd_t *wnd;

    assert(app != NULL);
    assert(cfg != NULL);

    wnd = malloc(sizeof(mainwnd_t));
    wnd->current_time = 0.5;
    wnd->time_start = -2.0;
    wnd->time_end = 2.0;
    wnd->cfg = cfg;
    wnd->output_file = outfile;
    wnd->fluxout_file = fluxoutfile;

    wnd->send_matlab_status = send_matlab_status;
    wnd->last_status_message = -1.0;
    
    wnd->laser_play_data_n = 0;
    wnd->laser_play_data = NULL;

    wnd->render_rate = 10;
    wnd->time_step_size = TIME_RESOLUTION;

    wnd->processing = 0;
    wnd->play_stepping = 0;

    setup_fluxlines(wnd);

    frame = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    wnd->accel_group = gtk_accel_group_new ();
    gtk_window_add_accel_group(GTK_WINDOW(frame), wnd->accel_group);

    vpane = gtk_vpaned_new();
    layout1 = gtk_vbox_new(FALSE, 0);
    layout2 = gtk_vbox_new(FALSE, 0);
    menu_bar = gtk_menu_bar_new();

    gtk_container_add(GTK_CONTAINER(frame), vpane);
    gtk_container_add(GTK_CONTAINER(vpane), layout1);
    gtk_container_add(GTK_CONTAINER(vpane), layout2);
    gtk_box_pack_start(GTK_BOX(layout1), menu_bar, FALSE, FALSE, 0);

    // This is a bug, but the ordering here tells which will get the 
    // mouse wheel/scroll events for zooming
    wnd->timeline = rtk_canvas_create_given_frame(app, frame, layout2, menu_bar);
    wnd->timeline->userdata = wnd;
    wnd->canvas = rtk_canvas_create_given_frame(app, frame, layout1, menu_bar);
    wnd->canvas->userdata = wnd;

    gtk_paned_set_position(GTK_PANED(vpane), 540);
    //gtk_paned_set_position(GTK_PANED(vpane), 380);

    gtk_container_child_set(GTK_CONTAINER(vpane), layout1, "resize", TRUE, "shrink", TRUE, NULL);
    gtk_container_child_set(GTK_CONTAINER(vpane), layout2, "resize", FALSE, "shrink", TRUE, NULL);

    rtk_canvas_movemask(wnd->canvas, RTK_MOVE_PAN | RTK_MOVE_ZOOM);
    rtk_canvas_size(wnd->canvas, 1200, 600);
    rtk_canvas_scale(wnd->canvas, 0.015, 0.015);
    rtk_canvas_origin(wnd->canvas, -1, 4.5);

    rtk_canvas_movemask(wnd->timeline, RTK_MOVE_PAN_HORIZ | RTK_MOVE_ZOOM);
    rtk_canvas_size(wnd->timeline, 1200, 600);
    rtk_canvas_scale(wnd->timeline, 0.020, 0.020);
    rtk_canvas_origin(wnd->timeline, 0, 0);

    rtk_canvas_title(wnd->canvas, "Laser Scan Panel");

    // Create file menu
    wnd->file_menu = rtk_menu_create(wnd->canvas, "_File");
    wnd->add_log_item = rtk_menuitem_create(wnd->file_menu, "_Import Logfile", 0);
    gtk_widget_add_accelerator(wnd->add_log_item->item, "activate", wnd->accel_group,
            gdk_keyval_from_name("n"), GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_append(GTK_MENU(wnd->file_menu->menu), gtk_separator_menu_item_new());
    wnd->save_item = rtk_menuitem_create(wnd->file_menu, "_Save", 0);
    gtk_widget_add_accelerator(GTK_WIDGET(wnd->save_item->item), "activate", wnd->accel_group,
            gdk_keyval_from_name("s"), GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    wnd->exit_item = rtk_menuitem_create(wnd->file_menu, "E_xit", 0);


    gtk_widget_add_accelerator(wnd->exit_item->item, "activate", wnd->accel_group,
            gdk_keyval_from_name("q"), GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    wnd->playback_menu = rtk_menu_create(wnd->canvas, "_Playback");
    wnd->proc_item = rtk_menuitem_create(wnd->playback_menu, "Pr_ocess", 0);
    gtk_menu_append(GTK_MENU(wnd->playback_menu->menu), gtk_separator_menu_item_new());
    wnd->play_item = rtk_menuitem_create(wnd->playback_menu, "_Play", 0);
    wnd->ffwd_item = rtk_menuitem_create(wnd->playback_menu, "_Fast-Forward", 0);
        gtk_widget_add_accelerator(wnd->ffwd_item->item, "activate", wnd->accel_group,
            gdk_keyval_from_name("f"), GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
        wnd->rew_item = rtk_menuitem_create(wnd->playback_menu, "_Rewind", 0);
        gtk_widget_add_accelerator(wnd->rew_item->item, "activate", wnd->accel_group,
            gdk_keyval_from_name("r"), GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    wnd->stop_item = rtk_menuitem_create(wnd->playback_menu, "_Stop", 0);
    gtk_widget_add_accelerator(wnd->play_item->item, "activate", wnd->accel_group,
            gdk_keyval_from_name("p"), GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_append(GTK_MENU(wnd->playback_menu->menu), gtk_separator_menu_item_new());

    wnd->time_based_item = rtk_menuitem_create(wnd->playback_menu, "_Time playback", 2);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(wnd->time_based_item->item));
    wnd->line_based_item = rtk_menuitem_create(wnd->playback_menu, "_Line playback", 2);
    gtk_radio_menu_item_set_group(GTK_RADIO_MENU_ITEM(wnd->line_based_item->item), group);
    rtk_menuitem_check(wnd->time_based_item, time_based_play);
    rtk_menuitem_check(wnd->line_based_item, !time_based_play);

    gtk_menu_append(GTK_MENU(wnd->playback_menu->menu), gtk_separator_menu_item_new());
    wnd->playback_options_menu = rtk_menu_create_sub(wnd->playback_menu, "Time step size");
    wnd->timestep_001_item = rtk_menuitem_create(wnd->playback_options_menu, "0.01 seconds", 2);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(wnd->timestep_001_item->item));
    wnd->timestep_01_item = rtk_menuitem_create(wnd->playback_options_menu , "0.1 seconds", 2);
    gtk_radio_menu_item_set_group(GTK_RADIO_MENU_ITEM(wnd->timestep_01_item->item), group);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(wnd->timestep_001_item->item));
    wnd->timestep_1_item = rtk_menuitem_create(wnd->playback_options_menu , "1.0 seconds", 2);
    gtk_radio_menu_item_set_group(GTK_RADIO_MENU_ITEM(wnd->timestep_1_item->item), group);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(wnd->timestep_001_item->item));

    gtk_menu_append(GTK_MENU(wnd->playback_menu->menu), gtk_separator_menu_item_new());
    wnd->do_flux_item = rtk_menuitem_create(wnd->playback_menu, "_Flux Calculation", TRUE);
    rtk_menuitem_check(wnd->do_flux_item, wnd->fluxout_file != NULL);
    rtk_menuitem_enable(wnd->do_flux_item, wnd->fluxout_file != NULL);

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wnd->timestep_001_item->item), wnd->time_step_size == 0.01);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wnd->timestep_01_item->item),  wnd->time_step_size == 0.1);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wnd->timestep_1_item->item),   wnd->time_step_size == 1.0);

    rtk_menuitem_enable(wnd->ffwd_item, wnd->play_stepping != 5);
    rtk_menuitem_enable(wnd->rew_item, wnd->play_stepping != -1);
    rtk_menuitem_enable(wnd->play_item, wnd->play_stepping != 1);
    rtk_menuitem_enable(wnd->stop_item, wnd->play_stepping != 0);


    wnd->view_menu = rtk_menu_create(wnd->canvas, "_View");
    wnd->show_map_fill_item = rtk_menuitem_create(wnd->view_menu, "_Tunnel Fill", 1);
    wnd->show_map_outline_item = rtk_menuitem_create(wnd->view_menu, "Tunnel _Outline", 1);
    wnd->show_bg_item = rtk_menuitem_create(wnd->view_menu, "Learnt _Background", 1);
    rtk_menuitem_check(wnd->show_bg_item, 0);
    wnd->show_fg_item = rtk_menuitem_create(wnd->view_menu, "Extracted Foreground", 1);
    rtk_menuitem_check(wnd->show_fg_item, 1);
    wnd->show_clusters_item = rtk_menuitem_create(wnd->view_menu, "Clusters", 1);
    rtk_menuitem_check(wnd->show_clusters_item, 0);
    wnd->show_tracking_pos_item = rtk_menuitem_create(wnd->view_menu, "Tracking", 1);
    rtk_menuitem_check(wnd->show_tracking_pos_item , 1);
    wnd->show_tracking_part_item = rtk_menuitem_create(wnd->view_menu, "Tracking Part.", 1);
    rtk_menuitem_check(wnd->show_tracking_part_item , 1);

    wnd->show_fluxlines_item = rtk_menuitem_create(wnd->view_menu, "Flux lines", 1);
    rtk_menuitem_check(wnd->show_fluxlines_item, 1);

    wnd->laser_menu = rtk_menu_create(wnd->canvas, "_Lasers");
    wnd->lock_laser_item = rtk_menuitem_create(wnd->laser_menu, "_Lock", 1);
    gtk_menu_append(GTK_MENU(wnd->laser_menu->menu), gtk_separator_menu_item_new());
    wnd->show_laser_fill_item = rtk_menuitem_create(wnd->laser_menu, "Show _Fill", 1);
    wnd->show_laser_outline_item = rtk_menuitem_create(wnd->laser_menu, "Show _Outline", 1);
    wnd->rate_sub_menu = rtk_menu_create_sub(wnd->laser_menu, "Render _Rate");
    wnd->rate_1_item = rtk_menuitem_create(wnd->rate_sub_menu, "Every Frame", 2);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(wnd->rate_1_item->item));
    wnd->rate_2_item = rtk_menuitem_create(wnd->rate_sub_menu, "Second Frame", 2);
    gtk_radio_menu_item_set_group(GTK_RADIO_MENU_ITEM(wnd->rate_2_item->item), group);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(wnd->rate_1_item->item));
    wnd->rate_5_item = rtk_menuitem_create(wnd->rate_sub_menu, "Fifth Frame", 2);
    gtk_radio_menu_item_set_group(GTK_RADIO_MENU_ITEM(wnd->rate_5_item->item), group);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(wnd->rate_1_item->item));
    wnd->rate_10_item = rtk_menuitem_create(wnd->rate_sub_menu, "Tenth Frame", 2);
    gtk_radio_menu_item_set_group(GTK_RADIO_MENU_ITEM(wnd->rate_10_item->item), group);

    rtk_menuitem_check(wnd->lock_laser_item, laser_locked);
    rtk_menuitem_check(wnd->show_laser_fill_item, laser_show_fill);
    rtk_menuitem_check(wnd->show_laser_outline_item, laser_show_outl);

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wnd->rate_1_item->item), wnd->render_rate == 1);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wnd->rate_2_item->item), wnd->render_rate == 2);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wnd->rate_5_item->item), wnd->render_rate == 5);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wnd->rate_10_item->item), wnd->render_rate == 10);

    wnd->anno_menu = rtk_menu_create(wnd->canvas, "_Annotations");
    wnd->add_anno_item = rtk_menuitem_create(wnd->anno_menu, "_New", 0);

    gtk_menu_append(GTK_MENU(wnd->anno_menu->menu), gtk_separator_menu_item_new());

    wnd->lock_anno_item = rtk_menuitem_create(wnd->anno_menu, "_Lock", TRUE);
    wnd->show_anno_item = rtk_menuitem_create(wnd->anno_menu, "_Show", TRUE);
    rtk_menuitem_check(wnd->lock_anno_item, an_locked);
    rtk_menuitem_enable(wnd->add_anno_item, !an_locked);
    rtk_menuitem_check(wnd->show_anno_item, an_show);

    // Create figure to draw the grid on
    wnd->grid_fig = rtk_fig_create_ex(wnd->canvas, NULL, -90, wnd);

    rtk_fig_color_rgb32(wnd->grid_fig, COLOUR_GRID_BG);
    rtk_fig_rectangle(wnd->grid_fig,0.0, 0.0, 0.0, 50.0, 50.0, 1);
    //rtk_fig_color_rgb32(wnd->grid_fig, COLOUR_GRID_MINOR);
    //rtk_fig_grid(wnd->grid_fig, 0, 0, 50, 50, 0.2);
    rtk_fig_color_rgb32(wnd->grid_fig, COLOUR_GRID_MAJOR);
    rtk_fig_grid(wnd->grid_fig, 0, 0, 50, 50, 1.0);


    wnd->foreground_fig = rtk_fig_create_ex(wnd->canvas, NULL, -40, wnd);
    wnd->tracking_visualization_fig = rtk_fig_create_ex(wnd->canvas, NULL, -30, wnd);
    wnd->flux_visualization_fig= rtk_fig_create_ex(wnd->canvas, NULL, -50, wnd);

    wnd->outline_scan_fig = rtk_fig_create_ex(wnd->canvas, NULL, -80, wnd);
    rtk_menuitem_check(wnd->show_map_fill_item, 1);
    if (rtk_menuitem_ischecked(wnd->show_map_fill_item))
    {
        rtk_fig_color_rgb32(wnd->outline_scan_fig, COLOUR_BG_FILL);
        rtk_fig_polygon(wnd->outline_scan_fig, 0.0, 0.0, 0.0, NUM_BG_LINE_SEQS_1, bg_lines_sequence_1, 1);
    }

    rtk_menuitem_check(wnd->show_map_outline_item, 0);
    if (rtk_menuitem_ischecked(wnd->show_map_outline_item))
    {
        rtk_fig_color_rgb32(wnd->outline_scan_fig, COLOUR_BG_OUTLINE);
        for (i = 1;i < NUM_BG_LINE_SEQS_1; i++)
        {
            rtk_fig_line(wnd->outline_scan_fig, bg_lines_sequence_1_x[i-1], bg_lines_sequence_1_y[i-1],
                    bg_lines_sequence_1_x[i], bg_lines_sequence_1_y[i]);

        }
    }


    wnd->timeline_bg_fig = NULL;
    wnd->tr_mod = NULL;
    
    build_timeline(wnd);
    wnd->tr_mod = tracking_mod_create();

    if (auto_process)
        wnd->auto_process = -1;
    else
        wnd->auto_process = 0;

    signal(SIGUSR1, sig_flush_to_disk);

    return wnd;
}


void build_timeline(mainwnd_t *wnd)
{
    double x;
    assert(wnd != NULL);

    if (wnd->timeline_bg_fig != NULL)
        rtk_fig_clear(wnd->timeline_bg_fig);

    log_cfg_times_get(wnd->cfg, &wnd->time_start, &wnd->time_end); 
    wnd->time_start -= 2.0;
    wnd->time_end += 2.0;

    //printf("<%f %f>\n", wnd->time_start, wnd->time_end);

    // For each unit of time, give SPACE_UNITS of area
    wnd->timeline_length = (wnd->time_end - wnd->time_start)*SPACE_UNITS;

    // Create figure to draw the grid on
    wnd->timeline_bg_fig = rtk_fig_create_ex(wnd->timeline, NULL, -90, wnd);
    rtk_fig_color_rgb32(wnd->timeline_bg_fig, COLOUR_TL_BG);
    rtk_fig_rectangle(wnd->timeline_bg_fig, 0.0, 0.0, 0.0, wnd->timeline_length, TIME_LINE_HEIGHT, 1);

    // Figure for the cursor number time
    wnd->timeline_cursor_pos_fig = rtk_fig_create_ex(wnd->canvas, NULL, 60, wnd);
    rtk_fig_clear(wnd->timeline_cursor_pos_fig);

    wnd->timeline_cursor_fig = rtk_fig_create_ex(wnd->timeline, NULL, -60, wnd);
    // The place timeline cursor
    rtk_fig_color_rgb32(wnd->timeline_cursor_fig, COLOUR_TL_CURSOR);

    x = TIME_TO_TL_SPACE(wnd, wnd->current_time);

    rtk_fig_rectangle(wnd->timeline_cursor_fig, 0.0, 0.0, 0.0, CURSOR_WIDTH, TIME_LINE_HEIGHT, 1);

    rtk_fig_origin_global(wnd->timeline_cursor_fig, x, 0.0, 0.0);

    rtk_fig_movemask(wnd->timeline_cursor_fig, RTK_MOVE_TRANS_HORIZ);
    rtk_fig_add_mouse_handler(wnd->timeline_cursor_fig, cursor_mouse_ev);


    // Add the annotations
    wnd->timeline_anno_figs = NULL;
    wnd_anno_add_annotation_figures(wnd);

    // Add the logfile figures
    wnd->timeline_log_figs = NULL;
    wnd_log_add_log_figures(wnd);


    // Figure for the annotation details
    wnd->timeline_anno_sum_fig = rtk_fig_create_ex(wnd->canvas, NULL, 60, wnd);
    rtk_fig_clear(wnd->timeline_anno_sum_fig);

    // Update cursor other things
    move_cursor(wnd, wnd->current_time, 1);

    rtk_canvas_add_mouse_click_handler(wnd->timeline, timeline_canvas_click);
}


void shutdown_matlab(mainwnd_t *wnd)
{
    if (wnd->send_matlab_status)
    {
        FILE *fifo;
        fifo = fopen(MATLAB_COMMS_FILE, "a");
        fprintf(fifo,  "quit\n");
        fclose(fifo);
    } 

}


// Update the window.
// Returns 1 if the program should quit.
int mainwnd_update(mainwnd_t *wnd)
{
    int i;
    double x;
    log_cfg_anno_type *new_anno;
    log_cfg_logfile_type *new_log;
    int new_log_num;
    int new_stepping;




    // See if we should quit
    if (rtk_canvas_isclosed(wnd->canvas))
    {
        tracking_mod_destroy(wnd->tr_mod);
        if (wnd->send_matlab_status) 
            shutdown_matlab(wnd);
        return 1;
    }
    if (rtk_menuitem_isactivated(wnd->exit_item))
     {
        tracking_mod_destroy(wnd->tr_mod);
        if (wnd->send_matlab_status) 
            shutdown_matlab(wnd);
        return 1;
    } 

    if (rtk_menuitem_isactivated(wnd->save_item))
    {
        write_config_output(wnd->cfg);
        return 0;
    }


    if (rtk_menuitem_isactivated(wnd->lock_anno_item))
    {
        if (rtk_menuitem_ischecked(wnd->lock_anno_item))
        {
            rtk_menuitem_enable(wnd->add_anno_item, FALSE);
            
            // Lock the figures
            assert(wnd->timeline_anno_figs != NULL);

            for (i = 0; i < wnd->cfg->num_annos; i++)
                rtk_fig_movemask(wnd->timeline_anno_figs[i],
                                RTK_REGISTER_ALL_MOUSE_CLICKS);
        }
        else
        {
            rtk_menuitem_enable(wnd->add_anno_item, TRUE);

            // Unlock the figures
            assert(wnd->timeline_anno_figs != NULL);

            for (i = 0; i < wnd->cfg->num_annos; i++)
                rtk_fig_movemask(wnd->timeline_anno_figs[i],
                                RTK_MOVE_TRANS_HORIZ |
                                RTK_REGISTER_ALL_MOUSE_CLICKS);
        }

    }

    if (rtk_menuitem_isactivated(wnd->show_anno_item))
    {
        if (rtk_menuitem_ischecked(wnd->show_anno_item))
        { // Must show

            assert(wnd->timeline_anno_figs != NULL);

            for (i = 0; i < wnd->cfg->num_annos; i++)
            {
                rtk_fig_rectangle(wnd->timeline_anno_figs[i], 0.0, 0.0, 0.0,
                        ANNOT_WIDTH, TIME_LINE_HEIGHT, 1);

                
                if (rtk_menuitem_ischecked(wnd->lock_anno_item))
                {
                    rtk_fig_movemask(wnd->timeline_anno_figs[i],
                            RTK_REGISTER_ALL_MOUSE_CLICKS);
                }
                else
                {
                    rtk_fig_movemask(wnd->timeline_anno_figs[i],
                            RTK_MOVE_TRANS_HORIZ |
                            RTK_REGISTER_ALL_MOUSE_CLICKS);
                }
            }
        }
        else
        { // Must hide
            rtk_canvas_unselect(wnd->timeline);
            rtk_fig_clear(wnd->timeline_anno_sum_fig);

            for (i = 0; i < wnd->cfg->num_annos; i++)
            {
                rtk_fig_clear(wnd->timeline_anno_figs[i]);
                rtk_fig_movemask(wnd->timeline_anno_figs[i], 0);
            }
        }
    }

    if (rtk_menuitem_isactivated(wnd->add_anno_item))
    {
        // Scroll to the cursor
        x = TIME_TO_TL_SPACE(wnd, wnd->current_time);
        rtk_canvas_origin(wnd->timeline, x, 0.0);
    
        // Delete the figures
        wnd_anno_del_annotation_figures(wnd);

        // Add the annotation
        new_anno = log_cfg_add_annotation(wnd->cfg);
        new_anno->time = wnd->current_time;

        // Add the figures again
        wnd_anno_add_annotation_figures(wnd);

        wnd_anno_edit_dlg_create(wnd, new_anno->user_data);
    }

    if (rtk_menuitem_isactivated(wnd->add_log_item))
    {
        // Delete the figures
        wnd_log_del_log_figures(wnd);

        // Add the new log
        new_log = log_cfg_add_logfile(wnd->cfg);

        // Add the figures again
        wnd_log_add_log_figures(wnd);

        new_log_num  = -1;
        for (i = 0; i < wnd->cfg->num_logfiles; i++)
            if (wnd->cfg->logfiles[i] == new_log)
                new_log_num = i;


        wnd_log_edit_dlg_create(wnd, new_log->user_data, new_log_num);
    }


    if ((rtk_menuitem_isactivated(wnd->show_map_fill_item)) || (rtk_menuitem_isactivated(wnd->show_map_outline_item)))
    {
        rtk_fig_clear(wnd->outline_scan_fig);

        if (rtk_menuitem_ischecked(wnd->show_map_fill_item))
        {
            rtk_fig_color_rgb32(wnd->outline_scan_fig, COLOUR_BG_FILL);
            rtk_fig_polygon(wnd->outline_scan_fig, 0.0, 0.0, 0.0, NUM_BG_LINE_SEQS_1, bg_lines_sequence_1, 1);
        }
        if (rtk_menuitem_ischecked(wnd->show_map_outline_item))
        {
            rtk_fig_color_rgb32(wnd->outline_scan_fig, COLOUR_BG_OUTLINE);
            for (i = 1;i < NUM_BG_LINE_SEQS_1; i++)
            {
                rtk_fig_line(wnd->outline_scan_fig, bg_lines_sequence_1_x[i-1], bg_lines_sequence_1_y[i-1],
                        bg_lines_sequence_1_x[i], bg_lines_sequence_1_y[i]);

            }

        }
    }

    if (rtk_menuitem_isactivated(wnd->show_bg_item))
    {
        for (i = 0; i < wnd->laser_play_data_n; i++)
            draw_laser_bg_figure(wnd, i);
    }


    if (rtk_menuitem_isactivated(wnd->show_fg_item))
    {
        rtk_fig_clear(wnd->foreground_fig);
        for (i = 0; i < wnd->cfg->num_logfiles; i++)
            wnd->laser_play_data[i].render_count = -1;

        redraw_lasers(wnd);
    }

    if (rtk_menuitem_isactivated(wnd->show_fluxlines_item))
    {
        redraw_lasers(wnd);
    }



    if ((rtk_menuitem_isactivated(wnd->show_laser_fill_item)) || (rtk_menuitem_isactivated(wnd->show_laser_outline_item)))
    {
        for (i = 0; i < wnd->cfg->num_logfiles; i++)
        {
            wnd->laser_play_data[i].render_count = -1;
            rtk_fig_clear(wnd->laser_play_data[i].data_fig);
        }
        redraw_lasers(wnd);
    }

    if (rtk_menuitem_isactivated(wnd->lock_laser_item))
    {
        for (i = 0; i < wnd->laser_play_data_n; i++)
            draw_laser_figure(wnd, i);
    }

    if ((rtk_menuitem_isactivated(wnd->rate_1_item)) || (rtk_menuitem_isactivated(wnd->rate_2_item)) || (rtk_menuitem_isactivated(wnd->rate_5_item)) || (rtk_menuitem_isactivated(wnd->rate_10_item)))
    {
        if (rtk_menuitem_ischecked(wnd->rate_1_item))
            wnd->render_rate = 1;
        else if (rtk_menuitem_ischecked(wnd->rate_2_item))
            wnd->render_rate = 2;
        else if (rtk_menuitem_ischecked(wnd->rate_5_item))
            wnd->render_rate = 5;
        else if (rtk_menuitem_ischecked(wnd->rate_10_item))
            wnd->render_rate = 10;

        if (wnd->laser_play_data != NULL)
            for (i = 0; i < wnd->cfg->num_logfiles; i++)
                wnd->laser_play_data[i].render_count = -1;
    }
    

    if ((rtk_menuitem_isactivated(wnd->timestep_1_item)) || (rtk_menuitem_isactivated(wnd->timestep_01_item)) || (rtk_menuitem_isactivated(wnd->timestep_001_item)))
    {
        if (rtk_menuitem_ischecked(wnd->timestep_1_item))
            wnd->time_step_size = 1.0;
        else if (rtk_menuitem_ischecked(wnd->timestep_01_item))
            wnd->time_step_size = 0.1;
        else if (rtk_menuitem_ischecked(wnd->timestep_001_item))
            wnd->time_step_size = 0.01;
    }

    new_stepping = wnd->play_stepping;
    if (rtk_menuitem_isactivated(wnd->ffwd_item))
        new_stepping = 5;
    if (rtk_menuitem_isactivated(wnd->rew_item))
        new_stepping = -1;
    if (rtk_menuitem_isactivated(wnd->play_item))
        new_stepping = 1;
    if (rtk_menuitem_isactivated(wnd->stop_item))
    {
        new_stepping = 0;
        wnd->processing = 0;
    }

    if ((rtk_menuitem_isactivated(wnd->proc_item)) || (wnd->auto_process == -1))
    {
        if (wnd->auto_process == -1) // from negative, on start up.
        {
            double start_time = log_cfg_all_period_starting_times(wnd->cfg);
            if (start_time < 0.0)
                return 1; // there is no active window to process.
            move_cursor(wnd, start_time, 1);

            wnd->auto_process = 1;

        }

        rtk_menuitem_check(wnd->time_based_item, 0);
        rtk_menuitem_check(wnd->line_based_item, 1);

        rtk_menuitem_check(wnd->show_fg_item, 1);
        rtk_fig_clear(wnd->foreground_fig);
        for (i = 0; i < wnd->cfg->num_logfiles; i++)
            wnd->laser_play_data[i].render_count = -1;
        redraw_lasers(wnd);

        //rtk_menuitem_check(wnd->show_bg_item, 1);
        //for (i = 0; i < wnd->laser_play_data_n; i++)
        //    draw_laser_bg_figure(wnd, i);

        rtk_menuitem_check(wnd->rate_10_item, 1);
        rtk_menuitem_check(wnd->rate_5_item, 0);
        rtk_menuitem_check(wnd->rate_2_item, 0);
        rtk_menuitem_check(wnd->rate_1_item, 0);

        new_stepping = 1;
        wnd->processing = 1;

        if (wnd->auto_process)
        { // I prefer these settings for the non-interactive processing
            rtk_menuitem_check(wnd->show_clusters_item, 0);
            rtk_menuitem_check(wnd->show_tracking_pos_item , 0);
            rtk_menuitem_check(wnd->show_tracking_part_item , 0);
            rtk_menuitem_check(wnd->show_fg_item, 0);

            rtk_fig_clear(wnd->foreground_fig);
            rtk_fig_clear(wnd->tracking_visualization_fig);

        }
    }

    


    if (wnd->play_stepping != new_stepping)
    {
        wnd->play_stepping = new_stepping;
        for (i = 0; i < wnd->cfg->num_logfiles; i++)
            lr_seek_size_set(wnd->cfg->logfiles[i]->lr, new_stepping);

        rtk_menuitem_enable(wnd->ffwd_item, wnd->play_stepping != 5);
        rtk_menuitem_enable(wnd->rew_item, wnd->play_stepping != -1);
        rtk_menuitem_enable(wnd->play_item, wnd->play_stepping != 1);
        rtk_menuitem_enable(wnd->stop_item, wnd->play_stepping != 0);

        if (wnd->play_stepping != 0)
        {
            gtk_widget_remove_accelerator(wnd->play_item->item,  wnd->accel_group, gdk_keyval_from_name("p"), GDK_CONTROL_MASK);
            gtk_widget_add_accelerator(wnd->stop_item->item, "activate", wnd->accel_group,
                    gdk_keyval_from_name("p"), GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
        }
        else
        {
            gtk_widget_remove_accelerator(wnd->stop_item->item,  wnd->accel_group, gdk_keyval_from_name("p"), GDK_CONTROL_MASK);
            gtk_widget_add_accelerator(wnd->play_item->item, "activate", wnd->accel_group,
                    gdk_keyval_from_name("p"), GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
        }
    }


    if (wnd->play_stepping != 0)
    {
        move_cursor(wnd, wnd->current_time + ((float)wnd->play_stepping) * wnd->time_step_size, 0);
    }


    // Render the canvas
    rtk_canvas_render(wnd->canvas);

    // Render the timeline
    rtk_canvas_render(wnd->timeline);


    return 0;
}


void cursor_mouse_ev(struct _rtk_fig_t *fig, int event, int mode)
{
    double x;
    int ix, iy, width;
    double dx, dy;
    char txt[200];
    assert(fig);

    mainwnd_t *wnd = (mainwnd_t *)fig->userdata;
    /*
    if (event == RTK_EVENT_PRESS) printf("PRESS ");
    else if (event == RTK_EVENT_MOTION) printf("MOTION ");
    else if (event == RTK_EVENT_RELEASE) printf("RELEASE ");
    else if (event == RTK_EVENT_MOUSE_OVER) printf("MOUSE_OVER ");
    else if (event == RTK_EVENT_MOUSE_NOT_OVER) printf("MOUSE_NOT_OVER ");
    */

    x = TL_SPACE_TO_TIME(wnd, fig->dox);
    if (x > wnd->time_end)
    {
        rtk_fig_origin_global(fig, TIME_TO_TL_SPACE(wnd, wnd->time_end), fig->doy, fig->doa);
    }
    else if (x < wnd->time_start)
    {
        rtk_fig_origin_global(fig, TIME_TO_TL_SPACE(wnd, wnd->time_start), fig->doy, fig->doa);
    }

    move_cursor(wnd, TL_SPACE_TO_TIME(wnd, fig->dox), 1);

    if ((event == RTK_EVENT_MOUSE_OVER) || (event == RTK_EVENT_MOTION))
    { // Hack to display in the correct place on other pane
        rtk_fig_clear(wnd->timeline_cursor_pos_fig);
        rtk_fig_color_rgb32(wnd->timeline_cursor_pos_fig, COLOUR_TL_CURSOR);
        snprintf(txt, 200, "%6.2f", wnd->current_time);

        // How long would the text be:
        gdk_text_extents(wnd->canvas->font, txt, strlen(txt),
                     NULL, NULL, &width, NULL, NULL);

        ix = rtk_canvas_logical_to_dev_x(wnd->timeline, TIME_TO_TL_SPACE(wnd, wnd->current_time)) - width/2;
        dx = rtk_canvas_dev_to_logical_x(wnd->canvas, ix);
        iy = (wnd->canvas->canvas)->allocation.height - 10;
        dy = rtk_canvas_dev_to_logical_y(wnd->canvas, iy);

        rtk_fig_text(wnd->timeline_cursor_pos_fig, dx, dy, 0.0, txt);
    }
    else
    if (event == RTK_EVENT_MOUSE_NOT_OVER) 
    {
        rtk_fig_clear(wnd->timeline_cursor_pos_fig);
    }

}

void timeline_canvas_click(void *cvs, int buttons)
{
    rtk_canvas_t *canvas =  (rtk_canvas_t *)cvs;
    mainwnd_t *wnd = (mainwnd_t *)canvas->userdata;

    // Move the current time cursor after button 2 is pressed
    if (buttons == 2)
    {
        move_cursor(wnd, TL_SPACE_TO_TIME(wnd, rtk_canvas_dev_to_logical_x(wnd->timeline, wnd->timeline->mouse_x)), 1);
    }
}

void flush_hints(mainwnd_t *wnd)
{
    int i;

    if (wnd->laser_play_data_n > 0)
    {
        // Free the wnd->laser_play_data

        for (i = 0; i < wnd->laser_play_data_n; i++)
        {
            rtk_fig_destroy(wnd->laser_play_data[i].laser_fig);
            rtk_fig_destroy(wnd->laser_play_data[i].data_fig);
            rtk_fig_destroy(wnd->laser_play_data[i].bg_fig);
        }
        free(wnd->laser_play_data);

        // Force load at next update  
        wnd->laser_play_data_n = 0;
        wnd->laser_play_data = NULL;
    }
    // Make sure that next time run, we do get a render:
    for (i = 0; i < wnd->laser_play_data_n; i++)
        wnd->laser_play_data[i].render_count = -1;
}

void redraw_lasers(mainwnd_t *wnd)
{
    move_cursor(wnd, wnd->current_time, 0);
}

void draw_laser_figure(mainwnd_t *wnd, int num)
{
    int h;
    assert(num < wnd->laser_play_data_n);

    h = wnd->laser_play_data[num].active_period;
    if (h == -1)
    {
        rtk_fig_clear(wnd->laser_play_data[num].laser_fig);

    }
    else
    {
        rtk_fig_clear(wnd->laser_play_data[num].laser_fig);
        rtk_fig_color_rgb32(wnd->laser_play_data[num].laser_fig, COLOUR_LASER_RECT);
        rtk_fig_rectangle(wnd->laser_play_data[num].laser_fig, 0.0, 0.0, 0.0, 0.3, 0.3, 1);
        rtk_fig_origin_global(wnd->laser_play_data[num].laser_fig, wnd->cfg->logfiles[num]->periods[h]->position_x, 
                wnd->cfg->logfiles[num]->periods[h]->position_y, 
                wnd->cfg->logfiles[num]->periods[h]->position_t);

        if (!rtk_menuitem_ischecked(wnd->lock_laser_item))
        {
            rtk_fig_movemask(wnd->laser_play_data[num].laser_fig, RTK_MOVE_TRANS | RTK_MOVE_ROT);
            rtk_fig_add_mouse_handler(wnd->laser_play_data[num].laser_fig, laser_mouse_ev);
        }
        else
        {
            rtk_fig_movemask(wnd->laser_play_data[num].laser_fig, 0);
            rtk_fig_remove_mouse_handler(wnd->laser_play_data[num].laser_fig, laser_mouse_ev);
        }
    }

}

void draw_laser_data_figure(mainwnd_t *wnd, int num)
{
    int h;
    assert(num < wnd->laser_play_data_n);

    h = wnd->laser_play_data[num].active_period;
    if (h == -1)
    {
        rtk_fig_clear(wnd->laser_play_data[num].data_fig);
    }
    else
    {
        rtk_fig_clear(wnd->laser_play_data[num].data_fig);
        rtk_fig_origin_global(wnd->laser_play_data[num].data_fig, wnd->cfg->logfiles[num]->periods[h]->position_x, 
                wnd->cfg->logfiles[num]->periods[h]->position_y, 
                wnd->cfg->logfiles[num]->periods[h]->position_t);
    }
}

void draw_laser_bg_figure(mainwnd_t *wnd, int num)
{
    int h, j;
    double angl, r, R;
    int_item_type *item;

    assert(num < wnd->laser_play_data_n);

    if (num != 0) return;

    h = wnd->laser_play_data[num].active_period;
    if (h == -1)
    {
        rtk_fig_clear(wnd->laser_play_data[num].bg_fig);
    }
    else
    {
        rtk_fig_clear(wnd->laser_play_data[num].bg_fig);
        rtk_fig_color_rgb32(wnd->laser_play_data[num].bg_fig, COLOUR_LASER_BG);

        rtk_fig_linewidth(wnd->laser_play_data[num].bg_fig, 3);

        if (rtk_menuitem_ischecked(wnd->show_bg_item))
        {

            for (j = 0; j < LASER_ARRAY_LEN; j++)
            {

                item = wnd->cfg->logfiles[num]->periods[h]->background_lists[j]->head;
                while (item != NULL)
                {
                    angl = -M_PI_2	+ M_PI * ((j - 0.1) / 180.0);

                    rtk_fig_color_rgb32(wnd->laser_play_data[num].bg_fig, 
                            COLOUR_LASER_BG);// + (int)rint((float)item->count * 5.0 / ((float)wnd->cfg->logfiles[num]->periods[h]->background_lists[j]->total_count)));

                    //rtk_fig_color_rgb32(wnd->laser_play_data[num].bg_fig, 
                    //        COLOUR_LASER_BG + (int)rint(100.0*(float)item->count));

                    rtk_fig_color_rgb32(wnd->laser_play_data[num].bg_fig, 
                            COLOUR_LASER_BG + (int)rint((float)item->count * 250.0 / ((float)wnd->cfg->logfiles[num]->periods[h]->background_lists[j]->total_count)));

                    r = item->start_value;
                    R = item->end_value;

                    /*if (R - 0.05 < r) 
                    {
                        r = r - 0.025;
                        R = r + 0.05;
                    }*/

                    rtk_fig_line(wnd->laser_play_data[num].bg_fig,
                            r*cos(angl), r*sin(angl),
                            R*cos(angl), R*sin(angl));

                    item = item->next;
                }


            }
            
        }

        rtk_fig_linewidth(wnd->laser_play_data[num].bg_fig, 1);
        rtk_fig_origin_global(wnd->laser_play_data[num].bg_fig, wnd->cfg->logfiles[num]->periods[h]->position_x, 
                                                                wnd->cfg->logfiles[num]->periods[h]->position_y, 
                                                                wnd->cfg->logfiles[num]->periods[h]->position_t);
    }

}

void update_laser_play_data(mainwnd_t *wnd)
{
    int i, h, miss;

    // Go through the log files, find the relevant periods, and
    // store them in the _period_hints array

    if (wnd->laser_play_data_n != wnd->cfg->num_logfiles)
    { // Time to resize and rebuild the per laser state array

        // Delete old 
        if (wnd->laser_play_data_n > 0)
        {
            for (i = 0; i < wnd->laser_play_data_n; i++)
            {
                rtk_fig_destroy(wnd->laser_play_data[i].laser_fig);
                rtk_fig_destroy(wnd->laser_play_data[i].data_fig);
                rtk_fig_destroy(wnd->laser_play_data[i].bg_fig);
            }
            free(wnd->laser_play_data); // Free the wnd->laser_play_data
        }

        if (wnd->cfg->num_logfiles > 0)
        {
            wnd->laser_play_data_n = wnd->cfg->num_logfiles;
            wnd->laser_play_data = (perlaserstate_t *)malloc(sizeof(perlaserstate_t) * wnd->cfg->num_logfiles);

            for (i = 0; i < wnd->laser_play_data_n; i++)
            {
                wnd->laser_play_data[i].laser_fig = rtk_fig_create_ex(wnd->canvas, NULL, -11, wnd);
                wnd->laser_play_data[i].data_fig = rtk_fig_create_ex(wnd->canvas, NULL, -12, wnd);
                wnd->laser_play_data[i].bg_fig = rtk_fig_create_ex(wnd->canvas, NULL, -13, wnd);
                wnd->laser_play_data[i].render_count = -1; // Next time though loop render
                
                h = log_cfg_find_period(wnd->cfg, i, wnd->current_time);
                wnd->laser_play_data[i].active_period = h;

                draw_laser_figure(wnd, i);
                draw_laser_data_figure(wnd, i);
                draw_laser_bg_figure(wnd, i);
            }
        }
        else
        {
            wnd->laser_play_data_n = 0;
            wnd->laser_play_data = NULL;
        }
    }
    else
    { // We have the right number of lasers, let's update them.
      // (We try to use the active period bit to save work)

        for (i = 0; i < wnd->laser_play_data_n; i++)
        {
            h = wnd->laser_play_data[i].active_period;
            if (h != -1)
            {
                if ((wnd->cfg->logfiles[i]->periods[h]->start_time <= wnd->current_time) && 
                    (wnd->cfg->logfiles[i]->periods[h]->end_time > wnd->current_time))
                {
                    miss = 0;
                }
                else
                {
                    h = log_cfg_find_period(wnd->cfg, i, wnd->current_time);
                    miss = 1;
                }
            }
            else
            {
                h = log_cfg_find_period(wnd->cfg, i, wnd->current_time);
                if (h != -1)
                    miss = 1;
                else
                    miss = 0;
            }

            if (miss)
            { // We need to reload
                wnd->laser_play_data[i].active_period = h;

                draw_laser_figure(wnd, i);
                draw_laser_data_figure(wnd, i);
                draw_laser_bg_figure(wnd, i);
            }
        }
    }
}

void read_log_files_to_time(mainwnd_t *wnd)
{
    int i, h;

    // Now go through each log file, and if there is an overlapping period, read until we get a timestamp past current_time
    for (i = 0; i < wnd->cfg->num_logfiles; i++)
    {
        h = wnd->laser_play_data[i].active_period;
        if (h != -1);
        {   
            if (wnd->cfg->logfiles[i]->lr != NULL)
                lr_read_to_time(wnd->cfg->logfiles[i]->lr, wnd->current_time);
        }
    }
}

void read_log_files_next_line(mainwnd_t *wnd)
{
    int i, h;
    double max_time;
    double min_time;

    // We read from each log file, and set the current time depending on
    // what we get.

    max_time = wnd->current_time - 1000.0;
    min_time = wnd->current_time + 1000.0;

    for (i = 0; i < wnd->cfg->num_logfiles; i++)
    {
        h = wnd->laser_play_data[i].active_period;
        if ((h != -1) && (wnd->cfg->logfiles[i]->lr != NULL))
        {
            lr_step(wnd->cfg->logfiles[i]->lr);

            max_time = MAX(max_time, lr_time_stamp_get(wnd->cfg->logfiles[i]->lr));
            min_time = MIN(min_time, lr_time_stamp_get(wnd->cfg->logfiles[i]->lr));
        }
    }


    if (wnd->play_stepping >  0)
    {
        wnd->current_time = min_time;
    }
    else if (wnd->play_stepping <  0)
    {
        wnd->current_time = max_time;
    }

    if (((max_time - min_time)) > TIME_WINDOW)
    {
        read_log_files_to_time(wnd);
    }
}

void setup_fluxlines(mainwnd_t *wnd)
{
#define FLUX_LINE_FACTOR_DIV (1.0/18.0)
    int i;
    flux_line_data_type *fl;

    for (i = 0; i < wnd->cfg->num_flux_lines; i ++)
    {
        fl = (void *)malloc(sizeof(flux_line_data_type));
        fl->n = rint(hypot(wnd->cfg->flux_lines[i]->x2 - wnd->cfg->flux_lines[i]->x1, 
                            wnd->cfg->flux_lines[i]->y2 - wnd->cfg->flux_lines[i]->y1)/ FLUX_LINE_FACTOR_DIV );
        fl->counts = (int *)malloc(sizeof(int) * fl->n);

        wnd->cfg->flux_lines[i]->user_data = fl;
    }

}

void process_flux_line(mainwnd_t *wnd)
{
    int i, j, k, l;
    flux_line_data_type *fl;
    double xd, yd, xi, yi;
    double X, Y;
    double u, v;
    double cs, sn;
    double angl, r;
    double line_length;
    int h;

    // Set each of the flux line values to unsupported
    for (i = 0; i < wnd->cfg->num_flux_lines; i ++)
    {
        fl = (flux_line_data_type *)wnd->cfg->flux_lines[i]->user_data;
        for (j = 0; j < fl->n; j++)
                fl->counts[j] = 0;
    }

    // Process each of the lasers
    for (i = 0; i < wnd->cfg->num_logfiles; i++)
    {
        cs = cos(wnd->laser_play_data[i].data_fig->oa);
        sn = sin(wnd->laser_play_data[i].data_fig->oa);

        h = wnd->laser_play_data[i].active_period;
        if (h != -1)
        {
            for (j = 0; j < LASER_ARRAY_LEN; j++)
            {
                angl = -M_PI_2	+ M_PI * ((j) / 180.0);
                r = lr_data_points_get(wnd->cfg->logfiles[i]->lr)[j];
                u = r*cos(angl);
                v = r*sin(angl);

                xi = wnd->laser_play_data[i].data_fig->ox;
                yi = wnd->laser_play_data[i].data_fig->oy;
                xd = cs * u - sn * v + xi;
                yd = sn * u + cs * v + yi;

                // Now, take the line (xi, yi) - (xd, yd) and intersect
                // with each of the flux lines
                
                for (k = 0; k < wnd->cfg->num_flux_lines; k++)
                {
                    fl = (flux_line_data_type *)wnd->cfg->flux_lines[k]->user_data;
                    l = line_intersection(wnd->cfg->flux_lines[k]->x1,
                                          wnd->cfg->flux_lines[k]->y1,
                                          wnd->cfg->flux_lines[k]->x2,
                                          wnd->cfg->flux_lines[k]->y2,
                                            xi, yi, xd, yd, &X, &Y);

                    if (l != DONT_INTERSECT)
                    { // Point of intersection is (X, Y)
                        line_length = hypot(wnd->cfg->flux_lines[k]->x1 - X, wnd->cfg->flux_lines[k]->y1 - Y);

                        h = (int)rint((line_length / wnd->cfg->flux_lines[k]->length) * fl->n);
                        if (h == fl->n) 
                            h = h-1;

                        fl->counts[h] = fl->counts[h] + 1;
                    } 
                }


            }

        }
    }

    if (wnd->fluxout_file != NULL)
    {
        static double last_time = -1.0;

        if (last_time != wnd->current_time) 
        {

            // Write the flux file 
            fprintf(wnd->fluxout_file,"%6.2lf\t",wnd->current_time);

            for (i = 0; i < wnd->cfg->num_flux_lines; i ++)
            {
                fl = (flux_line_data_type *)wnd->cfg->flux_lines[i]->user_data;
                l = 0;



                for (j = 0; j < fl->n; j++)
                    if (fl->counts[j] > 0)
                        l++;

                fprintf(wnd->fluxout_file,"\"%s\" %d\t", wnd->cfg->flux_lines[i]->name, l);
            }


            fprintf(wnd->fluxout_file,"\n");

            last_time = wnd->current_time;
        }
    }

    // This is set via a signal catcher. So that when you send "kill -signal USR1 <pid>"
    // the open output files are flushed.
    if (must_flush_to_disk)
    {
        if (wnd->output_file != NULL)
                fflush(wnd->output_file);
        if (wnd->fluxout_file != NULL)
                fflush(wnd->fluxout_file);
        fflush(stdout);
        fflush(stderr);

        must_flush_to_disk = 0;
    }
}


void move_cursor(mainwnd_t *wnd, double new_time, int jump)
{
    double ox, oy, oa;
    double x;
    double angl, r;
    int i, j, h;
    double points[LASER_ARRAY_LEN+1][2];
    //double X, Y, 
    double cs, sn;

    double X[360];
    double Y[360];
    int    Z[360];
    int    W[360];
    int N;


    cluster_set_t *c_set;
    GList *clist, *plist;
    cluster_t *cluster; 
    point_t *point;
    person_t *person;
    particle_t *part;

    ped_set_t *peds;
    pedestrian_t *pedestrian;
    GList *l;

    if (new_time < wnd->time_start) new_time = wnd->time_start; 
    if (new_time > wnd->time_end) new_time = wnd->time_end; 

    if ((jump) || (!rtk_menuitem_ischecked(wnd->line_based_item)))
    {
        /* Do the cursor rendering */
        wnd->current_time = new_time;
        x = TIME_TO_TL_SPACE(wnd, wnd->current_time);

        rtk_fig_get_origin(wnd->timeline_cursor_fig, &ox, &oy, &oa);
        if (ABS(ox - x) > CURSOR_DIFF)
        {
            rtk_fig_origin_global(wnd->timeline_cursor_fig, x, 0.0, 0.0);
            rtk_fig_clear(wnd->timeline_cursor_pos_fig);
        }

        update_laser_play_data(wnd); // Get the figures and period cache updated

        // We've set a desired time, and asked to read up to it.
        read_log_files_to_time(wnd);
    }
    else
    {
        if (wnd->current_time == new_time)
        { // We don't fetch new data, but do the rest

            update_laser_play_data(wnd); // Get the figures and period cache updated

            x = TIME_TO_TL_SPACE(wnd, wnd->current_time);

            rtk_fig_get_origin(wnd->timeline_cursor_fig, &ox, &oy, &oa);
            if (ABS(ox - x) > CURSOR_DIFF)
            {
                rtk_fig_origin_global(wnd->timeline_cursor_fig, x, 0.0, 0.0);
                rtk_fig_clear(wnd->timeline_cursor_pos_fig);
            }

        }
        else
        {
            // Note that we ignore the new_time, since we're just going to do
            // a read by line.
            update_laser_play_data(wnd); // Get the figures and period cache updated

            read_log_files_next_line(wnd);

            // Here we must set the x to react to the given current_time,
            // rather than the other way.

            x = TIME_TO_TL_SPACE(wnd, wnd->current_time);

            rtk_fig_get_origin(wnd->timeline_cursor_fig, &ox, &oy, &oa);
            if (ABS(ox - x) > CURSOR_DIFF)
            {
                rtk_fig_origin_global(wnd->timeline_cursor_fig, x, 0.0, 0.0);
                rtk_fig_clear(wnd->timeline_cursor_pos_fig);
            }
        }
    }


    N = 0;
    /* Okay, now do the laser rendering  */
    for (i = 0; i < wnd->cfg->num_logfiles; i++)
    {
        cs = cos(wnd->laser_play_data[i].data_fig->oa);
        sn = sin(wnd->laser_play_data[i].data_fig->oa);

        h = wnd->laser_play_data[i].active_period;
        if (h != -1)
        {
            wnd->laser_play_data[i].render_count = (wnd->laser_play_data[i].render_count + 1) % (wnd->render_rate);

            if ((wnd->processing) || (wnd->laser_play_data[i].render_count == 0))
            {
                for (j = 0; j < LASER_ARRAY_LEN; j++)
                {
                    angl = -M_PI_2	+ M_PI * ((j) / 180.0);
                    r = lr_data_points_get(wnd->cfg->logfiles[i]->lr)[j];
                    points[j][0] = r*cos(angl);
                    points[j][1] = r*sin(angl);

                    if (int_list_find_item(wnd->cfg->logfiles[i]->periods[h]->background_lists[j], r) == NULL)
                    {
                        static int first = 1;

                        if (first)
                        {
                            //wnd->cfg->logfiles[i]->periods[h]->background_lists[15]->head->start_value = 4.0;
                            //wnd->cfg->logfiles[i]->periods[h]->background_lists[15]->head->start_value = 8.1170;
                            //wnd->cfg->logfiles[i]->periods[h]->background_lists[15]->head->end_value = 9.0;
                            //wnd->cfg->logfiles[i]->periods[h]->background_lists[15]->head->end_value = 8.1360;
                            //first = 0;
                        }

                        X[N] = cs * points[j][0] - sn * points[j][1] + wnd->laser_play_data[i].data_fig->ox;
                        Y[N] = sn * points[j][0] + cs * points[j][1] + wnd->laser_play_data[i].data_fig->oy;
                        Z[N] = j;
                        W[N] = i;
                        N++;

                    }
                }

                points[LASER_ARRAY_LEN][0] = 0.0;
                points[LASER_ARRAY_LEN][1] = 0.0;


                if (wnd->laser_play_data[i].render_count == 0)
                {
                    if ((rtk_menuitem_ischecked(wnd->show_laser_fill_item)) || (rtk_menuitem_ischecked(wnd->show_laser_outline_item)))
                        rtk_fig_clear(wnd->laser_play_data[i].data_fig);

                    if (rtk_menuitem_ischecked(wnd->show_laser_fill_item))
                    {
                        rtk_fig_color_rgb32(wnd->laser_play_data[i].data_fig, COLOUR_LASER_RANGES_BG);
                        rtk_fig_polygon(wnd->laser_play_data[i].data_fig, 0.0, 0.0, 0.0, 180, points, 1);
                    }

                    if (rtk_menuitem_ischecked(wnd->show_laser_outline_item))
                    {
                        rtk_fig_color_rgb32(wnd->laser_play_data[i].data_fig, COLOUR_LASER_RANGES_FG);
                        rtk_fig_polygon(wnd->laser_play_data[i].data_fig, 0.0, 0.0, 0.0, 180, points, 0);
                    }


                    if (rtk_menuitem_ischecked(wnd->show_fg_item))
                    {
                        if (i == 0)
                        {
                            rtk_fig_clear(wnd->foreground_fig);
                            rtk_fig_color_rgb32(wnd->foreground_fig, COLOUR_LASER_FG);
                        }
                        for (j = 0; j < N; j++)
                            rtk_fig_rectangle(wnd->foreground_fig, X[j], Y[j], 0.0, 0.075, 0.075, 1);
                    }

                }
            }
        }

    }

    if (rtk_menuitem_ischecked(wnd->show_fluxlines_item))
    {

        rtk_fig_clear(wnd->flux_visualization_fig);
        rtk_fig_color_rgb32(wnd->flux_visualization_fig, COLOUR_FLUXLINE);

        for (i = 0; i < wnd->cfg->num_flux_lines; i++)
        {
            rtk_fig_line(wnd->flux_visualization_fig, 
                        wnd->cfg->flux_lines[i]->x1,
                        wnd->cfg->flux_lines[i]->y1,
                        wnd->cfg->flux_lines[i]->x2,
                        wnd->cfg->flux_lines[i]->y2);

            rtk_fig_ellipse(wnd->flux_visualization_fig, 
                                0.05 * wnd->cfg->flux_lines[i]->x1 + 0.95 * wnd->cfg->flux_lines[i]->x2,
                                0.05 * wnd->cfg->flux_lines[i]->y1 + 0.95 * wnd->cfg->flux_lines[i]->y2,
                                0, 0.05, 0.05, 1);

            flux_line_data_type * fl = (flux_line_data_type *)wnd->cfg->flux_lines[i]->user_data;

            if (fl != NULL)
            {
                for (j = 0; j < fl->n; j++)
                {
                    if (fl->counts[j])
                    {
                        float r = ((float)j + 0.5) / ((float)fl->n);

                        rtk_fig_ellipse(wnd->flux_visualization_fig, 
                                (1.0 - r) * wnd->cfg->flux_lines[i]->x1 + (r) * wnd->cfg->flux_lines[i]->x2,
                                (1.0 - r) * wnd->cfg->flux_lines[i]->y1 + (r) * wnd->cfg->flux_lines[i]->y2,
                                0, 0.08, 0.08, 0);
                    }
                }
            }

        }

    }
    else
    {
        rtk_fig_clear(wnd->flux_visualization_fig);
    }

    if ((rtk_menuitem_ischecked(wnd->show_clusters_item)) || 
            (rtk_menuitem_ischecked(wnd->show_tracking_pos_item)) ||
            (rtk_menuitem_ischecked(wnd->show_tracking_part_item)) )
    {

        rtk_fig_clear(wnd->tracking_visualization_fig);


    
        if (rtk_menuitem_ischecked(wnd->show_clusters_item))
        {
            rtk_fig_color_rgb32(wnd->tracking_visualization_fig, COLOUR_LASER_CLUSTER);

            if (wnd->tr_mod != NULL)
            { 
                c_set = wnd->tr_mod->clustering->cluster_set[wnd->tr_mod->clustering->current_cluster_set];
                clist = c_set->cluster_list;
                while (clist != NULL)
                {
                    cluster = clist->data;

                    plist = cluster->points_list;
                    //printf("Cluster had %d items\n", g_list_length(plist));

                    while (plist != NULL)
                    {
                        point = plist->data;

                        rtk_fig_line(wnd->tracking_visualization_fig, 
                                cluster->mean_X, cluster->mean_Y, 
                                point->X, point->Y); 

                        plist = plist->next;
                    }

                    rtk_fig_ellipse(wnd->tracking_visualization_fig, 
                            cluster->mean_X, cluster->mean_Y, 0, 0.1, 0.1, 1);


                    clist = clist->next;
                }
            }

        }

        if (rtk_menuitem_ischecked(wnd->show_tracking_part_item))
        {
            rtk_fig_color_rgb32(wnd->tracking_visualization_fig, COLOUR_LASER_TR_PART);
            if (wnd->tr_mod != NULL)
            {
                for (i = wnd->tr_mod->tracking->start_peds_index; i < wnd->tr_mod->tracking->num_ppl; i++)
                {

                    if (wnd->tr_mod->tracking->people[i].life > 0)
                    {
                        for (j = 0; j < NUM_PARTICLES; j++)
                        {
                            part = &(wnd->tr_mod->tracking->people[i].particles[j]);

                            if (part != NULL)
                            {

                                rtk_fig_color_rgb32(wnd->tracking_visualization_fig, COLOUR_LASER_TR_PART);
                                rtk_fig_line(wnd->tracking_visualization_fig, 
                                        part->x[0], part->y[0], 
                                        part->x[1], part->y[1]); 

                                rtk_fig_color_rgb32(wnd->tracking_visualization_fig, COLOUR_LASER_TR_PART1);
                                rtk_fig_ellipse(wnd->tracking_visualization_fig, 
                                        part->x[0], part->y[0], 0, 0.02, 0.02, 0);

                                rtk_fig_color_rgb32(wnd->tracking_visualization_fig, COLOUR_LASER_TR_PART2);
                                rtk_fig_ellipse(wnd->tracking_visualization_fig, 
                                        part->x[1], part->y[1], 0, 0.02, 0.02, 0);
                            }
                        }
                    }

                }
            }
        }

        if (rtk_menuitem_ischecked(wnd->show_tracking_pos_item))
        {
            if (wnd->tr_mod != NULL)
            {

                peds = temp_corr_calc_peds_get(wnd->tr_mod->temp_corr_calc);
                for (i = wnd->tr_mod->tracking->start_peds_index; i < wnd->tr_mod->tracking->num_ppl; i++)
                {
                    person = &(wnd->tr_mod->tracking->people[i]);

                    pedestrian = g_list_nth_data(peds->ped_list,i);

                    if (person->updated == 1) // if person is being updated by new clusters only then display
                    {
                        rtk_fig_color_rgb32(wnd->tracking_visualization_fig, COLOUR_LASER_TR_POS);
                    }
                    else
                    {
                        rtk_fig_color_rgb32(wnd->tracking_visualization_fig, COLOUR_LASER_TR_POS_G);
                    }


                    if (person->life > 0)  // if a person is deleted then do not show cluster to person associations
                    {
                        l = pedestrian->cluster_list;
                        while (l != NULL)
                        {
                            cluster = (cluster_t *)l->data;

                            rtk_fig_line(wnd->tracking_visualization_fig, 
                                    person->est_X, person->est_Y, 
                                    cluster->mean_X, cluster->mean_Y); 

                            l = l->next;
                        }

                        rtk_fig_ellipse(wnd->tracking_visualization_fig, person->est_X, person->est_Y, 0, 0.1, 0.1, 1);

                    }


                }

            }
        }


    }
    else
    {
        rtk_fig_clear(wnd->tracking_visualization_fig);
    }



    if ((wnd->fluxout_file != NULL) && (rtk_menuitem_ischecked(wnd->do_flux_item)))
    {
        process_flux_line(wnd);

    }

    if (wnd->processing)
    {

        if (wnd->auto_process)
        {
            int end_of_laser = 0;

            for (i = 0; i < wnd->laser_play_data_n; i++)
            {
                h = wnd->laser_play_data[i].active_period;
                if (h != -1)
                {
                    if ((wnd->cfg->logfiles[i]->periods[h]->start_time <= wnd->current_time) && 
                            (wnd->cfg->logfiles[i]->periods[h]->end_time > wnd->current_time))
                    {
                        //NOP
                    }
                    else
                    {
                        h = log_cfg_find_period(wnd->cfg, i, wnd->current_time);
                        if (h != -1)
                        {
                            // previous was the cached hint
                            if ((wnd->cfg->logfiles[i]->periods[h]->start_time <= wnd->current_time) && 
                                    (wnd->cfg->logfiles[i]->periods[h]->end_time > wnd->current_time))
                            {
                                //NOP
                            }
                            else
                            {
                                end_of_laser = 1;
                                break;
                            }
                        }
                        else
                        {
                            end_of_laser = 1;
                            break;

                        }
                    }
                }
                else
                {
                    end_of_laser = 1;
                    break;
                }
            }

            if (end_of_laser) 
            {
                gtk_widget_destroy(wnd->canvas->frame);
                return;
            }
        }



        //printf("[main_wnd.c] processing new data.\n");
        process_data(wnd, X, Y, N);
    }

    if ((wnd->send_matlab_status) && 
        (wnd->current_time - wnd->last_status_message > 0.1))
    {
        FILE *fifo;
        wnd->last_status_message = wnd->current_time;
        fifo = fopen(MATLAB_COMMS_FILE, "a");
        fprintf(fifo,  "sim_time = %6.6f; process = 1;\n", wnd->current_time);
        fclose(fifo);
    } 

}




void laser_mouse_ev(struct _rtk_fig_t *fig, int event, int mode)
{
    mainwnd_t *wnd;
    rtk_fig_t *laser_fig;
    int i, h;
    int num_log;

    assert(fig != NULL);

    // The *fig is the appropriate wnd->laser_play_data[i].laser_fig
    // The *fig->userdata is the  wnd

    laser_fig = (rtk_fig_t *)fig;
    wnd = (mainwnd_t *)laser_fig->userdata;

    // Okay, figure out which i we have:
    num_log = -1;
    for (i = 0; i < wnd->cfg->num_logfiles; i++)
        if (wnd->laser_play_data[i].laser_fig == fig)
            num_log = i;

    if (num_log < 0) return;

    h = wnd->laser_play_data[num_log].active_period;

    if (h < 0) return;

    rtk_fig_origin_global(wnd->laser_play_data[num_log].data_fig, fig->ox, fig->oy, fig->oa);

    wnd->cfg->logfiles[num_log]->periods[h]->position_x = fig->ox;
    wnd->cfg->logfiles[num_log]->periods[h]->position_y = fig->oy;
    wnd->cfg->logfiles[num_log]->periods[h]->position_t = fig->oa;
}

void write_output(FILE *f, double time, tracking_mod_t *tr_mod)
{
    int i, count;
    person_t person;
    static double last_time = -1.0;

    if (last_time == time) return; // Request a duplicate time
    last_time = time;

    count = 0;
    for (i = tr_mod->tracking->start_peds_index; i < tr_mod->tracking->num_ppl; i++)
    {
        person = tr_mod->tracking->people[i];
        if (person.life > 0)
        {
               count++; 
        }
    }
    if (count == 0) return;

    fprintf(f, "%6.6f\t%d\t", time, count);
    for (i = tr_mod->tracking->start_peds_index; i < tr_mod->tracking->num_ppl; i++)
    {
        person = tr_mod->tracking->people[i];
        if (person.life > 0)
        {
            fprintf(f, "%d\t%6.6f\t%6.6f\t%d\t", i, person.est_X, person.est_Y, person.life);
        }
    }

    fprintf(f,"\n");

}

void process_data(mainwnd_t *wnd, double *X, double *Y, int N)
{
    //int i;
    assert(wnd != NULL);

    //printf("[main_wnd.c|%f] %d\n",wnd->current_time, N);

    tracking_mod_process_new_data(wnd->current_time, wnd->tr_mod, X, Y, N);

    if (wnd->output_file != NULL)
        write_output(wnd->output_file, wnd->current_time, wnd->tr_mod);

}



