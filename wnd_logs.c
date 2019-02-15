/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: wnd_logs.c                               Date: 11 Jul. 2007    **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#include <assert.h>
#include <string.h>
#include "wnd_logs.h"

void _wnd_log_mouse_ev(struct _rtk_fig_t *fig, int event, int mode);
void _wnd_log_popup_menu(GtkWidget * my_widget, GdkEventButton * event, rtk_fig_t * log_fig);

void _wnd_log_edit(GtkMenuItem * menuitem, gpointer user_data);
void _wnd_log_delete(GtkMenuItem * menuitem, gpointer data);
void _wnd_log_edit_dlg_destroy(GtkDialog * dialog, gint arg1, gpointer user_data);

void _wnd_log_period_edit_dlg_destroy(GtkDialog * dialog, gint arg1, gpointer user_data);

void _wnd_period_edit(GtkMenuItem * menuitem, gpointer user_data);

void _wnd_period_split(GtkMenuItem * menuitem, gpointer user_data);
void _wnd_period_join_next(GtkMenuItem * menuitem, gpointer user_data);
void _wnd_period_join_previous(GtkMenuItem * menuitem, gpointer user_data); 

void _wnd_period_init_timeset(GtkButton *widget, gpointer user_data);
void _wnd_period_final_timeset(GtkButton *widget, gpointer user_data);


void wnd_log_add_log_figures(mainwnd_t * wnd)
{
    int i, j;
    double x, xx, y;

    assert(wnd != NULL);
    assert(wnd->timeline_log_figs == NULL);

    wnd->timeline_log_figs =
        (rtk_fig_t **) malloc(sizeof(rtk_fig_t *) * wnd->cfg->num_logfiles);

    // Lay down the underlying lines
    for (i = 0; i < wnd->cfg->num_logfiles; i++)
    {
        wnd->timeline_log_figs[i] = rtk_fig_create_ex(wnd->timeline, NULL, -70, wnd);
        wnd->cfg->logfiles[i]->user_data = wnd->timeline_log_figs[i];
        rtk_fig_color_rgb32(wnd->timeline_log_figs[i], COLOUR_TL_LINE);

        if (wnd->cfg->num_logfiles > 1)
            y = -((float)wnd->cfg->num_logfiles)*0.25*(((float)i)/((float)wnd->cfg->num_logfiles-1) - 0.5);
        else
            y = 0.0;

        rtk_fig_line(wnd->timeline_log_figs[i], - wnd->timeline_length/2.0, y,
                    wnd->timeline_length/2.0, y);

    }

    // Now for each log, add the periods
    for (i = 0; i < wnd->cfg->num_logfiles; i++)
    {
        for (j = 0; j < wnd->cfg->logfiles[i]->num_periods; j++)
        {
            if (wnd->cfg->logfiles[i]->periods[j]->active)
                rtk_fig_color_rgb32(wnd->timeline_log_figs[i], COLOUR_PRD_ACTIVE);
            else
                rtk_fig_color_rgb32(wnd->timeline_log_figs[i], COLOUR_PRD_INACTIVE);

            assert(wnd->cfg->logfiles[i]->periods[j]->start_time > wnd->time_start);
            assert(wnd->cfg->logfiles[i]->periods[j]->end_time < wnd->time_end);

            x = TIME_TO_TL_SPACE(wnd, wnd->cfg->logfiles[i]->periods[j]->start_time);


            xx = TIME_TO_TL_SPACE(wnd,wnd->cfg->logfiles[i]->periods[j]->end_time);

            if (wnd->cfg->num_logfiles > 1)
                y = -((float)wnd->cfg->num_logfiles)*0.25*(((float)i)/((float)wnd->cfg->num_logfiles-1) - 0.5);
            else
                y = 0.0;

            rtk_fig_rectangle(wnd->timeline_log_figs[i], 0.5*(x+xx), y, 0.0, (xx-x), PERIOD_WIDTH, 1);

        }

        for (j = 0; j < wnd->cfg->logfiles[i]->num_periods; j++)
        {
            if ((j + 1  < wnd->cfg->logfiles[i]->num_periods))
            {
                rtk_fig_color_rgb32(wnd->timeline_log_figs[i], COLOUR_TL_PERIODSPLIT);

                xx = TIME_TO_TL_SPACE(wnd,wnd->cfg->logfiles[i]->periods[j]->end_time);

                if (wnd->cfg->num_logfiles > 1)
                    y = -((float)wnd->cfg->num_logfiles)*0.25*(((float)i)/((float)wnd->cfg->num_logfiles-1) - 0.5);
                else
                    y = 0.0;


                rtk_fig_line(wnd->timeline_log_figs[i], xx, y-0.50*PERIOD_WIDTH, xx, y+0.50*PERIOD_WIDTH);
            }
        }


        rtk_fig_movemask(wnd->timeline_log_figs[i], RTK_REGISTER_ALL_MOUSE_CLICKS);

        rtk_fig_add_mouse_handler(wnd->timeline_log_figs[i], _wnd_log_mouse_ev);

    }

    redraw_lasers(wnd);

}

