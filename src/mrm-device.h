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

#ifndef __MRM_DEVICE_H__
#define __MRM_DEVICE_H__

#include <gtk/gtk.h>
#include <libqmi-glib.h>

G_BEGIN_DECLS

#define MRM_TYPE_DEVICE         (mrm_device_get_type ())
#define MRM_DEVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MRM_TYPE_DEVICE, MrmDevice))
#define MRM_DEVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), MRM_TYPE_DEVICE, MrmDeviceClass))
#define MRM_IS_DEVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MRM_TYPE_DEVICE))
#define MRM_IS_DEVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MRM_TYPE_DEVICE))
#define MRM_DEVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MRM_TYPE_DEVICE, MrmDeviceClass))

typedef struct _MrmDevice        MrmDevice;
typedef struct _MrmDeviceClass   MrmDeviceClass;
typedef struct _MrmDevicePrivate MrmDevicePrivate;

struct _MrmDevice {
    GObject parent_instance;
    MrmDevicePrivate *priv;
};

struct _MrmDeviceClass {
    GObjectClass parent_class;
};

GType mrm_device_get_type (void) G_GNUC_CONST;

void       mrm_device_new        (GFile *file,
                                  GCancellable *cancellable,
                                  GAsyncReadyCallback callback,
                                  gpointer user_data);
MrmDevice *mrm_device_new_finish (GAsyncResult *res,
                                  GError **error);

const gchar *mrm_device_get_name         (MrmDevice *self);
const gchar *mrm_device_get_manufacturer (MrmDevice *self);
const gchar *mrm_device_get_model        (MrmDevice *self);
const gchar *mrm_device_get_revision     (MrmDevice *self);
QmiDevice   *mrm_device_peek_qmi_device  (MrmDevice *self);

G_END_DECLS

#endif /* __MRM_DEVICE_H__ */
