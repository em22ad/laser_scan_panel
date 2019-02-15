/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: wnd_annos.c                              Date: 11 Jul. 2007    **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#include "wnd_annos.h"
#include "utils.h"
#include <string.h>
#include <math.h>
#include <assert.h>

void _wnd_anno_mouse_ev(struct _rtk_fig_t *fig, int event, int mode);
void _wnd_anno_popup_menu(GtkWidget * my_widget, GdkEventButton * event,
                          rtk_fig_t * annot_fig);
void _wnd_anno_edit(GtkMenuItem * menuitem, gpointer user_data);
void _wnd_anno_delete(GtkMenuItem * menuitem, gpointer data);
void _wnd_anno_edit_dlg_destroy(GtkDialog * dialog, gint arg1, gpointer user_data);

void wnd_anno_add_annotation_figures(mainwnd_t * wnd)
{
    int i;
    double x;

    assert(wnd != NULL);
    assert(wnd->timeline_anno_figs == NULL);

    wnd->timeline_anno_figs =
        (rtk_fig_t **) malloc(sizeof(rtk_fig_t *) * wnd->cfg->num_annos);
    for (i = 0; i < wnd->cfg->num_annos; i++)
    {
        wnd->timeline_anno_figs[i] =
            rtk_fig_create_ex(wnd->timeline, NULL, -65, wnd);

        x = TIME_TO_TL_SPACE(wnd, wnd->cfg->anno[i]->time);
        rtk_fig_origin_global(wnd->timeline_anno_figs[i], x, 0.0, 0.0);

        if (wnd->cfg->anno[i]->shortnote)
        {
            rtk_fig_color_rgb32(wnd->timeline_anno_figs[i], COLOUR_TL_SH_ANNO);

            if (wnd->cfg->anno[i]->text == NULL)
                rtk_fig_text(wnd->timeline_anno_figs[i], 0.0, 0.0, 0.0,"");
            else
                rtk_fig_text(wnd->timeline_anno_figs[i], 0.0, 0.0, 0.0,
                        wnd->cfg->anno[i]->text);
        }
        else
        {
            if (rtk_menuitem_ischecked(wnd->show_anno_item))
            {
                rtk_fig_color_rgb32(wnd->timeline_anno_figs[i], COLOUR_TL_ANNO);

                rtk_fig_rectangle(wnd->timeline_anno_figs[i], 0.0, 0.0, 0.0,
                        ANNOT_WIDTH, TIME_LINE_HEIGHT, 1);
                if (rtk_menuitem_ischecked(wnd->lock_anno_item))
                    rtk_fig_movemask(wnd->timeline_anno_figs[i],
                            RTK_REGISTER_ALL_MOUSE_CLICKS);
                else
                    rtk_fig_movemask(wnd->timeline_anno_figs[i],
                            RTK_MOVE_TRANS_HORIZ |
                            RTK_REGISTER_ALL_MOUSE_CLICKS);
            }
            else
            {
                rtk_fig_movemask(wnd->timeline_anno_figs[i], 0);
            }

            rtk_fig_add_mouse_handler(wnd->timeline_anno_figs[i],
                    _wnd_anno_mouse_ev);
        }

        wnd->cfg->anno[i]->user_data = wnd->timeline_anno_figs[i];
    }
}

void wnd_anno_del_annotation_figures(mainwnd_t * wnd)
{
    int i;

    assert(wnd != NULL);
    assert(wnd->timeline_anno_figs != NULL);

    rtk_fig_clear(wnd->timeline_anno_sum_fig);
    // Remove the figure
    for (i = 0; i < wnd->cfg->num_annos; i++)
        rtk_fig_destroy(wnd->timeline_anno_figs[i]);
    free(wnd->timeline_anno_figs);
    wnd->timeline_anno_figs = NULL;
}