void wnd_log_del_log_figures(mainwnd_t * wnd)
{
    int i;
    assert(wnd != NULL);
    assert(wnd->timeline_log_figs != NULL);

    for (i = 0; i < wnd->cfg->num_logfiles; i++)
    {
        //rtk_fig_clear(wnd->timeline_log_figs[i]);
        rtk_fig_destroy(wnd->timeline_log_figs[i]);
    }
    free(wnd->timeline_log_figs);

    wnd->timeline_log_figs = NULL;
}


void _wnd_log_mouse_ev(struct _rtk_fig_t *fig, int event, int mode)
{
    int num_log, i;

    mainwnd_t *wnd = (mainwnd_t *)fig->userdata;

    num_log  = -1;
    for (i = 0; i < wnd->cfg->num_logfiles; i++)
        if (wnd->timeline_log_figs[i] == fig)
            num_log = i;
    if (num_log < 0)
        return;

    if ((event == RTK_EVENT_RELEASE) && (mode == 6))
    {
        _wnd_log_popup_menu(wnd->timeline->canvas, NULL,
                wnd->timeline_log_figs[num_log]);
    }
    else if (event == RTK_EVENT_MOUSE_OVER)
    {
        // Are there little laser boxes drawn?
        if (wnd->laser_play_data[num_log].active_period != -1)
        { // Okay, then we find redraw the lasers, plus a highlight.
            draw_laser_figure(wnd, num_log);

            rtk_fig_color_rgb32(wnd->laser_play_data[num_log].laser_fig, COLOUR_LASER_HIGH);
            rtk_fig_rectangle(wnd->laser_play_data[num_log].laser_fig, 0.0, 0.0, 0.0, 0.42, 0.42, 0);

        }
    }
    else if (event == RTK_EVENT_MOUSE_NOT_OVER)
    {
        // Are there little laser boxes drawn?
        if (wnd->laser_play_data[num_log].active_period != -1)
        { // Okay, then we find redraw the lasers, plus a highlight.
            draw_laser_figure(wnd, num_log);
        }
    }

}

/* Check which period in the given log figure the mouse is over.
 * returns -1 if none*/
int _wnd_log_over_period(mainwnd_t * wnd, rtk_fig_t * log_fig, int num_log, int mouse_x, int mouse_y)
{
    double dx, dy;
    double x, xx, y;
    double x1, y1, x2 ,y2;
    int j;


    // Ensure the mouse is within the figure
    if (mouse_x < log_fig->region->rect.x) return -1;
    if (mouse_x > log_fig->region->rect.x + log_fig->region->rect.width) return -1;
    if (mouse_y < log_fig->region->rect.y) return -1;
    if (mouse_y > log_fig->region->rect.y + log_fig->region->rect.height) return -1;

    dx = rtk_canvas_dev_to_logical_x(wnd->timeline, mouse_x);
    dy = rtk_canvas_dev_to_logical_y(wnd->timeline, mouse_y);

    for (j = 0; j < wnd->cfg->logfiles[num_log]->num_periods; j++)
    {
        x = TIME_TO_TL_SPACE(wnd, wnd->cfg->logfiles[num_log]->periods[j]->start_time);
        xx = TIME_TO_TL_SPACE(wnd,wnd->cfg->logfiles[num_log]->periods[j]->end_time);

        if (wnd->cfg->num_logfiles > 1)
            y = -((float)wnd->cfg->num_logfiles)*0.25*(((float)num_log)/((float)wnd->cfg->num_logfiles-1) - 0.5);
        else
            y = 0.0;

        x1 = MIN(x, xx);
        x2 = MAX(x, xx);
        y1 = MIN(y - PERIOD_WIDTH/2.0, y + PERIOD_WIDTH/2.0);
        y2 = MAX(y - PERIOD_WIDTH/2.0, y + PERIOD_WIDTH/2.0);

        if ((dx >= x1) && (dx <= x2) && (dy >= y1) && (dy <= y2))
        {
            return j;
        }
    }

    

    return -1;
}

