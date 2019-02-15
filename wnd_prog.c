/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: wnd_prog.c                               Date: 29 Jul. 2007    **
** Author: Dylan A. Shell                                               **
** Notes:                                                               **
*************************************************************************/

#include "wnd_prog.h"

GtkWindow *wnd_progress_dialog_create(char *title, char *label)
{
    GtkWidget *window, *fbox, *vbox, *prog, *lab;

    window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_decorated(GTK_WINDOW(window), 0);
    gtk_window_set_modal(GTK_WINDOW(window), 1);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    fbox = gtk_frame_new(title);
    gtk_container_add(GTK_CONTAINER(window), fbox);
    gtk_container_set_border_width(GTK_CONTAINER(fbox), 2);
    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

    gtk_window_set_default_size(GTK_WINDOW(window), 350, 50);

    gtk_container_add(GTK_CONTAINER(fbox), vbox);
    prog = gtk_progress_bar_new();
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(prog), 0.0);
    lab = gtk_label_new(label);
    gtk_container_add(GTK_CONTAINER(vbox), lab);
    gtk_container_add(GTK_CONTAINER(vbox), prog);

    gtk_widget_show_all(window);

    g_object_set_data(G_OBJECT(window), "progress_widget", (gpointer) prog);

    return GTK_WINDOW(window);
}


void wnd_progress_dialog_destroy(GtkWindow *win)
{
    gtk_widget_destroy(GTK_WIDGET(win));
}


void wnd_progress_dialog_set_progress(GtkWindow *win, float fract)
{
    GtkWidget *prog;
    char str[200];

    prog = GTK_WIDGET(g_object_get_data(G_OBJECT(win), "progress_widget"));

    sprintf(str, "%2.0f%%", 100.0*fract);

    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(prog), str);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(prog), MIN(fract, 1.0));

    // Process all waiting events
    while (gtk_events_pending())
        gtk_main_iteration();
}