void _wnd_anno_mouse_ev(struct _rtk_fig_t *fig, int event, int mode)
{
    int i;
    int num_anno;
    double poly_points[7][2];
    int ix, iy;
    double dx, dy;
    int ox, oy;
    int l, w, h, n, split_w, split_h;
    int px_w, px_h;
    float pos_tail, temp_x;
    char *bubble_text;


    mainwnd_t *wnd = (mainwnd_t *)fig->userdata;

    num_anno = -1;
    for (i = 0; i < wnd->cfg->num_annos; i++)
        if (wnd->timeline_anno_figs[i] == fig)
            num_anno = i;
    if (num_anno < 0)
        return;

    wnd->cfg->anno[num_anno]->time = TL_SPACE_TO_TIME(wnd, fig->dox);

    if ((event == RTK_EVENT_MOUSE_OVER) || (event == RTK_EVENT_MOTION)
        || (event == RTK_EVENT_MOTION))
    {
        rtk_fig_clear(wnd->timeline_anno_sum_fig);

        split_w = 12;
        split_h = 55;

        ix = rtk_canvas_logical_to_dev_x(wnd->timeline,
                                         TIME_TO_TL_SPACE(wnd,
                                                          wnd->cfg->
                                                          anno[num_anno]->
                                                          time));
        dx = rtk_canvas_dev_to_logical_x(wnd->canvas, ix);
        iy = (wnd->canvas->canvas)->allocation.height - 2;
        dy = rtk_canvas_dev_to_logical_y(wnd->canvas, iy);

        // Okay, calculate the width and height of the popup.
        bubble_text = strdup(wnd->cfg->anno[num_anno]->text);
        l = strlen(bubble_text);
        n = (int)ceil(sqrt((0.15) * l));
        if (l > 40)
        {
            wordwrap(bubble_text, (l) / n);

            get_multi_line_text_extents(wnd->canvas->font, bubble_text,
                                        strlen(bubble_text), &px_w, &px_h);
        }
        else
        {
            gdk_text_extents(wnd->canvas->font, bubble_text, l,
                             NULL, NULL, &px_w, &px_h, NULL);

        }

        w = px_w + 50;
        h = px_h + 30;
        if (w < 200)
            w = 200;
        if (h < 100)
            h = 100;

        // Which third does the annotation line sit in:
        if (ix < (wnd->canvas->canvas)->allocation.width / 3)
        {
            ox = 10;
        }
        else if (ix >
                 (wnd->canvas->canvas)->allocation.width -
                 (wnd->canvas->canvas)->allocation.width / 3)
        {
            ox = (wnd->canvas->canvas)->allocation.width - w - 10;
        }
        else
        {
            ox = (wnd->canvas->canvas)->allocation.width / 2 - w / 2;
        }
        oy = (wnd->canvas->canvas)->allocation.height - split_h - h;

        temp_x =
            ox + w / 2.0 - (ox + w / 2.0 -
                            (float)ix) * ((float)h / 2.0) / ((float)h / 2.0 +
                                                             (float)split_h);
        pos_tail = ((float)(temp_x - ox)) / ((float)w);


        if (w * pos_tail < (10.0 + split_w))
            pos_tail = (10.0 + split_w) / ((float)w);
        if (w * pos_tail > w - (10.0 + split_w))
            pos_tail = ((float)w - (10.0 + split_w)) / ((float)w);

        poly_points[0][0] = ox + w * pos_tail - split_w;
        poly_points[0][1] = oy + h;

        poly_points[1][0] = ox;
        poly_points[1][1] = oy + h;

        poly_points[2][0] = ox;
        poly_points[2][1] = oy;

        poly_points[3][0] = ox + w;
        poly_points[3][1] = oy;

        poly_points[4][0] = ox + w;
        poly_points[4][1] = oy + h;

        poly_points[5][0] = ox + w * pos_tail + split_w;
        poly_points[5][1] = oy + h;

        poly_points[6][0] = ox + w * pos_tail;
        poly_points[6][1] = oy + h;


        for (i = 0; i < 7; i++)
        {
            poly_points[i][0] =
                rtk_canvas_dev_to_logical_x(wnd->canvas, poly_points[i][0]);
            poly_points[i][1] =
                rtk_canvas_dev_to_logical_y(wnd->canvas, poly_points[i][1]);
        }
        poly_points[6][0] = dx;
        poly_points[6][1] = dy;

        rtk_fig_color_rgb32(wnd->timeline_anno_sum_fig, COLOUR_ANNO_BG);
        rtk_fig_polygon(wnd->timeline_anno_sum_fig, 0, 0, 0., 7, poly_points, 1);

        rtk_fig_color_rgb32(wnd->timeline_anno_sum_fig, COLOUR_ANNO_FG);
        rtk_fig_polygon(wnd->timeline_anno_sum_fig, 0, 0, 0., 7, poly_points, 0);

        rtk_fig_color_rgb32(wnd->timeline_anno_sum_fig, COLOUR_ANNO_TXT);
        rtk_fig_text(wnd->timeline_anno_sum_fig,
                     rtk_canvas_dev_to_logical_x(wnd->canvas,
                                                 ox + w / 2 - px_w / 2),
                     rtk_canvas_dev_to_logical_y(wnd->canvas,
                                                 oy + h / 2 - px_h / 2 + 10),
                     0.0, bubble_text);

        free(bubble_text);

    }
    else if (event == RTK_EVENT_MOUSE_NOT_OVER)
    {
        rtk_fig_clear(wnd->timeline_anno_sum_fig);
    }
    else if ((event == RTK_EVENT_RELEASE) && (mode == 6))
    {
        if (!rtk_menuitem_ischecked(wnd->lock_anno_item))
            _wnd_anno_popup_menu(wnd->timeline->canvas, NULL,
                    wnd->timeline_anno_figs[num_anno]);
    }

}

