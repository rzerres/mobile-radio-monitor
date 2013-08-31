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
#include <gudev/gudev.h>

#include "mrm-app.h"
#include "mrm-window.h"
#include "mrm-device.h"

G_DEFINE_TYPE (MrmApp, mrm_app, GTK_TYPE_APPLICATION)

enum {
    SIGNAL_INITIAL_SCAN_DONE,
    SIGNAL_DEVICE_ADDED,
    SIGNAL_DEVICE_REMOVED,
    SIGNAL_LAST
};

static guint signals [SIGNAL_LAST] = { 0 };

struct _MrmAppPrivate {
    /* The UDev client */
    GUdevClient *udev_client;
    guint initial_scan_id;
    gboolean initial_scan_done;

    /* List of devices being added */
    GList *pending_devices;

    /* List of MrmDevices */
    GList *devices;
};

/******************************************************************************/

gboolean
mrm_app_is_initial_scan_done (MrmApp *self)
{
    g_return_val_if_fail (MRM_IS_APP (self), FALSE);

    return self->priv->initial_scan_done;
}

GList *
mrm_app_peek_devices (MrmApp *self)
{
    g_return_val_if_fail (MRM_IS_APP (self), NULL);

    return self->priv->devices;
}

/******************************************************************************/

typedef struct {
    gchar *device_name;
    GCancellable *cancellable;
} PendingDeviceInfo;

static PendingDeviceInfo *
pending_device_info_find (MrmApp *self,
                          const gchar *device_name)
{
    GList *l;

    for (l = self->priv->pending_devices; l; l = g_list_next (l)) {
        if (g_str_equal (((PendingDeviceInfo *)l->data)->device_name, device_name))
            return (PendingDeviceInfo *)l->data;
    }
    return NULL;
}

static void
pending_device_info_cancel (MrmApp *self,
                            const gchar *device_name)
{
    PendingDeviceInfo *info;

    info = pending_device_info_find (self, device_name);
    if (info)
        g_cancellable_cancel (info->cancellable);
}

static void
pending_device_info_remove (MrmApp *self,
                            const gchar *device_name)
{
    PendingDeviceInfo *info;

    info = pending_device_info_find (self, device_name);
    if (!info)
        return;

    self->priv->pending_devices = g_list_remove (self->priv->pending_devices, info);
    g_object_unref (info->cancellable);
    g_free (info->device_name);
    g_slice_free (PendingDeviceInfo, info);
}

static void
pending_device_info_add (MrmApp *self,
                         const gchar *device_name,
                         GCancellable *cancellable)
{
    PendingDeviceInfo *info;

    info = g_slice_new0 (PendingDeviceInfo);
    info->device_name = g_strdup (device_name);
    info->cancellable = g_object_ref (cancellable);

    self->priv->pending_devices = g_list_append (self->priv->pending_devices, info);
}

/******************************************************************************/

typedef struct {
    MrmApp *self;
    GFile *file;
    GCancellable *cancellable;
    gchar *device_name;
} PortAddedContext;

static void
port_added_context_free (PortAddedContext *ctx)
{
    g_free (ctx->device_name);
    g_object_unref (ctx->cancellable);
    g_object_unref (ctx->file);
    g_object_unref (ctx->self);
    g_slice_free (PortAddedContext, ctx);
}

static void
device_new_ready (GObject *source,
                  GAsyncResult *res,
                  PortAddedContext *ctx)
{
    GError *error = NULL;
    MrmDevice *device;

    device = mrm_device_new_finish (res, &error);
    if (!device) {
        g_warning ("Cannot create MRM device: %s", error->message);
        g_error_free (error);
    } else if (g_cancellable_is_cancelled (ctx->cancellable)) {
        g_warning ("MRM device creation cancelled");
        g_object_unref (device);
    } else {
        /* Add device */
        g_signal_emit (ctx->self, signals[SIGNAL_DEVICE_ADDED], 0, device);
        ctx->self->priv->devices = g_list_append (ctx->self->priv->devices, device);
    }

    pending_device_info_remove (ctx->self, ctx->device_name);

    /* If this was the last pending device, we're done */
    if (!ctx->self->priv->initial_scan_done && !ctx->self->priv->pending_devices) {
        ctx->self->priv->initial_scan_done = TRUE;
        g_signal_emit (ctx->self, signals[SIGNAL_INITIAL_SCAN_DONE], 0);
    }

    port_added_context_free (ctx);
}

