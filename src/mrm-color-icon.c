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
 * Copyright (C) 2013-2015 Aleksander Morgado <aleksander@aleksander.es>
 *
 * Based on the Gnome system monitor colour pickers
 *  Copyright (C) 2007 Karl Lattimer <karl@qdh.org.uk>
 */

#ifndef CMAKE_BUILD
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <cairo.h>

#include "mrm-color-icon.h"

G_DEFINE_TYPE (MrmColorIcon, mrm_color_icon, GTK_TYPE_DRAWING_AREA)

#define MIN_WIDTH 30
#define MIN_HEIGHT 15

enum
{
  PROP_0,
  PROP_COLOR,
};

struct _MrmColorIconPrivate {
    GdkRGBA color;
    cairo_surface_t *image_buffer;
};

static void
render (MrmColorIcon *self)
{
    cairo_t *cr;
    gint width, height;

    cr = gdk_cairo_create (gtk_widget_get_window (GTK_WIDGET (self)));

    gdk_cairo_set_source_rgba (cr, &(self->priv->color));
    width  = gdk_window_get_width  (gtk_widget_get_window (GTK_WIDGET (self)));
    height = gdk_window_get_height (gtk_widget_get_window (GTK_WIDGET (self)));

    cairo_paint (cr);
    cairo_set_line_width (cr, 1);
    cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
    cairo_rectangle (cr, 0.5, 0.5, width - 1, height - 1);
    cairo_stroke (cr);
    cairo_set_line_width (cr, 1);
    cairo_set_source_rgba (cr, 1, 1, 1, 0.4);
    cairo_rectangle (cr, 1.5, 1.5, width - 3, height - 3);
    cairo_stroke (cr);
    cairo_destroy (cr);
}

/******************************************************************************/

void
mrm_color_icon_set_color (MrmColorIcon *self,
                          guint8 color_red,
                          guint8 color_green,
                          guint8 color_blue)
{
    GdkRGBA color;

    color.red = ((gdouble)color_red) / 255.0;
    color.green = ((gdouble)color_green) / 255.0;
    color.blue = ((gdouble)color_blue) / 255.0;
    color.alpha = 1.0;

    g_object_set (self,
                  "color", &color,
                  NULL);
}

/******************************************************************************/

GtkWidget *
mrm_color_icon_new (const GdkRGBA *color)
{
    return g_object_new (MRM_TYPE_COLOR_ICON,
                         "color",   color,
                         NULL);
}

static gint
draw (GtkWidget *widget,
      cairo_t *cr)
{
    render (MRM_COLOR_ICON (widget));

    return FALSE;
}

static void
realize (GtkWidget *widget)
{
    GTK_WIDGET_CLASS (mrm_color_icon_parent_class)->realize (widget);

    render (MRM_COLOR_ICON (widget));
}

static void
get_preferred_width (GtkWidget *widget,
                     gint      *minimum,
                     gint      *natural)
{
    g_return_if_fail (widget != NULL || minimum != NULL || natural != NULL);
    g_return_if_fail (MRM_IS_COLOR_ICON (widget));

    *minimum = MIN_WIDTH;
    *natural = MIN_WIDTH;
}

static void
get_preferred_height (GtkWidget *widget,
                      gint      *minimum,
                      gint      *natural)
{
    g_return_if_fail (widget != NULL || minimum != NULL || natural != NULL);
    g_return_if_fail (MRM_IS_COLOR_ICON (widget));

    *minimum = MIN_HEIGHT;
    *natural = MIN_HEIGHT;
}

static void
size_allocate (GtkWidget *widget,
               GtkAllocation *allocation)
{
    g_return_if_fail (widget != NULL || allocation != NULL);
    g_return_if_fail (MRM_IS_COLOR_ICON (widget));

    gtk_widget_set_allocation (widget, allocation);

    if (gtk_widget_get_realized (widget)) {
        gdk_window_move_resize (gtk_widget_get_window (widget),
                                allocation->x,
                                allocation->y,
                                allocation->width,
                                allocation->height);
    }
}

static void
unrealize (GtkWidget *widget)
{
    GTK_WIDGET_CLASS (mrm_color_icon_parent_class)->unrealize (widget);
}

static void
style_set (GtkWidget *widget,
           GtkStyle *previous_style)
{
    GTK_WIDGET_CLASS (mrm_color_icon_parent_class)->style_set (widget, previous_style);
}

static void
set_property (GObject *object,
              guint param_id,
              const GValue *value,
              GParamSpec *pspec)
{
    MrmColorIcon *self = MRM_COLOR_ICON (object);

    switch (param_id)
    {
    case PROP_COLOR: {
        const GdkRGBA *color;

        color = g_value_get_boxed (value);
        self->priv->color.red = color ? color->red : 0.0;
        self->priv->color.green = color ? color->green : 0.0;
        self->priv->color.blue = color ? color->blue : 0.0;
        self->priv->color.alpha = color ? color->alpha : 1.0;
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
get_property (GObject *object,
              guint param_id,
              GValue *value,
              GParamSpec *pspec)
{
  MrmColorIcon *self = MRM_COLOR_ICON (object);

  switch (param_id) {
  case PROP_COLOR: {
      GdkRGBA color;

      color.red = self->priv->color.red;
      color.green = self->priv->color.green;
      color.blue = self->priv->color.blue;
      color.alpha = self->priv->color.alpha;
      g_value_set_boxed (value, &color);
      break;
  }
  default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
finalize (GObject *object)
{
    MrmColorIcon *self = MRM_COLOR_ICON (object);

    cairo_surface_destroy (self->priv->image_buffer);
    self->priv->image_buffer = NULL;

    G_OBJECT_CLASS (mrm_color_icon_parent_class)->finalize (object);
}

static void
mrm_color_icon_init (MrmColorIcon *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              MRM_TYPE_COLOR_ICON,
                                              MrmColorIconPrivate);

    self->priv->color.red = 0;
    self->priv->color.green = 0;
    self->priv->color.blue = 0;
    self->priv->color.alpha = 1.0;
    self->priv->image_buffer = NULL;

    g_signal_connect (self,
                      "draw",
                      G_CALLBACK (draw),
                      NULL);
}

static void
mrm_color_icon_class_init (MrmColorIconClass * klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    g_type_class_add_private (gobject_class, sizeof (MrmColorIconPrivate));

    gobject_class->get_property = get_property;
    gobject_class->set_property = set_property;
    gobject_class->finalize = finalize;

    widget_class->get_preferred_width  = get_preferred_width;
    widget_class->get_preferred_height = get_preferred_height;
    widget_class->size_allocate = size_allocate;
    widget_class->realize = realize;
    widget_class->unrealize = unrealize;
    widget_class->style_set = style_set;

    g_object_class_install_property (gobject_class,
                                     PROP_COLOR,
                                     g_param_spec_boxed ("color",
                                                         "Current Color",
                                                         "The selected color",
                                                         GDK_TYPE_RGBA,
                                                         G_PARAM_READWRITE));
}