void _wnd_log_popup_menu(GtkWidget * my_widget, GdkEventButton * event, rtk_fig_t * log_fig)
{
    GtkWidget *menu, *submenu;
    GtkWidget *menu_item;
    int button, event_time, period_over;
    int j, num_log;
    mainwnd_t *wnd;

    wnd = (mainwnd_t *)log_fig->userdata;

    menu = gtk_menu_new();

    num_log = -1;
    for (j = 0; j < wnd->cfg->num_logfiles; j++)
        if (wnd->timeline_log_figs[j] == log_fig)
            num_log = j;
    if (num_log < 0)
        return;

    // Did we click over a period, or just on the log file line?
    period_over = _wnd_log_over_period(wnd, log_fig, num_log, wnd->timeline->mouse_x, wnd->timeline->mouse_y);

    g_object_set_data(G_OBJECT(wnd->timeline->canvas), "active_period", (gpointer)period_over);

    menu_item = gtk_menu_item_new_with_mnemonic("_Edit Period");
    gtk_menu_append(GTK_MENU(menu), menu_item);
    g_signal_connect(GTK_OBJECT(menu_item), "activate",
            GTK_SIGNAL_FUNC(_wnd_period_edit), log_fig);
    gtk_widget_set_sensitive(menu_item, (period_over != -1));

    menu_item = gtk_menu_item_new_with_mnemonic("_Split");
    gtk_menu_append(GTK_MENU(menu), menu_item);
    g_signal_connect(GTK_OBJECT(menu_item), "activate",
            GTK_SIGNAL_FUNC(_wnd_period_split), log_fig);
    gtk_widget_set_sensitive(menu_item, (period_over != -1));

    menu_item = gtk_menu_item_new_with_mnemonic("Join");
    gtk_menu_append(GTK_MENU(menu), menu_item);
    gtk_widget_set_sensitive(menu_item, (period_over != -1));

    submenu =  gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), submenu);

    menu_item = gtk_menu_item_new_with_mnemonic("Preceding Period");
    gtk_menu_append(GTK_MENU(submenu), menu_item);
    g_signal_connect(GTK_OBJECT(menu_item), "activate",
            GTK_SIGNAL_FUNC(_wnd_period_join_previous), log_fig);
    gtk_widget_set_sensitive(menu_item, (period_over > 0));

    menu_item = gtk_menu_item_new_with_mnemonic("Succeeding Period");
    gtk_menu_append(GTK_MENU(submenu), menu_item);
    g_signal_connect(GTK_OBJECT(menu_item), "activate",
            GTK_SIGNAL_FUNC(_wnd_period_join_next), log_fig);
    gtk_widget_set_sensitive(menu_item, 
            (period_over < wnd->cfg->logfiles[num_log]->num_periods - 1));

    gtk_menu_append(GTK_MENU(menu), gtk_separator_menu_item_new());

    menu_item = gtk_menu_item_new_with_mnemonic("Edit _Log");
    gtk_menu_append(GTK_MENU(menu), menu_item);
    g_signal_connect(GTK_OBJECT(menu_item), "activate",
            GTK_SIGNAL_FUNC(_wnd_log_edit), log_fig);
    menu_item = gtk_menu_item_new_with_mnemonic("_Remove Log");
    gtk_menu_append(GTK_MENU(menu), menu_item);
    gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
            GTK_SIGNAL_FUNC(_wnd_log_delete), log_fig);

    gtk_widget_show_all(menu);

    if (event)
    {
        button = event->button;
        event_time = event->time;
    }
    else
    {
        button = 0;
        event_time = gtk_get_current_event_time();
    }

    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
            button, event_time);
}

void _wnd_log_delete(GtkMenuItem * menuitem, gpointer data)
{
    int i, num_log, ret;
    rtk_fig_t *log_fig;
    mainwnd_t *wnd;
    log_fig = (rtk_fig_t *) data;
    wnd = (mainwnd_t *)log_fig->userdata;
    GtkWidget *dialog;

    num_log  = -1;
    for (i = 0; i < wnd->cfg->num_logfiles; i++)
        if (wnd->timeline_log_figs[i] == log_fig)
            num_log = i;
    if (num_log < 0)
        return;

    dialog = gtk_message_dialog_new(NULL,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_YES_NO,
                                    "You are about to irreversibly remove a logfile from the configuration and delete all associated periods. The logfile itself will remain on disk.\nAre you sure sure you wish to proceed?");
    ret = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (ret == GTK_RESPONSE_YES)
    {
        // Delete figures
        wnd_log_del_log_figures(wnd);

        // Now, let's remove this log
        log_cfg_delete_logfile(wnd->cfg, num_log);

        // Update display of annotations
        wnd_log_add_log_figures(wnd);
    }
}


