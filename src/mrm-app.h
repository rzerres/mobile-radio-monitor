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

#ifndef __MRM_APP_H__
#define __MRM_APP_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MRM_TYPE_APP         (mrm_app_get_type ())
#define MRM_APP(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MRM_TYPE_APP, MrmApp))
#define MRM_APP_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), MRM_TYPE_APP, MrmAppClass))
#define MRM_IS_APP(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MRM_TYPE_APP))
#define MRM_IS_APP_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MRM_TYPE_APP))
#define MRM_APP_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MRM_TYPE_APP, MrmAppClass))

typedef struct _MrmApp        MrmApp;
typedef struct _MrmAppClass   MrmAppClass;
typedef struct _MrmAppPrivate MrmAppPrivate;

struct _MrmApp {
    GtkApplication parent_instance;
    MrmAppPrivate *priv;
};

struct _MrmAppClass {
    GtkApplicationClass parent_class;
};

GType mrm_app_get_type (void) G_GNUC_CONST;

MrmApp   *mrm_app_new                  (void);
void      mrm_app_quit                 (MrmApp *self);
gboolean  mrm_app_is_initial_scan_done (MrmApp *self);
GList    *mrm_app_peek_devices         (MrmApp *self);

G_END_DECLS

#endif /* __MRM_APP_H__ */
