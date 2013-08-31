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

#ifndef __MRM_WINDOW_H__
#define __MRM_WINDOW_H__

#include <gtk/gtk.h>

#include "mrm-app.h"

G_BEGIN_DECLS

#define MRM_TYPE_WINDOW         (mrm_window_get_type ())
#define MRM_WINDOW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MRM_TYPE_WINDOW, MrmWindow))
#define MRM_WINDOW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), MRM_TYPE_WINDOW, MrmWindowClass))
#define MRM_IS_WINDOW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MRM_TYPE_WINDOW))
#define MRM_IS_WINDOW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MRM_TYPE_WINDOW))
#define MRM_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MRM_TYPE_WINDOW, MrmWindowClass))

typedef struct _MrmWindow        MrmWindow;
typedef struct _MrmWindowClass   MrmWindowClass;
typedef struct _MrmWindowPrivate MrmWindowPrivate;

struct _MrmWindow {
    GtkApplicationWindow parent_instance;
    MrmWindowPrivate *priv;
};

struct _MrmWindowClass {
    GtkApplicationWindowClass parent_class;
};

GType mrm_window_get_type(void) G_GNUC_CONST;

GtkWidget *mrm_window_new (MrmApp *application);

G_END_DECLS

#endif /* __MRM_WINDOW_H__ */