void _wnd_log_edit(GtkMenuItem * menuitem, gpointer user_data)
{
    rtk_fig_t *log_fig;
    mainwnd_t *wnd;
    int num_log, i;

    log_fig = (rtk_fig_t *)user_data;
    wnd = (mainwnd_t *)log_fig->userdata;

    num_log  = -1;
    for (i = 0; i < wnd->cfg->num_logfiles; i++)
        if (wnd->timeline_log_figs[i] == log_fig)
            num_log = i;
    if (num_log < 0)
        return;

    wnd_log_edit_dlg_create(wnd, log_fig, num_log);
}

void wnd_log_edit_dlg_create(mainwnd_t * wnd, rtk_fig_t *log_fig, int num_log)
{
    GtkWidget *dialog, *label, *textview, *nameentry, *fileentry;
    GtkWidget *layout, *sublayout, *scrl;
    GtkTextBuffer *buffer;

    char tempstr[200];

    assert(wnd != NULL);
    assert(log_fig != NULL);


    /* Create the widgets */
    if (wnd->cfg->logfiles[num_log]->filename == NULL)
        strcpy(tempstr, "New Logfile");
    else
        strcpy(tempstr, "Edit Logfile");

    dialog = gtk_dialog_new_with_buttons(tempstr, NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 375, 250);

    layout = gtk_vbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), layout);

    sublayout = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(layout), sublayout);
    gtk_box_set_child_packing(GTK_BOX(layout), sublayout, FALSE, FALSE, 4, GTK_PACK_START);

        label = gtk_label_new("Name:");
        gtk_misc_set_alignment(GTK_MISC(label),0, 0);
        gtk_widget_set_size_request(label, 50, 20);
        gtk_container_add(GTK_CONTAINER(sublayout), label);
        gtk_box_set_child_packing(GTK_BOX(sublayout), label, FALSE, FALSE, 4, GTK_PACK_START);

        nameentry = gtk_entry_new();
        if (wnd->cfg->logfiles[num_log]->name != NULL)
            gtk_entry_set_text(GTK_ENTRY(nameentry), wnd->cfg->logfiles[num_log]->name);
        gtk_container_add(GTK_CONTAINER(sublayout), nameentry);
        gtk_box_set_child_packing(GTK_BOX(sublayout), nameentry, TRUE, TRUE, 4, GTK_PACK_START);


    sublayout = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(layout), sublayout);
    gtk_box_set_child_packing(GTK_BOX(layout), sublayout, FALSE, FALSE, 4, GTK_PACK_START);


        label = gtk_label_new("File:");
        gtk_misc_set_alignment(GTK_MISC(label),0, 0);
        gtk_widget_set_size_request(label, 50, 20);
        gtk_container_add(GTK_CONTAINER(sublayout), label);
        gtk_box_set_child_packing(GTK_BOX(sublayout), label, FALSE, FALSE, 4, GTK_PACK_START);


        fileentry = gtk_file_chooser_button_new("Select a logfile\n", GTK_FILE_CHOOSER_ACTION_OPEN);
        if (wnd->cfg->logfiles[num_log]->filename != NULL)
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fileentry), wnd->cfg->logfiles[num_log]->filename);
        gtk_container_add(GTK_CONTAINER(sublayout), fileentry);
        gtk_box_set_child_packing(GTK_BOX(sublayout), fileentry, TRUE, TRUE, 4, GTK_PACK_START);


    sublayout = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(layout), sublayout);
    gtk_box_set_child_packing(GTK_BOX(layout), sublayout, TRUE, TRUE, 4, GTK_PACK_START);


        label = gtk_label_new("Note:");
        gtk_misc_set_alignment(GTK_MISC(label),0, 0);
        gtk_widget_set_size_request(label, 50, 20);
        gtk_container_add(GTK_CONTAINER(sublayout), label);
        gtk_box_set_child_packing(GTK_BOX(sublayout), label, FALSE, FALSE, 4, GTK_PACK_START);


        scrl = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrl), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(sublayout), scrl);
        gtk_box_set_child_packing(GTK_BOX(sublayout), scrl, TRUE, TRUE, 4, GTK_PACK_START);

    textview = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(scrl), textview);

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    if (wnd->cfg->logfiles[num_log]->notes == NULL)
        gtk_text_buffer_set_text(buffer, "", -1);
    else
        gtk_text_buffer_set_text(buffer, wnd->cfg->logfiles[num_log]->notes, -1);



    /* Ensure that the dialog box is destroyed when the user responds. */
    g_signal_connect_swapped(dialog, "response", ((GCallback)_wnd_log_edit_dlg_destroy), dialog);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_widget_show_all(dialog);

    g_object_set_data(G_OBJECT(dialog), "wnd", wnd);
    g_object_set_data(G_OBJECT(dialog), "num_log", (gpointer)num_log);
    g_object_set_data(G_OBJECT(dialog), "name", nameentry);
    g_object_set_data(G_OBJECT(dialog), "filename", fileentry);
    g_object_set_data(G_OBJECT(dialog), "buffer", buffer);

    if (wnd->cfg->logfiles[num_log]->filename == NULL)
        g_object_set_data(G_OBJECT(dialog), "was_new", (gpointer)TRUE);
    else
        g_object_set_data(G_OBJECT(dialog), "was_new", (gpointer)FALSE);
}


