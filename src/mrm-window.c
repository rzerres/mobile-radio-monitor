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
 * Copyright (C) 2013-2015 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>
#include <gtk/gtk.h>
#include <libqmi-glib.h>

#include "mrm-window.h"
#include "mrm-device.h"
#include "mrm-signal-tab.h"
#include "mrm-power-tab.h"

#define NOTEBOOK_TAB_DEVICE_LIST 0
#define NOTEBOOK_TAB_GRAPHS      1

struct _MrmWindowPrivate {
    /* Current device */
    MrmDevice *current;

    /* Header bar */
    GtkWidget *header_bar;
    GtkWidget *back_button;
    GtkWidget *graph_stack_switcher;

    /* Notebook */
    GtkWidget *notebook;

    /* Device list tab */
    GtkWidget *device_list_frame;
    GtkWidget *device_list_box;
    GtkWidget *device_list_label;
    GtkWidget *device_list_spinner_box;
    GtkWidget *pin_entry_frame;
    GtkWidget *pin_entry;
    GtkWidget *attempts_label;
    GtkWidget *pin_check_spinner_box;

    /* Graphs */
    GtkWidget *signal_box;
    GtkWidget *power_box;

    guint initial_scan_done_id;
    guint device_detection_id;
    guint device_added_id;
    guint device_removed_id;
};

G_DEFINE_TYPE_WITH_PRIVATE (MrmWindow, mrm_window, GTK_TYPE_APPLICATION_WINDOW)

/******************************************************************************/

static void
change_current_device (MrmWindow *self,
                       MrmDevice *new_device)
{
    /* If same device, nothing else needed */
    if (new_device &&
        self->priv->current &&
        (self->priv->current == new_device ||
         g_str_equal (mrm_device_get_name (self->priv->current), mrm_device_get_name (new_device))))
        return;

    g_clear_object (&self->priv->current);

    mrm_signal_tab_change_current_device (MRM_SIGNAL_TAB (self->priv->signal_box), new_device);
    mrm_power_tab_change_current_device (MRM_POWER_TAB (self->priv->power_box), new_device);

    if (new_device)
        /* Keep a ref to current device */
        self->priv->current = g_object_ref (new_device);
}

/******************************************************************************/

static void
go_back_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
    MrmWindow *self = MRM_WINDOW (user_data);

    gtk_header_bar_set_title (GTK_HEADER_BAR (self->priv->header_bar), "Mobile Radio Monitor");
    gtk_widget_set_sensitive (self->priv->back_button, FALSE);
    gtk_widget_hide (self->priv->graph_stack_switcher);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (self->priv->notebook), NOTEBOOK_TAB_DEVICE_LIST);
}

static void
go_graphs_tab_cb (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
    MrmWindow *self = MRM_WINDOW (user_data);

    if (self->priv->current) {
        gchar *title;

        title = g_strdup_printf ("Mobile Radio Monitor - %s", mrm_device_get_model (self->priv->current));
        gtk_header_bar_set_title (GTK_HEADER_BAR (self->priv->header_bar), title);
        g_free (title);
    }

    gtk_widget_set_sensitive (self->priv->back_button, TRUE);
    gtk_widget_show (self->priv->graph_stack_switcher);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (self->priv->notebook), NOTEBOOK_TAB_GRAPHS);
}

static GActionEntry win_entries[] = {
    /* go */
    { "go-back",       go_back_cb,       NULL, "false", NULL },
    { "go-graphs-tab", go_graphs_tab_cb, NULL, "false", NULL },
};

/******************************************************************************/

