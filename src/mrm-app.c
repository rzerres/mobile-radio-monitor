/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
 */

#include <config.h>

#include "mrm-app.h"
#include "mrm-window.h"

G_DEFINE_TYPE (MrmApp, mrm_app, GTK_TYPE_APPLICATION)

struct _MrmAppPrivate {
    gpointer unused;
};

/******************************************************************************/
/* Main application window management */

static GtkWidget *
peek_main_window (MrmApp *self)
{
    GList *l;

    /* Remove all windows registered in the application */
    l = gtk_application_get_windows (GTK_APPLICATION (self));
    g_assert_cmpuint (g_list_length (l), <=, 1);

    return (l ? GTK_WIDGET (l->data) : NULL);
}

static void
ensure_main_window (MrmApp *self)
{
    GtkWidget *window;

    if (peek_main_window (self))
        return;

    window = mrm_window_new (self);
    gtk_application_add_window (GTK_APPLICATION (self), GTK_WINDOW (window));
    gtk_window_maximize (GTK_WINDOW (window));
    gtk_widget_show_all (window);
}

void
mrm_app_quit (MrmApp *self)
{
    g_action_group_activate_action (G_ACTION_GROUP (self), "quit", NULL);
}

/******************************************************************************/
/* Application actions setup */

static void
about_cb (GSimpleAction *action,
          GVariant *parameter,
          gpointer user_data)
{
    MrmApp *self = MRM_APP (user_data);
    const gchar *authors[] = {
        "Aleksander Morgado <aleksander@gnu.org>",
        NULL
    };

    gtk_show_about_dialog (GTK_WINDOW (peek_main_window (self)),
                           "name",     "Mobile Radio Monitor",
                           "version",  PACKAGE_VERSION,
                           "comments", "A monitor for mobile radio environment parameters",
                           "authors",  authors,
                           NULL);
}

static void
quit_cb (GSimpleAction *action,
         GVariant *parameter,
         gpointer user_data)
{
    MrmApp *self = MRM_APP (user_data);
    GList *l;

    /* Remove all windows registered in the application */
    while ((l = gtk_application_get_windows (GTK_APPLICATION (self))))
        gtk_application_remove_window (GTK_APPLICATION (self), GTK_WINDOW (l->data));
}

static GActionEntry app_entries[] = {
    { "about", about_cb, NULL, NULL, NULL },
    { "quit",  quit_cb,  NULL, NULL, NULL },
};

/******************************************************************************/

static void
activate (GApplication *application)
{
    ensure_main_window (MRM_APP (application));
}

/******************************************************************************/

static void
open (GApplication *application,
      GFile **files,
      gint n_files,
      const gchar *hint)
{
    GtkWidget *window;
    gchar *path;
    gchar *display;

    /* No files to open, just return */
    if (n_files <= 0)
        return;

    /* More than one file; warn */
    if (n_files > 1) {
        guint i;

        g_warning ("Too many device files given (%d); only one expected.", n_files);
        for (i = 1; i < n_files; i++) {
            path = g_file_get_path (files[i]);
            display = g_filename_display_name (path);
            g_warning ("  Ignoring device file '%s'...", display);
            g_free (path);
            g_free (display);
        }
    }

    /* Exactly one file, open it */
    path = g_file_get_path (files[0]);
    display = g_filename_display_name (path);
    g_debug ("Opening device file '%s'...", display);
    g_free (path);
    g_free (display);

    /* Peek window and open device file */
    ensure_main_window (MRM_APP (application));
    window = peek_main_window (MRM_APP (application));
    mrm_window_open (MRM_WINDOW (window), files[0]);
}

/******************************************************************************/

static void
startup (GApplication *application)
{
    MrmApp *self = MRM_APP (application);
    GtkBuilder *builder;
    GError *error = NULL;

    /* Chain up parent's startup */
    G_APPLICATION_CLASS (mrm_app_parent_class)->startup (application);

    /* Setup actions */
    g_action_map_add_action_entries (G_ACTION_MAP (self),
                                     app_entries, G_N_ELEMENTS (app_entries),
                                     self);

    /* Setup menu */
    builder = gtk_builder_new ();
    if (!gtk_builder_add_from_resource (builder,
                                        "/es/aleksander/mrm/mrm-menu.ui",
                                        &error)) {
        g_warning ("loading menu builder file: %s", error->message);
        g_error_free (error);
    } else {
        GMenuModel *app_menu;

        app_menu = G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu"));
        gtk_application_set_app_menu (GTK_APPLICATION (application), app_menu);
    }

    g_object_unref (builder);
}

/******************************************************************************/

MrmApp *
mrm_app_new (void)
{
    return g_object_new (MRM_TYPE_APP,
                         "application-id",   "es.aleksander.MobileRadioMonitor",
                         "flags",            G_APPLICATION_HANDLES_OPEN,
                         "register-session", TRUE,
                         NULL);
}

static void
mrm_app_init (MrmApp *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MRM_TYPE_APP, MrmAppPrivate);

    g_set_application_name ("MobileRadioMonitor");
    gtk_window_set_default_icon_name ("mobile-radio-monitor");
}

static void
mrm_app_class_init (MrmAppClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (MrmAppPrivate));

    application_class->startup = startup;
    application_class->activate = activate;
    application_class->open = open;
}