void _wnd_log_edit_dlg_destroy(GtkDialog * dialog, gint arg1, gpointer user_data)
{
    mainwnd_t * wnd;
    GtkFileChooser *fileentry;
    GtkEntry *nameentry;
    GtkTextBuffer *buffer;
    int num_log;
    GtkTextIter start, end;
    char *p;
    int was_new;

    wnd = (mainwnd_t *)g_object_get_data(G_OBJECT(dialog), "wnd");
    num_log = (int)g_object_get_data(G_OBJECT(dialog), "num_log");
    was_new = (int)g_object_get_data(G_OBJECT(dialog), "was_new");

    if (arg1 == GTK_RESPONSE_OK)
    {
        nameentry = (GtkEntry *)g_object_get_data(G_OBJECT(dialog), "name");
        buffer = (GtkTextBuffer *)g_object_get_data(G_OBJECT(dialog), "buffer");
        fileentry = (GtkFileChooser *)g_object_get_data(G_OBJECT(dialog), "filename");

        if (wnd->cfg->logfiles[num_log]->name != NULL)
            free(wnd->cfg->logfiles[num_log]->name);

        wnd->cfg->logfiles[num_log]->name = strdup(gtk_entry_get_text(nameentry));

        if (wnd->cfg->logfiles[num_log]->filename != NULL)
            free(wnd->cfg->logfiles[num_log]->filename);

        p = gtk_file_chooser_get_filename(fileentry);
        log_cfg_logfile_update_filename(wnd->cfg, num_log, p);

        gtk_text_buffer_get_bounds(buffer, &start, &end);
        p = gtk_text_iter_get_text(&start, &end);

        if (wnd->cfg->logfiles[num_log]->notes != NULL)
            free(wnd->cfg->logfiles[num_log]->notes);

        wnd->cfg->logfiles[num_log]->notes = p;
        //g_free(p);
        
        // Delete figures
        wnd_log_del_log_figures(wnd);

        log_cfg_times_get(wnd->cfg, &wnd->time_start, &wnd->time_end); 
        wnd->time_start -= 2.0;
        wnd->time_end += 2.0;
        // For each unit of time, give SPACE_UNITS of area
        wnd->timeline_length = (wnd->time_end - wnd->time_start)*SPACE_UNITS;

        rtk_fig_clear(wnd->timeline_bg_fig);
        rtk_fig_color_rgb32(wnd->timeline_bg_fig, COLOUR_TL_BG);
        rtk_fig_rectangle(wnd->timeline_bg_fig, 0.0, 0.0, 0.0, wnd->timeline_length, TIME_LINE_HEIGHT, 1);



        // Update display of logfiles
        wnd_log_add_log_figures(wnd);
    }
    else
    {
        if (was_new)
        {
            // Delete figures
            wnd_log_del_log_figures(wnd);

            // Now, let's remove this log
            log_cfg_delete_logfile(wnd->cfg, num_log);

            // Update display of logfiles
            wnd_log_add_log_figures(wnd);
        }

    }
    
    gtk_widget_destroy(GTK_WIDGET(dialog));
 
}


void _wnd_period_edit(GtkMenuItem * menuitem, gpointer user_data)
{
    rtk_fig_t *log_fig;
    mainwnd_t *wnd;
    int num_log;
    int i;

    log_fig = (rtk_fig_t *)user_data;
    wnd = (mainwnd_t *)log_fig->userdata;

    num_log  = -1;
    for (i = 0; i < wnd->cfg->num_logfiles; i++)
        if (wnd->timeline_log_figs[i] == log_fig)
            num_log = i;
    if (num_log < 0)
        return;


    wnd_log_edit_period_dlg_create(wnd, log_fig, num_log,  (int)g_object_get_data(G_OBJECT(wnd->timeline->canvas), "active_period"));
}