static gboolean
filter_usb_device (GUdevDevice *device)
{
    const gchar *subsystem;
    const gchar *name;
    const gchar *driver;
    GUdevDevice *parent = NULL;
    gboolean filtered = TRUE;

    /* Subsystems: 'usb', 'usbmisc' or 'net' */
    subsystem = g_udev_device_get_subsystem (device);
    if (!subsystem || !g_str_has_prefix (subsystem, "usb"))
        goto out;

    /* Names: if 'usb' or 'usbmisc' only 'cdc-wdm' prefixed names allowed */
    name = g_udev_device_get_name (device);
    if (!name || !g_str_has_prefix (name, "cdc-wdm"))
        goto out;

    /* Drivers: 'qmi_wwan' only */
    driver = g_udev_device_get_driver (device);
    if (!driver) {
        parent = g_udev_device_get_parent (device);
        if (parent)
            driver = g_udev_device_get_driver (parent);
    }
    if (!driver || !g_str_equal (driver, "qmi_wwan"))
        goto out;

    /* Not filtered! */
    filtered = FALSE;

 out:
    if (parent)
        g_object_unref (parent);
    return filtered;
}

static void
port_added (MrmApp *self,
            GUdevDevice *udev_device)
{
    PortAddedContext *ctx;
    gchar *path;

    /* Filter */
    if (filter_usb_device (udev_device))
        return;

    path = g_strdup_printf ("/dev/%s", g_udev_device_get_name (udev_device));
    g_debug ("QMI device file available: %s", path);

    ctx = g_slice_new (PortAddedContext);
    ctx->self = g_object_ref (self);
    ctx->device_name = g_strdup (g_udev_device_get_name (udev_device));
    ctx->file = g_file_new_for_path (path);
    ctx->cancellable = g_cancellable_new ();
    pending_device_info_add (self, ctx->device_name, ctx->cancellable);

    mrm_device_new (ctx->file,
                    ctx->cancellable,
                    (GAsyncReadyCallback) device_new_ready,
                    ctx);

    g_free (path);
}

static void
port_removed (MrmApp *self,
              GUdevDevice *udev_device)
{
    GList *l;

    /* Remove from device file list */
    for (l = self->priv->devices; l; l = g_list_next (l)) {
        MrmDevice *device = MRM_DEVICE (l->data);

        if (g_str_equal (mrm_device_get_name (device), g_udev_device_get_name (udev_device))) {
            g_debug ("QMI device file unavailable: /dev/%s", g_udev_device_get_name (udev_device));
            self->priv->devices = g_list_delete_link (self->priv->devices, l);
            g_signal_emit (self, signals[SIGNAL_DEVICE_REMOVED], 0, device);
            g_object_unref (device);
            return;
        }
    }

    /* In case we were still adding it... */
    pending_device_info_cancel (self, g_udev_device_get_name (udev_device));
}

static void
uevent_cb (GUdevClient *client,
           const gchar *action,
           GUdevDevice *device,
           MrmApp *self)
{
    /* Port added */
    if (g_str_equal (action, "add") ||
        g_str_equal (action, "move") ||
        g_str_equal (action, "change")) {
        port_added (self, device);
        return;
    }

    /* Port removed */
    if (g_str_equal (action, "remove")) {
        port_removed (self, device);
        return;
    }

    /* Ignore other actions */
}

