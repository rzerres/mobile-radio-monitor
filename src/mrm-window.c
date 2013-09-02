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
#include <libqmi-glib.h>

#include "mrm-window.h"
#include "mrm-device.h"

#define NOTEBOOK_TAB_DEVICE_LIST 0
#define NOTEBOOK_TAB_SIGNAL_INFO 1

struct _MrmWindowPrivate {
    GtkWidget *notebook;

    /* Header bar */
    GtkWidget *back_button;

    /* Device list tab */
    GtkWidget *device_list_frame;
    GtkWidget *device_list_box;
    GtkWidget *device_list_label;
    GtkWidget *device_list_spinner_box;

    guint initial_scan_done_id;
    guint device_detection_id;
    guint device_added_id;
    guint device_removed_id;
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

static void
select_device_list_tab (MrmWindow *self)
{
    gtk_widget_set_sensitive (self->priv->back_button, FALSE);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (self->priv->notebook), NOTEBOOK_TAB_DEVICE_LIST);
}

static void
select_signal_info_tab (MrmWindow *self)
{
    gtk_widget_set_sensitive (self->priv->back_button, TRUE);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (self->priv->notebook), NOTEBOOK_TAB_SIGNAL_INFO);
}

/******************************************************************************/

void
mrm_window_open_device (MrmWindow *self,
                        MrmDevice *device)
{
    select_signal_info_tab (self);

    /* TODO: more */
}

/******************************************************************************/

#define DEVICE_TAG "device-tag"

static void
device_list_update_header (GtkListBoxRow  *row,
                           GtkListBoxRow  *before,
                           gpointer    user_data)
{
    GtkWidget *current;

    if (!before)
        return;

    current = gtk_list_box_row_get_header (row);
    if (!current) {
        current = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
        gtk_widget_show (current);
        gtk_list_box_row_set_header (row, current);
    }
}

static void
device_list_activate_row (MrmWindow *self,
                          GtkListBoxRow *row)
{
    mrm_window_open_device (self,
                            MRM_DEVICE (g_object_get_data (G_OBJECT (row), DEVICE_TAG)));
}

static void
device_added_cb (MrmApp *application,
                 MrmDevice *device,
                 MrmWindow *self)
{
    GtkWidget *row;
    GtkWidget *box;
    GtkWidget *button_label;
    gchar *button_label_markup;
    GtkWidget *status;

    row = gtk_list_box_row_new ();
    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 50);
    gtk_container_add (GTK_CONTAINER (row), box);
    gtk_widget_set_hexpand (box, TRUE);
    gtk_container_add (GTK_CONTAINER (self->priv->device_list_box), row);

    g_object_set_data_full (G_OBJECT (row),
                            DEVICE_TAG,
                            g_object_ref (device),
                            g_object_unref);

    button_label_markup = g_strdup_printf ("[%s]\n\t<span weight=\"bold\">%s</span>\n\t<span style=\"italic\">%s</span>",
                                           mrm_device_get_name (device),
                                           mrm_device_get_model (device),
                                           mrm_device_get_manufacturer (device));
    button_label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (button_label), button_label_markup);
    gtk_misc_set_alignment (GTK_MISC (button_label), 0.0f, 0.5f);
    gtk_widget_set_margin_left (button_label, 20);
    gtk_widget_set_margin_right (button_label, 20);
    gtk_widget_set_margin_top (button_label, 12);
    gtk_widget_set_margin_bottom (button_label, 12);
    gtk_widget_set_halign (button_label, GTK_ALIGN_START);
    gtk_widget_set_valign (button_label, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand (button_label, TRUE);
    gtk_box_pack_start (GTK_BOX (box), button_label, TRUE, TRUE, 0);

    status = gtk_label_new (NULL);
    gtk_widget_set_margin_left (status, 20);
    gtk_widget_set_margin_right (status, 20);
    gtk_widget_set_halign (status, GTK_ALIGN_END);
    gtk_widget_set_valign (status, GTK_ALIGN_CENTER);
    gtk_box_pack_end (GTK_BOX (box), status, FALSE, FALSE, 0);

    switch (mrm_device_get_status (device)) {
    case MRM_DEVICE_STATUS_UNKNOWN:
        gtk_label_set_text (GTK_LABEL (status), "Unknown status");
        gtk_widget_set_sensitive (row, FALSE);
        break;
    case MRM_DEVICE_STATUS_READY:
        gtk_label_set_text (GTK_LABEL (status), "Ready");
        break;
    case MRM_DEVICE_STATUS_SIM_PIN_LOCKED:
        gtk_label_set_text (GTK_LABEL (status), "PIN required");
        break;
    case MRM_DEVICE_STATUS_SIM_PUK_LOCKED:
        gtk_label_set_text (GTK_LABEL (status), "PUK required");
        gtk_widget_set_sensitive (row, FALSE);
        break;
    case MRM_DEVICE_STATUS_SIM_ERROR:
        gtk_label_set_text (GTK_LABEL (status), "SIM error");
        gtk_widget_set_sensitive (row, FALSE);
        break;
    }

    gtk_widget_show_all (row);

    gtk_widget_hide (self->priv->device_list_label);
    gtk_frame_set_shadow_type (GTK_FRAME (self->priv->device_list_frame), GTK_SHADOW_IN);
    gtk_widget_show (self->priv->device_list_frame);
}