void _wnd_period_join_previous(GtkMenuItem * menuitem, gpointer user_data)
{
    mainwnd_t *wnd;
    rtk_fig_t *log_fig;
    int period;
    int num_log;
    int i;

    log_fig = (rtk_fig_t *)user_data;
    wnd = (mainwnd_t *)log_fig->userdata;

    num_log  = -1;
    for (i = 0; i < wnd->cfg->num_logfiles; i++)
        if (wnd->timeline_log_figs[i] == log_fig)
            num_log = i;
    if (num_log < 0)
        return;

    // Delete figures
    wnd_log_del_log_figures(wnd);

    //The number of periods will change, so we invalidate the period hint cache
    flush_hints(wnd);

    // Now merge 'em
    period = (int)g_object_get_data(G_OBJECT(wnd->timeline->canvas), "active_period");
    log_cfg_merge_period_with_next(wnd->cfg, num_log, period-1);

    // Update display of annotations
    wnd_log_add_log_figures(wnd);

}

void _wnd_period_join_next(GtkMenuItem * menuitem, gpointer user_data)
{
    mainwnd_t *wnd;
    rtk_fig_t *log_fig;
    int period;
    int num_log;
    int i;

    log_fig = (rtk_fig_t *)user_data;
    wnd = (mainwnd_t *)log_fig->userdata;

    num_log  = -1;
    for (i = 0; i < wnd->cfg->num_logfiles; i++)
        if (wnd->timeline_log_figs[i] == log_fig)
            num_log = i;
    if (num_log < 0)
        return;

    // Delete figures
    wnd_log_del_log_figures(wnd);

    //The number of periods will change, so we invalidate the period hint cache
    flush_hints(wnd);

    // Now merge 'em
    period = (int)g_object_get_data(G_OBJECT(wnd->timeline->canvas), "active_period");
    log_cfg_merge_period_with_next(wnd->cfg, num_log, period);

    // Update display of annotations
    wnd_log_add_log_figures(wnd);

}

void wnd_log_edit_period_dlg_create(mainwnd_t * wnd, rtk_fig_t *log_fig, 
                                    int num_log, int num_period)