static gboolean
initial_scan_cb (MrmApp *self)
{
    GList *devices, *iter;

    self->priv->initial_scan_id = 0;

    g_debug ("Scanning usb subsystems...");

    devices = g_udev_client_query_by_subsystem (self->priv->udev_client, "usb");
    for (iter = devices; iter; iter = g_list_next (iter)) {
        port_added (self, G_UDEV_DEVICE (iter->data));
        g_object_unref (G_OBJECT (iter->data));
    }
    g_list_free (devices);

    devices = g_udev_client_query_by_subsystem (self->priv->udev_client, "usbmisc");
    for (iter = devices; iter; iter = g_list_next (iter)) {
        port_added (self, G_UDEV_DEVICE (iter->data));
        g_object_unref (G_OBJECT (iter->data));
    }
    g_list_free (devices);

    /* If no pending devices, we're done */
    if (!self->priv->initial_scan_done && !self->priv->pending_devices) {
        self->priv->initial_scan_done = TRUE;
        g_signal_emit (self, signals[SIGNAL_INITIAL_SCAN_DONE], 0);
    }

    return FALSE;
}

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
    /* GtkWidget *window; */
    /* gchar *path; */
    /* gchar *display; */

    /* /\* No files to open, just return *\/ */
    /* if (n_files <= 0) */
    /*     return; */

    /* /\* More than one file; warn *\/ */
    /* if (n_files > 1) { */
    /*     guint i; */

    /*     g_warning ("Too many device files given (%d); only one expected.", n_files); */
    /*     for (i = 1; i < n_files; i++) { */
    /*         path = g_file_get_path (files[i]); */
    /*         display = g_filename_display_name (path); */
    /*         g_warning ("  Ignoring device file '%s'...", display); */
    /*         g_free (path); */
    /*         g_free (display); */
    /*     } */
    /* } */

    /* /\* Exactly one file, open it *\/ */
    /* path = g_file_get_path (files[0]); */
    /* display = g_filename_display_name (path); */
    /* g_debug ("Opening device file '%s'...", display); */
    /* g_free (path); */
    /* g_free (display); */

    /* /\* Peek window and open device file *\/ */
    /* ensure_main_window (MRM_APP (application)); */
    /* window = peek_main_window (MRM_APP (application)); */
    /* mrm_window_open (MRM_WINDOW (window), files[0]); */
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
                         "application-id", "es.aleksander.MobileRadioMonitor",
                         "flags",          G_APPLICATION_HANDLES_OPEN,
                         NULL);
}

static void
mrm_app_init (MrmApp *self)
{
    const gchar *subsys[] = { "usb", "usbmisc", NULL };

    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MRM_TYPE_APP, MrmAppPrivate);

    g_set_application_name ("MobileRadioMonitor");
    gtk_window_set_default_icon_name ("mobile-radio-monitor");

    /* Setup UDev client */
    self->priv->udev_client = g_udev_client_new (subsys);
    g_signal_connect (self->priv->udev_client, "uevent", G_CALLBACK (uevent_cb), self);

    /* Setup initial scan */
    self->priv->initial_scan_id = g_idle_add ((GSourceFunc) initial_scan_cb, self);
}

static void
dispose (GObject *object)
{
    MrmApp *self = MRM_APP (object);
    GList *l;

    if (self->priv->initial_scan_id != 0) {
        g_source_remove (self->priv->initial_scan_id);
        self->priv->initial_scan_id = 0;
    }

    g_assert (self->priv->pending_devices == NULL);

    g_list_free_full (self->priv->devices, g_object_unref);
    self->priv->devices = NULL;

    g_clear_object (&self->priv->udev_client);

    G_OBJECT_CLASS (mrm_app_parent_class)->dispose (object);
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

    signals[SIGNAL_DEVICE_ADDED] =
        g_signal_new ("device-added",
                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_NONE,
                      1,
                      MRM_TYPE_DEVICE);

    signals[SIGNAL_DEVICE_REMOVED] =
        g_signal_new ("device-removed",
                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_NONE,
                      1,
                      MRM_TYPE_DEVICE);

    signals[SIGNAL_INITIAL_SCAN_DONE] =
        g_signal_new ("initial-scan-done",
                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_NONE,
                      0);
}