static void
device_removed_cb (MrmApp *application,
                   MrmDevice *device,
                   MrmWindow *self)
{
    GList *children, *l;
    guint valid = 0;
    gboolean removed = FALSE;

    children = gtk_container_get_children (GTK_CONTAINER (self->priv->device_list_box));
    for (l = children; l; l = g_list_next (l)) {
        gpointer ldevice;

        ldevice = g_object_get_data (G_OBJECT (l->data), DEVICE_TAG);
        if (!ldevice)
            continue;

        if (device == ldevice ||
            g_str_equal (mrm_device_get_name (MRM_DEVICE (ldevice)),
                         mrm_device_get_name (device))) {
            gtk_container_remove (GTK_CONTAINER (self->priv->device_list_box), GTK_WIDGET (l->data));
            removed = TRUE;
        } else
            valid++;
    }

    /* If this was the last item; show label and hide list */
    if (removed && !valid) {
        gtk_widget_hide (self->priv->device_list_frame);
        gtk_widget_show (self->priv->device_list_label);
    }

    g_list_free (children);
}

static void
populate_device_list (MrmWindow *self)
{
    MrmApp *application = NULL;
    GList *l;

    g_object_get (self,
                  "application", &application,
                  NULL);

    self->priv->device_added_id =
        g_signal_connect (application,
                          "device-added",
                          G_CALLBACK (device_added_cb),
                          self);
    self->priv->device_removed_id =
        g_signal_connect (application,
                          "device-removed",
                          G_CALLBACK (device_removed_cb),
                          self);

    for (l = mrm_app_peek_devices (application); l; l = g_list_next (l))
        device_added_cb (application, MRM_DEVICE (l->data), self);

    g_object_unref (application);
}

static void
initial_scan_done_cb (MrmApp *application,
                      MrmWindow *self)
{
    populate_device_list (self);
}

static void
device_detection_cb (MrmApp *application,
                     gboolean detection,
                     MrmWindow *self)
{
    if (detection)
        gtk_widget_show (self->priv->device_list_spinner_box);
    else
        gtk_widget_hide (self->priv->device_list_spinner_box);
}

static void
setup_device_list_updates (MrmWindow *self)
{
    MrmApp *application = NULL;

    g_object_get (self,
                  "application", &application,
                  NULL);

    self->priv->device_detection_id =
        g_signal_connect (application,
                          "device-detection",
                          G_CALLBACK (device_detection_cb),
                          self);

    if (mrm_app_is_initial_scan_done (application))
        populate_device_list (self);
    else
        self->priv->initial_scan_done_id =
            g_signal_connect (application,
                              "initial-scan-done",
                              G_CALLBACK (initial_scan_done_cb),
                              self);

    g_object_unref (application);
}

/******************************************************************************/

static void
back_button_clicked (GtkButton *button,
                     MrmWindow *self)
{
    select_device_list_tab (self);
}

/******************************************************************************/

GtkWidget *
mrm_window_new (MrmApp *application)
{
    MrmWindow *self;

    self = g_object_new (MRM_TYPE_WINDOW,
                         "application", application,
                         NULL);

    setup_device_list_updates (self);

    return GTK_WIDGET (self);
}

static void
mrm_window_init (MrmWindow *self)
{
    self->priv = mrm_window_get_instance_private (self);

    gtk_widget_init_template (GTK_WIDGET (self));

    g_signal_connect (self->priv->back_button,
                      "clicked",
                      G_CALLBACK (back_button_clicked),
                      self);

    g_signal_connect_swapped (self->priv->device_list_box,
                              "row-activated",
                              G_CALLBACK (device_list_activate_row),
                              self);

    gtk_list_box_set_header_func (GTK_LIST_BOX (self->priv->device_list_box),
                                  device_list_update_header,
                                  NULL, NULL);

    gtk_widget_show (self->priv->device_list_label);
    gtk_frame_set_shadow_type (GTK_FRAME (self->priv->device_list_frame), GTK_SHADOW_NONE);
    gtk_widget_hide (self->priv->device_list_frame);
}

static void
dispose (GObject *object)
{
    MrmWindow *self = MRM_WINDOW (object);
    MrmApp *application = NULL;

    /* Remove signal handlers */
    g_object_get (self,
                  "application", &application,
                  NULL);
    if (application) {
        if (self->priv->initial_scan_done_id) {
            g_signal_handler_disconnect (application,
                                         self->priv->initial_scan_done_id);
            self->priv->initial_scan_done_id = 0;
        }
        if (self->priv->device_detection_id) {
            g_signal_handler_disconnect (application,
                                         self->priv->device_detection_id);
            self->priv->device_detection_id = 0;
        }
        if (self->priv->device_added_id) {
            g_signal_handler_disconnect (application,
                                         self->priv->device_added_id);
            self->priv->device_added_id = 0;
        }
        if (self->priv->device_removed_id) {
            g_signal_handler_disconnect (application,
                                         self->priv->device_removed_id);
            self->priv->device_removed_id = 0;
        }
        g_object_unref (application);
    }

    G_OBJECT_CLASS (mrm_window_parent_class)->dispose (object);
}

static void
mrm_window_class_init (MrmWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->dispose = dispose;

    /* Bind class to template */
    gtk_widget_class_set_template_from_resource  (widget_class, "/es/aleksander/mrm/mrm-window.ui");
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, notebook);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, back_button);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, device_list_frame);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, device_list_box);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, device_list_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, device_list_spinner_box);
}
