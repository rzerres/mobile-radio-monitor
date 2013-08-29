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
#include <gtk/gtk.h>

#include "mrm-window.h"

struct _MrmWindowPrivate {
    gpointer dummy;
};

G_DEFINE_TYPE_WITH_PRIVATE (MrmWindow, mrm_window, GTK_TYPE_APPLICATION_WINDOW)

/******************************************************************************/

static void
error_dialog (const gchar *primary_text,
              const gchar *secondary_text,
              GtkWindow *parent)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (parent,
                                     0,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_OK,
                                     NULL);
	if (parent)
		gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

	g_object_set (dialog,
                  "text", primary_text,
                  "secondary-text", secondary_text,
                  NULL);
	g_signal_connect (dialog,
                      "response",
                      G_CALLBACK (gtk_widget_destroy),
                      NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	gtk_widget_show (GTK_WIDGET (dialog));
}

/******************************************************************************/

void
mrm_window_open (MrmWindow *self,
                 GFile *device_file)
{
    error_dialog ("File doesn't exist", NULL, GTK_WINDOW (self));
}

/******************************************************************************/

GtkWidget *
mrm_window_new (MrmApp *application)
{
    MrmWindow *self;

    self = g_object_new (MRM_TYPE_WINDOW,
                         "application", application,
                         NULL);

    return GTK_WIDGET (self);
}

static void
mrm_window_init (MrmWindow *self)
{
    self->priv = mrm_window_get_instance_private (self);

    gtk_widget_init_template (GTK_WIDGET (self));
}

static void
mrm_window_class_init (MrmWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    /* Bind class to template */
    gtk_widget_class_set_template_from_resource  (widget_class, "/es/aleksander/mrm/mrm-window.ui");
}