{
    GtkWidget *dialog, *label, *textview, *timeentry;
    GtkWidget *layout, *sublayout, *scrl, *but;;
    GtkTextBuffer *buffer;
    char tempstr[200];

    assert(wnd != NULL);
    assert(log_fig != NULL);

    /* Create the widgets */
    dialog = gtk_dialog_new_with_buttons("Edit period", NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

    g_object_set_data(G_OBJECT(dialog), "wnd", wnd);
    g_object_set_data(G_OBJECT(dialog), "num_log", (gpointer)num_log);
    g_object_set_data(G_OBJECT(dialog), "num_period", (gpointer)num_period);


    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 415, 190);

    layout = gtk_vbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), layout);

    sublayout = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(layout), sublayout);
    gtk_box_set_child_packing(GTK_BOX(layout), sublayout, FALSE, FALSE, 4, GTK_PACK_START);

        label = gtk_label_new("Laser Position:");
        gtk_misc_set_alignment(GTK_MISC(label),0, 0);
        gtk_widget_set_size_request(label, 105, 10);
        gtk_container_add(GTK_CONTAINER(sublayout), label);
        gtk_box_set_child_packing(GTK_BOX(sublayout), label, FALSE, FALSE, 4, GTK_PACK_START);

        sprintf(tempstr,"%2.2fm, %2.2fm, %2.2f", wnd->cfg->logfiles[num_log]->periods[num_period]->position_x, wnd->cfg->logfiles[num_log]->periods[num_period]->position_y, wnd->cfg->logfiles[num_log]->periods[num_period]->position_t);

        label = gtk_label_new(tempstr);
        gtk_misc_set_alignment(GTK_MISC(label),0, 0);
        gtk_container_add(GTK_CONTAINER(sublayout), label);
        gtk_box_set_child_packing(GTK_BOX(sublayout), label, TRUE, TRUE, 4, GTK_PACK_START);

        but = gtk_check_button_new_with_label("Active");
        gtk_container_add(GTK_CONTAINER(sublayout), but);
        gtk_box_set_child_packing(GTK_BOX(sublayout), but, FALSE, FALSE, 6, GTK_PACK_START);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(but), wnd->cfg->logfiles[num_log]->periods[num_period]->active);
        g_object_set_data(G_OBJECT(dialog), "active-button", but);


    sublayout = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(layout), sublayout);
    gtk_box_set_child_packing(GTK_BOX(layout), sublayout, FALSE, FALSE, 4, GTK_PACK_START);

        label = gtk_label_new("Start Time:");
        gtk_misc_set_alignment(GTK_MISC(label),0, 0);
        gtk_widget_set_size_request(label, 75, 10);
        gtk_container_add(GTK_CONTAINER(sublayout), label);
        gtk_box_set_child_packing(GTK_BOX(sublayout), label, FALSE, FALSE, 4, GTK_PACK_START);

        timeentry = gtk_entry_new();
        sprintf(tempstr,"%6.2lf", wnd->cfg->logfiles[num_log]->periods[num_period]->start_time);
        gtk_entry_set_text(GTK_ENTRY(timeentry), tempstr);
        gtk_container_add(GTK_CONTAINER(sublayout), timeentry);
        gtk_box_set_child_packing(GTK_BOX(sublayout), timeentry, TRUE, TRUE, 4, GTK_PACK_START);
        gtk_widget_set_size_request(timeentry, 175, 20);

        g_object_set_data(G_OBJECT(dialog), "start-timeentry", timeentry);

        but = gtk_button_new_with_label(" Set to cursor ");
        gtk_container_add(GTK_CONTAINER(sublayout), but);
        gtk_box_set_child_packing(GTK_BOX(sublayout), but, FALSE, FALSE, 6, GTK_PACK_START);
        g_signal_connect(GTK_OBJECT(but), "clicked", GTK_SIGNAL_FUNC(_wnd_period_init_timeset), dialog);

    sublayout = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(layout), sublayout);
    gtk_box_set_child_packing(GTK_BOX(layout), sublayout, FALSE, FALSE, 4, GTK_PACK_START);

        label = gtk_label_new("End Time:");
        gtk_misc_set_alignment(GTK_MISC(label),0, 0);
        gtk_widget_set_size_request(label, 75, 10);
        gtk_container_add(GTK_CONTAINER(sublayout), label);
        gtk_box_set_child_packing(GTK_BOX(sublayout), label, FALSE, FALSE, 4, GTK_PACK_START);

        timeentry = gtk_entry_new();
        sprintf(tempstr,"%6.2lf", wnd->cfg->logfiles[num_log]->periods[num_period]->end_time);
        gtk_entry_set_text(GTK_ENTRY(timeentry), tempstr);
        gtk_container_add(GTK_CONTAINER(sublayout), timeentry);
        gtk_box_set_child_packing(GTK_BOX(sublayout), timeentry, TRUE, TRUE, 4, GTK_PACK_START);
        gtk_widget_set_size_request(timeentry, 175, 20);

        g_object_set_data(G_OBJECT(dialog), "end-timeentry", timeentry);

        but = gtk_button_new_with_label(" Set to cursor ");
        gtk_container_add(GTK_CONTAINER(sublayout), but);
        gtk_box_set_child_packing(GTK_BOX(sublayout), but, FALSE, FALSE, 6, GTK_PACK_START);
        g_signal_connect(GTK_OBJECT(but), "clicked", GTK_SIGNAL_FUNC(_wnd_period_final_timeset), dialog);


    sublayout = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(layout), sublayout);
    gtk_box_set_child_packing(GTK_BOX(layout), sublayout, TRUE, TRUE, 4, GTK_PACK_START);


        label = gtk_label_new("Note:");
        gtk_misc_set_alignment(GTK_MISC(label),0, 0);
        gtk_widget_set_size_request(label, 50, 20);
        gtk_container_add(GTK_CONTAINER(sublayout), label);
        gtk_box_set_child_packing(GTK_BOX(sublayout), label, FALSE, FALSE, 4, GTK_PACK_START);


        scrl = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrl), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(sublayout), scrl);
        gtk_box_set_child_packing(GTK_BOX(sublayout), scrl, TRUE, TRUE, 4, GTK_PACK_START);

        textview = gtk_text_view_new();
        gtk_container_add(GTK_CONTAINER(scrl), textview);

        buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
        if (wnd->cfg->logfiles[num_log]->periods[num_period]->note == NULL)
            gtk_text_buffer_set_text(buffer, "", -1);
        else
            gtk_text_buffer_set_text(buffer, wnd->cfg->logfiles[num_log]->periods[num_period]->note, -1);

    /* Ensure that the dialog box is destroyed when the user responds. */
    g_signal_connect_swapped(dialog, "response", ((GCallback)_wnd_log_period_edit_dlg_destroy), dialog);
                                                            
    g_object_set_data(G_OBJECT(dialog), "buffer", buffer);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_widget_show_all(dialog);

}