void _wnd_anno_popup_menu(GtkWidget * my_widget, GdkEventButton * event,
                          rtk_fig_t * annot_fig)
{
    GtkWidget *menu;
    GtkWidget *menu_item;
    int button, event_time;
    mainwnd_t *wnd;

    wnd = (mainwnd_t *) annot_fig->userdata;

    menu = gtk_menu_new();

    menu_item = gtk_menu_item_new_with_mnemonic("_Edit Note");
    gtk_menu_append(GTK_MENU(menu), menu_item);
    g_signal_connect(GTK_OBJECT(menu_item), "activate",
                     GTK_SIGNAL_FUNC(_wnd_anno_edit), annot_fig);
    gtk_widget_show(menu_item);

    menu_item = gtk_menu_item_new_with_mnemonic("_Delete");
    gtk_menu_append(GTK_MENU(menu), menu_item);
    gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
                       GTK_SIGNAL_FUNC(_wnd_anno_delete), annot_fig);
    gtk_widget_show(menu_item);

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


void _wnd_anno_delete(GtkMenuItem * menuitem, gpointer data)
{
    int i, num_anno, ret;
    rtk_fig_t *annot_fig;
    mainwnd_t *wnd;
    annot_fig = (rtk_fig_t *) data;
    wnd = (mainwnd_t *) annot_fig->userdata;
    GtkWidget *dialog;


    num_anno = -1;
    for (i = 0; i < wnd->cfg->num_annos; i++)
        if (wnd->timeline_anno_figs[i] == annot_fig)
            num_anno = i;
    if (num_anno < 0)
        return;

    dialog = gtk_message_dialog_new(NULL,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_YES_NO,
                                    "You are about to irreverably delete an annotation. Are you sure sure you wish to proceed?");
    ret = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (ret == GTK_RESPONSE_YES)
    {
        // Delete outdated annotation figures
        wnd_anno_del_annotation_figures(wnd);

        // Now, let's remove this annotation.
        log_cfg_delete_annotation(wnd->cfg, num_anno);

        // Update display of annotations
        wnd_anno_add_annotation_figures(wnd);
    }

}


void _wnd_anno_edit(GtkMenuItem * menuitem, gpointer user_data)
{
    rtk_fig_t *annot_fig;
    mainwnd_t *wnd;
    annot_fig = (rtk_fig_t *)user_data;
    wnd = (mainwnd_t *) annot_fig->userdata;

    rtk_fig_clear(wnd->timeline_anno_sum_fig);
    wnd_anno_edit_dlg_create(wnd, annot_fig);

}

