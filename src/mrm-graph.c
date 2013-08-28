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
 */


#include "mrm-graph.h"

G_DEFINE_TYPE (MrmGraph, mrm_graph, GTK_TYPE_BOX)

struct _MrmGraphPrivate {
    GtkWidget *drawing_area;
};

/*****************************************************************************/
/* Graph draw */

static gboolean
graph_draw (GtkWidget *widget,
            cairo_t *cr,
            MrmGraph *self)
{
    guint width, height;
    GdkRGBA color;

    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);
    cairo_arc (cr,
               width / 2.0, height / 2.0,
               MIN (width, height) / 2.0,
               0, 2 * G_PI);

    gtk_style_context_get_color (gtk_widget_get_style_context (widget), 0, &color);
    gdk_cairo_set_source_rgba (cr, &color);

    cairo_fill (cr);

    return FALSE;
}

/*****************************************************************************/
/* New MRM graph */

GtkWidget *
mrm_graph_new (void)
{
    MrmGraph *self;
    GtkWidget *drawing_area;

    self = MRM_GRAPH (g_object_new (MRM_TYPE_GRAPH,
                                    "orientation", GTK_ORIENTATION_VERTICAL,
                                    "spacing", 6,
                                    NULL));

    self->priv->drawing_area = gtk_drawing_area_new ();
    gtk_widget_set_size_request (self->priv->drawing_area, 100, 100);
    gtk_widget_set_events (self->priv->drawing_area, GDK_EXPOSURE_MASK);
    g_signal_connect (G_OBJECT (self->priv->drawing_area),
                      "draw",
                      G_CALLBACK (graph_draw),
                      self);

    gtk_box_pack_start (GTK_BOX (self), self->priv->drawing_area, TRUE, TRUE, 0);
    gtk_widget_show (self->priv->drawing_area);

    return GTK_WIDGET (self);
}

/*****************************************************************************/

static void
mrm_graph_init (MrmGraph *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MRM_TYPE_GRAPH, MrmGraphPrivate);
}

static void
mrm_graph_class_init (MrmGraphClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (MrmGraphPrivate));
}