void
mrm_window_open_device (MrmWindow *self,
                        MrmDevice *device)
{
    /* Setup device to use */
    change_current_device (self, device);

    /* Start NAS monitoring */
    mrm_device_start_nas (device, NULL, NULL);

    /* Go to the signal tab */
    g_action_group_activate_action (G_ACTION_GROUP (self), "go-graphs-tab", NULL);
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

static gint
device_list_sort (GtkListBoxRow *row1,
                  GtkListBoxRow *row2,
                  gpointer user_data)
{
    MrmDevice *device1;
    MrmDevice *device2;

    device1 = MRM_DEVICE (g_object_get_data (G_OBJECT (row1), DEVICE_TAG));
    device2 = MRM_DEVICE (g_object_get_data (G_OBJECT (row2), DEVICE_TAG));

    return g_strcmp0 (mrm_device_get_name (device1), mrm_device_get_name (device2));
}

static void
show_pin_request (MrmWindow *self,
                  MrmDevice *device)
{
    gint attempts;
    gchar *text;

    attempts = mrm_device_get_pin_attempts_left (device);
    if (attempts >= 0)
        text = g_strdup_printf ("%d attempts left", attempts);
    else
        text = g_strdup_printf ("unknown attempts left");

    gtk_label_set_text (GTK_LABEL (self->priv->attempts_label), text);
    gtk_entry_set_text (GTK_ENTRY (self->priv->pin_entry), "");
    gtk_widget_grab_focus (self->priv->pin_entry);
    gtk_widget_show (self->priv->attempts_label);
    gtk_widget_hide (self->priv->pin_check_spinner_box);
    gtk_widget_show (self->priv->pin_entry_frame);

    g_free (text);
}

static void
reset_pin_request (MrmWindow *self)
{
    gtk_label_set_text (GTK_LABEL (self->priv->attempts_label), "");
    gtk_entry_set_text (GTK_ENTRY (self->priv->pin_entry), "");
    gtk_widget_show (self->priv->attempts_label);
    gtk_widget_hide (self->priv->pin_check_spinner_box);
    gtk_widget_hide (self->priv->pin_entry_frame);
}

static void
ongoing_pin_request (MrmWindow *self)
{
    gtk_widget_show (self->priv->pin_check_spinner_box);
    gtk_widget_hide (self->priv->attempts_label);
}

static void
device_unlock_ready (MrmDevice *device,
                     GAsyncResult *res,
                     MrmWindow *self)
{
    /* Reset PIN unlocking widgets */
    reset_pin_request (self);

    if (!mrm_device_unlock_finish (device, res, NULL)) {
        /* Retry */
        show_pin_request (self, device);
        return;
    }

    /* Hide PIN unlocking widgets and go on */
    gtk_widget_hide (self->priv->pin_entry_frame);
    mrm_window_open_device (self, device);
}

static void
pin_entry_activated_cb (MrmWindow *self,
                        GtkEntry *pin_entry)
{
    GtkListBoxRow *row;
    MrmDevice *device;

    row = gtk_list_box_get_selected_row (GTK_LIST_BOX (self->priv->device_list_box));
    if (!row)
        return;

    device = MRM_DEVICE (g_object_get_data (G_OBJECT (row), DEVICE_TAG));
    if (!device)
        return;

    ongoing_pin_request (self);

    mrm_device_unlock (device,
                       gtk_entry_get_text (pin_entry),
                       (GAsyncReadyCallback)device_unlock_ready,
                       self);
}

static void
device_list_activate_row (MrmWindow *self,
                          GtkListBoxRow *row)
{
    MrmDevice *device;

    device = MRM_DEVICE (g_object_get_data (G_OBJECT (row), DEVICE_TAG));

    if (mrm_device_get_status (device) != MRM_DEVICE_STATUS_SIM_PIN_LOCKED) {
        mrm_window_open_device (self, device);
        return;
    }

    show_pin_request (self, device);
}

static void
update_row_sensitivity (MrmDevice *device,
                        GParamSpec *unused,
                        GtkWidget *row)
{
    switch (mrm_device_get_status (device)) {
    case MRM_DEVICE_STATUS_UNKNOWN:
    case MRM_DEVICE_STATUS_SIM_PUK_LOCKED:
    case MRM_DEVICE_STATUS_SIM_ERROR:
        gtk_widget_set_sensitive (row, FALSE);
        break;
    case MRM_DEVICE_STATUS_READY:
    case MRM_DEVICE_STATUS_SIM_PIN_LOCKED:
        gtk_widget_set_sensitive (row, TRUE);
        break;
    default:
        g_assert_not_reached ();
    }
}

static void
update_modem_status_label_text (MrmDevice *device,
                                GParamSpec *unused,
                                GtkWidget *status)
{
    switch (mrm_device_get_status (device)) {
    case MRM_DEVICE_STATUS_UNKNOWN:
        gtk_label_set_text (GTK_LABEL (status), "Unknown status");
        break;
    case MRM_DEVICE_STATUS_READY:
        gtk_label_set_text (GTK_LABEL (status), "Ready");
        break;
    case MRM_DEVICE_STATUS_SIM_PIN_LOCKED:
        gtk_label_set_text (GTK_LABEL (status), "PIN required");
        break;
    case MRM_DEVICE_STATUS_SIM_PUK_LOCKED:
        gtk_label_set_text (GTK_LABEL (status), "PUK required");
        break;
    case MRM_DEVICE_STATUS_SIM_ERROR:
        gtk_label_set_text (GTK_LABEL (status), "SIM error");
        break;
    default:
        g_assert_not_reached ();
    }
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
    g_object_set_data_full (G_OBJECT (row),
                            DEVICE_TAG,
                            g_object_ref (device),
                            g_object_unref);
    gtk_container_add (GTK_CONTAINER (self->priv->device_list_box), row);

    button_label_markup = g_strdup_printf ("[%s]\n\t<span weight=\"bold\">%s</span>\n\t<span style=\"italic\">%s</span>",
                                           mrm_device_get_name (device),
                                           mrm_device_get_model (device),
                                           mrm_device_get_manufacturer (device));
    button_label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (button_label), button_label_markup);
    gtk_widget_set_valign (GTK_WIDGET (button_label), 0.5f);

#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start (button_label, 20);
    gtk_widget_set_margin_end (button_label, 20);
#else
    gtk_widget_set_margin_left (button_label, 20);
    gtk_widget_set_margin_right (button_label, 20);
#endif
    gtk_widget_set_margin_top (button_label, 12);
    gtk_widget_set_margin_bottom (button_label, 12);
    gtk_widget_set_halign (button_label, GTK_ALIGN_START);
    gtk_widget_set_valign (button_label, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand (button_label, TRUE);
    gtk_box_pack_start (GTK_BOX (box), button_label, TRUE, TRUE, 0);

    status = gtk_label_new (NULL);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start (status, 20);
    gtk_widget_set_margin_end (status, 20);
#else
    gtk_widget_set_margin_left (status, 20);
    gtk_widget_set_margin_right (status, 20);
#endif

    gtk_widget_set_halign (status, GTK_ALIGN_END);
    gtk_widget_set_valign (status, GTK_ALIGN_CENTER);
    gtk_box_pack_end (GTK_BOX (box), status, FALSE, FALSE, 0);

    g_signal_connect (device,
                      "notify::status",
                      G_CALLBACK (update_row_sensitivity),
                      row);
    update_row_sensitivity (device, NULL, row);

    g_signal_connect (device,
                      "notify::status",
                      G_CALLBACK (update_modem_status_label_text),
                      status);
    update_modem_status_label_text (device, NULL, status);

    gtk_widget_show_all (row);

    gtk_widget_hide (self->priv->device_list_label);
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

    /* If we were monitoring this device already, stop it and back to the device
     * list tab */
    if (self->priv->current &&
        (device == self->priv->current ||
         g_str_equal (mrm_device_get_name (self->priv->current),
                      mrm_device_get_name (device)))) {
        change_current_device (self, NULL);
        g_action_group_activate_action (G_ACTION_GROUP (self), "go-back", NULL);
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

    /* Ensure we register the MrmSignalTab and MrmPowerTab before initiating
     * the template */
    g_warn_if_fail (mrm_signal_tab_get_type ());
    g_warn_if_fail (mrm_power_tab_get_type ());

    gtk_widget_init_template (GTK_WIDGET (self));

    g_action_map_add_action_entries (G_ACTION_MAP (self),
                                     win_entries, G_N_ELEMENTS (win_entries),
                                     self);

    g_signal_connect_swapped (self->priv->device_list_box,
                              "row-activated",
                              G_CALLBACK (device_list_activate_row),
                              self);

    g_signal_connect_swapped (self->priv->pin_entry,
                              "activate",
                              G_CALLBACK (pin_entry_activated_cb),
                              self);

    gtk_list_box_set_header_func (GTK_LIST_BOX (self->priv->device_list_box),
                                  device_list_update_header,
                                  NULL, NULL);

    gtk_list_box_set_sort_func (GTK_LIST_BOX (self->priv->device_list_box),
                                device_list_sort,
                                NULL, NULL);

    gtk_widget_show (self->priv->device_list_label);
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
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, header_bar);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, back_button);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, graph_stack_switcher);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, device_list_frame);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, device_list_box);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, device_list_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, device_list_spinner_box);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, pin_entry_frame);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, pin_entry);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, attempts_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, pin_check_spinner_box);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, signal_box);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, power_box);
}
