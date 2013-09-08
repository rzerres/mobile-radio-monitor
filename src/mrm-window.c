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
#include "mrm-graph.h"

#define NOTEBOOK_TAB_DEVICE_LIST 0
#define NOTEBOOK_TAB_SIGNAL_INFO 1

struct _MrmWindowPrivate {
    GtkWidget *notebook;

    /* Header bar */
    GtkWidget *header_bar;
    GtkWidget *back_button;

    /* Device list tab */
    GtkWidget *device_list_frame;
    GtkWidget *device_list_box;
    GtkWidget *device_list_label;
    GtkWidget *device_list_spinner_box;
    GtkWidget *pin_entry_frame;
    GtkWidget *pin_entry;
    GtkWidget *attempts_label;
    GtkWidget *pin_check_spinner_box;

    /* Signal info tab */
    MrmDevice *current;
    GtkWidget *rssi_graph;
    guint rssi_graph_updated_id;
    GtkWidget *ecio_graph;
    guint ecio_graph_updated_id;

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
    gtk_header_bar_set_title (GTK_HEADER_BAR (self->priv->header_bar), "Mobile Radio Monitor");
    gtk_widget_set_sensitive (self->priv->back_button, FALSE);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (self->priv->notebook), NOTEBOOK_TAB_DEVICE_LIST);
}

typedef enum {
    SERIES_RSSI_GSM  = 0,
    SERIES_RSSI_UMTS = 1,
    SERIES_RSSI_LTE  = 2,
    SERIES_RSSI_CDMA = 3,
    SERIES_RSSI_EVDO = 4,
} SeriesRssi;

static void
rssi_updated (MrmDevice *device,
              gdouble gsm_rssi,
              gdouble umts_rssi,
              gdouble lte_rssi,
              gdouble cdma_rssi,
              gdouble evdo_rssi,
              MrmWindow *self)
{
    mrm_graph_step_init (MRM_GRAPH (self->priv->rssi_graph));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->rssi_graph), SERIES_RSSI_GSM,  gsm_rssi);
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->rssi_graph), SERIES_RSSI_UMTS, umts_rssi);
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->rssi_graph), SERIES_RSSI_LTE,  lte_rssi);
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->rssi_graph), SERIES_RSSI_CDMA, cdma_rssi);
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->rssi_graph), SERIES_RSSI_EVDO, evdo_rssi);
    mrm_graph_step_finish (MRM_GRAPH (self->priv->rssi_graph));
}

typedef enum {
    SERIES_ECIO_UMTS = 0,
    SERIES_ECIO_CDMA = 1,
    SERIES_ECIO_EVDO = 2,
} SeriesEcio;

static void
ecio_updated (MrmDevice *device,
              gdouble umts_ecio,
              gdouble cdma_ecio,
              gdouble evdo_ecio,
              MrmWindow *self)
{
    mrm_graph_step_init (MRM_GRAPH (self->priv->ecio_graph));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->ecio_graph), SERIES_ECIO_UMTS, umts_ecio);
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->ecio_graph), SERIES_ECIO_CDMA, cdma_ecio);
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->ecio_graph), SERIES_ECIO_EVDO, evdo_ecio);
    mrm_graph_step_finish (MRM_GRAPH (self->priv->ecio_graph));
}