void _wnd_log_period_edit_dlg_destroy(GtkDialog * dialog, gint arg1, gpointer user_data)
{
    GtkEntry *starttimeentry;
    GtkEntry *endtimeentry;
    GtkWidget *activebut;
    GtkTextBuffer *buffer;
    mainwnd_t *wnd;
    int num_log, num_period;
    const char *c; 
    double new_time;
    char *p;
    GtkTextIter start, end;

    wnd = (mainwnd_t *)g_object_get_data(G_OBJECT(dialog), "wnd");
    num_log = (int)g_object_get_data(G_OBJECT(dialog), "num_log");
    num_period = (int)g_object_get_data(G_OBJECT(dialog), "num_period");
    starttimeentry = GTK_ENTRY(g_object_get_data(G_OBJECT(dialog), "start-timeentry"));
    endtimeentry = GTK_ENTRY(g_object_get_data(G_OBJECT(dialog), "end-timeentry"));
    activebut = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "active-button"));
    buffer = (GtkTextBuffer *)g_object_get_data(G_OBJECT(dialog), "buffer");

    if (arg1 == GTK_RESPONSE_OK)
    {
        // Delete figures
        wnd_log_del_log_figures(wnd);

        c = gtk_entry_get_text(starttimeentry);
        new_time = strtod(c, &p);
        if (*p == '\0')
        {                           // We got a valid time
            wnd->cfg->logfiles[num_log]->periods[num_period]->start_time = new_time;
        }

        c = gtk_entry_get_text(endtimeentry);
        new_time = strtod(c, &p);
        if (*p == '\0')
        {                           // We got a valid time
            wnd->cfg->logfiles[num_log]->periods[num_period]->end_time = new_time;
        }

        wnd->cfg->logfiles[num_log]->periods[num_period]->active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(activebut));

        gtk_text_buffer_get_bounds(buffer, &start, &end);
        p = gtk_text_iter_get_text(&start, &end);

        if (wnd->cfg->logfiles[num_log]->periods[num_period]->note != NULL)
            free(wnd->cfg->logfiles[num_log]->periods[num_period]->note);

        wnd->cfg->logfiles[num_log]->periods[num_period]->note = p;

        //The number of periods will change, so we invalidate the period hint cache
        flush_hints(wnd);

        // Update display of annotations
        wnd_log_add_log_figures(wnd);
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}


void _wnd_period_init_timeset(GtkButton *widget, gpointer user_data)
{
    GtkWidget *dialog;
    GtkWidget *timeentry;
    char tempstr[200];
    mainwnd_t *wnd;

    dialog = GTK_WIDGET(user_data);
    timeentry = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "start-timeentry"));

    wnd = (mainwnd_t *)g_object_get_data(G_OBJECT(dialog), "wnd");
    
    // Set to the time at the cursor
    sprintf(tempstr,"%6.2lf", wnd->current_time);

    gtk_entry_set_text(GTK_ENTRY(timeentry), tempstr);
}

void _wnd_period_final_timeset(GtkButton *widget, gpointer user_data)
{
    GtkWidget *dialog;
    GtkWidget *timeentry;
    char tempstr[200];
    mainwnd_t *wnd;


    dialog = GTK_WIDGET(user_data);
    timeentry = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "end-timeentry"));

    wnd = (mainwnd_t *)g_object_get_data(G_OBJECT(dialog), "wnd");
    
    // Set to the time at the cursor
    sprintf(tempstr,"%6.2lf", wnd->current_time);

    gtk_entry_set_text(GTK_ENTRY(timeentry), tempstr);
}


void _wnd_period_split(GtkMenuItem * menuitem, gpointer user_data)
{
    rtk_fig_t *log_fig;
    mainwnd_t *wnd;
    int num_log, i;
    int active_period;

    log_fig = (rtk_fig_t *)user_data;
    wnd = (mainwnd_t *)log_fig->userdata;

    num_log  = -1;
    for (i = 0; i < wnd->cfg->num_logfiles; i++)
        if (wnd->timeline_log_figs[i] == log_fig)
            num_log = i;
    if (num_log < 0)
        return;

    wnd = (mainwnd_t *)log_fig->userdata;

    active_period = (int)g_object_get_data(G_OBJECT(wnd->timeline->canvas), "active_period");

    // Delete figures
    wnd_log_del_log_figures(wnd);

    //The number of periods will change, so we invalidate the period hint cache
    flush_hints(wnd);

    log_cfg_split_period(wnd->cfg, num_log, active_period, wnd->current_time);

    // Update display of annotations
    wnd_log_add_log_figures(wnd);
}


