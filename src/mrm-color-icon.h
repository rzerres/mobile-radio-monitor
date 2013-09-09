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
 * GNU General Public License for more details:
 *
 * Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
 *
 * Based on the Gnome system monitor colour pickers
 *  Copyright (C) 2007 Karl Lattimer <karl@qdh.org.uk>
 */

#ifndef __MRM_COLOR_ICON_H__
#define __MRM_COLOR_ICON_H__

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MRM_TYPE_COLOR_ICON            (mrm_color_icon_get_type ())
#define MRM_COLOR_ICON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MRM_TYPE_COLOR_ICON, MrmColorIcon))
#define MRM_COLOR_ICON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MRM_TYPE_COLOR_ICON, MrmColorIconClass))
#define MRM_IS_COLOR_ICON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MRM_TYPE_COLOR_ICON))
#define MRM_IS_COLOR_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MRM_TYPE_COLOR_ICON))
#define MRM_COLOR_ICON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MRM_TYPE_COLOR_ICON, MrmColorIconClass))

typedef struct _MrmColorIcon           MrmColorIcon;
typedef struct _MrmColorIconClass      MrmColorIconClass;
typedef struct _MrmColorIconPrivate    MrmColorIconPrivate;

struct _MrmColorIcon
{
    GtkDrawingArea widget;
    MrmColorIconPrivate *priv;
};

struct _MrmColorIconClass
{
    GtkDrawingAreaClass parent_class;
};

GType mrm_color_icon_get_type (void) G_GNUC_CONST;

GtkWidget *mrm_color_icon_new (const GdkRGBA *color);

void mrm_color_icon_set_color (MrmColorIcon *self,
                               guint8 color_red,
                               guint8 color_green,
                               guint8 color_blue);

G_END_DECLS
#endif /* __MRM_COLOR_ICON_H__ */