static void
change_current_device (MrmWindow *self,
                       MrmDevice *new_device)
{
    if (self->priv->current) {
        /* If same device, nothing else needed */
        if (new_device &&
            (self->priv->current == new_device ||
             g_str_equal (mrm_device_get_name (self->priv->current), mrm_device_get_name (new_device))))
            return;

        /* Changing current device, cleanup */
        if (self->priv->rssi_graph_updated_id) {
            g_signal_handler_disconnect (self->priv->current, self->priv->rssi_graph_updated_id);
            self->priv->rssi_graph_updated_id = 0;
        }

        if (self->priv->ecio_graph_updated_id) {
            g_signal_handler_disconnect (self->priv->current, self->priv->ecio_graph_updated_id);
            self->priv->ecio_graph_updated_id = 0;
        }

        g_clear_object (&self->priv->current);

        /* Clear graphs */

        mrm_graph_clear_series (MRM_GRAPH (self->priv->rssi_graph), SERIES_RSSI_GSM);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->rssi_graph), SERIES_RSSI_UMTS);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->rssi_graph), SERIES_RSSI_LTE);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->rssi_graph), SERIES_RSSI_CDMA);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->rssi_graph), SERIES_RSSI_EVDO);

        mrm_graph_clear_series (MRM_GRAPH (self->priv->ecio_graph), SERIES_ECIO_UMTS);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->ecio_graph), SERIES_ECIO_CDMA);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->ecio_graph), SERIES_ECIO_EVDO);
    }

    if (new_device) {
        /* Keep a ref to current device */
        self->priv->current = g_object_ref (new_device);
        self->priv->rssi_graph_updated_id = g_signal_connect (new_device,
                                                              "rssi-updated",
                                                              G_CALLBACK (rssi_updated),
                                                              self);
        self->priv->ecio_graph_updated_id = g_signal_connect (new_device,
                                                              "ecio-updated",
                                                              G_CALLBACK (ecio_updated),
                                                              self);
    }
}

static void
select_signal_info_tab (MrmWindow *self,
                        MrmDevice *device)
{
    gchar *title;

    title = g_strdup_printf ("Mobile Radio Monitor - %s", mrm_device_get_model (device));
    gtk_header_bar_set_title (GTK_HEADER_BAR (self->priv->header_bar), title);
    g_free (title);
    gtk_widget_set_sensitive (self->priv->back_button, TRUE);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (self->priv->notebook), NOTEBOOK_TAB_SIGNAL_INFO);

    /* Setup device to use */
    change_current_device (self, device);
}

/******************************************************************************/

void
mrm_window_open_device (MrmWindow *self,
                        MrmDevice *device)
{
    select_signal_info_tab (self, device);

    /* Start NAS monitoring */
    mrm_device_start_nas (device, NULL, NULL);
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
        select_device_list_tab (self);
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

    /* Ensure we register the MrmGraph before initiating template */
    mrm_graph_get_type ();

    gtk_widget_init_template (GTK_WIDGET (self));

    g_signal_connect (self->priv->back_button,
                      "clicked",
                      G_CALLBACK (back_button_clicked),
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

#define GSM_RGB  76,  153, 0   /* green */
#define UMTS_RGB 204, 0,   0   /* red */
#define LTE_RGB  0,   76,  153 /* blue */
#define CDMA_RGB 153, 153, 0   /* yellow */
#define EVDO_RGB 153, 0,   153 /* purple */

    /* RSSI graph */
    mrm_graph_setup_series (MRM_GRAPH (self->priv->rssi_graph), SERIES_RSSI_GSM,  "GSM",  GSM_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->rssi_graph), SERIES_RSSI_UMTS, "UMTS", UMTS_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->rssi_graph), SERIES_RSSI_LTE,  "LTE",  LTE_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->rssi_graph), SERIES_RSSI_CDMA, "CDMA", CDMA_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->rssi_graph), SERIES_RSSI_EVDO, "EVDO", EVDO_RGB);


    /* ECIO graph */
    mrm_graph_setup_series (MRM_GRAPH (self->priv->ecio_graph), SERIES_ECIO_UMTS, "UMTS", UMTS_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->ecio_graph), SERIES_ECIO_CDMA, "CDMA", CDMA_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->ecio_graph), SERIES_ECIO_EVDO, "EVDO", EVDO_RGB);
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
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, device_list_frame);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, device_list_box);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, device_list_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, device_list_spinner_box);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, pin_entry_frame);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, pin_entry);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, attempts_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, pin_check_spinner_box);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, rssi_graph);
    gtk_widget_class_bind_template_child_private (widget_class, MrmWindow, ecio_graph);
}