void wnd_anno_edit_dlg_create(mainwnd_t * wnd, rtk_fig_t * annot_fig)
{
    GtkWidget *dialog, *label, *textview, *entry;
    GtkWidget *layout, *sublayout, *scrl;
    GtkTextBuffer *buffer;

    char tempstr[200];
    int i, num_anno;

    assert(wnd != NULL);
    assert(annot_fig != NULL);

    num_anno = -1;
    for (i = 0; i < wnd->cfg->num_annos; i++)
        if (wnd->timeline_anno_figs[i] == annot_fig)
            num_anno = i;
    if (num_anno < 0)
        return;

    /* Create the widgets */
    if (wnd->cfg->anno[num_anno]->text == NULL)
        strcpy(tempstr, "New Annotation");
    else
        strcpy(tempstr, "Edit Annotation");


    dialog = gtk_dialog_new_with_buttons(tempstr, NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 475, 225);
    //gtk_window_set_default_size(GTK_WINDOW(dialog), 375, 125);

    layout = gtk_vbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), layout);

    sublayout = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(layout), sublayout);
    gtk_box_set_child_packing(GTK_BOX(layout), sublayout, FALSE, FALSE, 4, GTK_PACK_START);

    
        label = gtk_label_new("Time:");
        gtk_misc_set_alignment(GTK_MISC(label),0, 0);
        gtk_widget_set_size_request(label, 50, 20);
        gtk_container_add(GTK_CONTAINER(sublayout), label);
        gtk_box_set_child_packing(GTK_BOX(sublayout), label, FALSE, FALSE, 4, GTK_PACK_START);

        snprintf(tempstr, 200, "%.2lf", wnd->cfg->anno[num_anno]->time);
        entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry), tempstr);
        gtk_container_add(GTK_CONTAINER(sublayout), entry);
        gtk_box_set_child_packing(GTK_BOX(sublayout), entry, TRUE, TRUE, 4, GTK_PACK_START);


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
    if (wnd->cfg->anno[num_anno]->text == NULL)
        gtk_text_buffer_set_text(buffer, "", -1);
    else
        gtk_text_buffer_set_text(buffer, wnd->cfg->anno[num_anno]->text, -1);


    /* Ensure that the dialog box is destroyed when the user responds. */
    g_signal_connect_swapped(dialog, "response", ((GCallback)_wnd_anno_edit_dlg_destroy), dialog);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_widget_show_all(dialog);

    g_object_set_data(G_OBJECT(dialog), "wnd", wnd);
    g_object_set_data(G_OBJECT(dialog), "active_anno_num", (gpointer)num_anno);
    g_object_set_data(G_OBJECT(dialog), "buffer", buffer);
    g_object_set_data(G_OBJECT(dialog), "timeentry", entry);

    if (wnd->cfg->anno[num_anno]->text == NULL)
        g_object_set_data(G_OBJECT(dialog), "was_new", (gpointer)TRUE);
    else
        g_object_set_data(G_OBJECT(dialog), "was_new", (gpointer)FALSE);
}

void _wnd_anno_edit_dlg_destroy(GtkDialog * dialog, gint arg1, gpointer user_data)
{
    GtkEntry *entry;
    GtkTextBuffer *buffer;
    mainwnd_t * wnd;
    const char *c; 
    char *p;
    double new_time;
    GtkTextIter start, end;
    int num_anno;
    int was_new;


    wnd = (mainwnd_t *)g_object_get_data(G_OBJECT(dialog), "wnd");
    num_anno = (int)g_object_get_data(G_OBJECT(dialog), "active_anno_num");
    was_new = (int)g_object_get_data(G_OBJECT(dialog), "was_new");


    if (arg1 == GTK_RESPONSE_OK)
    {
        entry = (GtkEntry *) g_object_get_data(G_OBJECT(dialog), "timeentry");
        buffer = (GtkTextBuffer *) g_object_get_data(G_OBJECT(dialog), "buffer");

        c = gtk_entry_get_text(entry);
        new_time = strtod(c, &p);
        if (*p == '\0')
        {                           // We got a valid time
            wnd->cfg->anno[num_anno]->time = new_time;
        }

        gtk_text_buffer_get_bounds(buffer, &start, &end);
        p = gtk_text_iter_get_text(&start, &end);
        if (wnd->cfg->anno[num_anno]->text != NULL)
            free(wnd->cfg->anno[num_anno]->text);

        wnd->cfg->anno[num_anno]->text = p;
        //g_free(p);
    }
    else
    {
        if (was_new)
        {
            // We need to remove the num_anno.

            // Delete outdated annotation figures
            wnd_anno_del_annotation_figures(wnd);

            // Now, let's remove this annotation.
            log_cfg_delete_annotation(wnd->cfg, num_anno);

            // Update display of annotations
            wnd_anno_add_annotation_figures(wnd);
        }
    }


    gtk_widget_destroy(GTK_WIDGET(dialog));

    // Delete outdated annotation figures
    wnd_anno_del_annotation_figures(wnd);
    // Update display of annotations
    wnd_anno_add_annotation_figures(wnd);

}

