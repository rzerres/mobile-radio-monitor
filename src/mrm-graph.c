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
 * Graph drawing based on gnome-system-monitor's LoadGraph implementation.
 */


#include "mrm-graph.h"

#include <math.h>

/* Number of points to show in the graph */
#define NUM_POINTS 60

/* Number of vertical separators in the graph */
#define N_VERTICAL_SEPARATORS 4

/* Number of horizontal separators in the graph */
#define N_HORIZONTAL_SEPARATORS 6

/* Width of the graph frame */
#define FRAME_WIDTH 4

/* Indentation of the actual graph within the drawing area */
#define INDENT 24.0

/* Font size */
#define FONTSIZE 8.0

/* Right margin; to account for the space needed to display the
 * horizontal line label */
#define RIGHT_LABEL_MARGIN (7 * FONTSIZE)

/* Bottom label vertical margin */
#define BOTTOM_LABEL_MARGIN 15

G_DEFINE_TYPE (MrmGraph, mrm_graph, GTK_TYPE_BOX)

struct _MrmGraphPrivate {
    /* The drawing area */
    GtkWidget *drawing_area;

    /* Actual width and height available for drawing in the widget,
     * changes whenever the widget allocation size changes */
    guint draw_width;
    guint draw_height;

    /* The background surface */
    cairo_surface_t *background;
};

/*****************************************************************************/
/* Graph background management */

static void
graph_background_clear (MrmGraph *self)
{
    if (!self->priv->background)
        return;

    cairo_surface_destroy (self->priv->background);
    self->priv->background = NULL;
}

static void
graph_background_draw (MrmGraph *self)
{
    GtkAllocation allocation;
    cairo_t *cr;
    GtkStyleContext *context;
    GdkRGBA fg;
    PangoLayout *layout;
    PangoFontDescription *font_desc;
    gdouble dash[2] = { 1.0, 2.0 };
    guint i;
    guint vertical_separator_relative_height;
    guint real_draw_height;

    /* Create surface ad cairo context */
    gtk_widget_get_allocation (self->priv->drawing_area, &allocation);
    self->priv->background = gdk_window_create_similar_surface (gtk_widget_get_window (self->priv->drawing_area),
                                                                CAIRO_CONTENT_COLOR_ALPHA,
                                                                allocation.width,
                                                                allocation.height);
    cr = cairo_create (self->priv->background);

    /* Setup foreground color from style context */
    context = gtk_widget_get_style_context (self->priv->drawing_area);
    gtk_style_context_get_color (context,
                                 GTK_STATE_FLAG_NORMAL,
                                 &fg);

    /* Setup pango */
    cairo_paint_with_alpha (cr, 0.0);
    layout = pango_cairo_create_layout (cr);
    gtk_style_context_get (context,
                           GTK_STATE_FLAG_NORMAL,
                           GTK_STYLE_PROPERTY_FONT, &font_desc,
                           NULL);
    pango_font_description_set_size (font_desc, 0.8 * FONTSIZE * PANGO_SCALE);
    pango_layout_set_font_description (layout, font_desc);
    pango_font_description_free (font_desc);

    /* Draw background rectangle */
    cairo_translate (cr, FRAME_WIDTH, FRAME_WIDTH);
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    vertical_separator_relative_height = (self->priv->draw_height - BOTTOM_LABEL_MARGIN) / N_VERTICAL_SEPARATORS;
    real_draw_height = vertical_separator_relative_height * N_VERTICAL_SEPARATORS;
    cairo_rectangle (cr, INDENT, 0,
                     self->priv->draw_width - INDENT - RIGHT_LABEL_MARGIN,
                     real_draw_height);
    cairo_fill (cr);

    /* Draw horizontal lines (vertical separators)  */
    cairo_set_line_width (cr, 1.0);
    cairo_set_dash (cr, dash, 2, 0);
    for (i = 0; i <= N_VERTICAL_SEPARATORS; ++i) {
        PangoRectangle extents;
        gdouble y;
        guint max = 100;
        gchar *caption;

        if (i == 0)
            y = 0.5 + FONTSIZE / 2.0;
        else if (i == N_VERTICAL_SEPARATORS)
            y = i * vertical_separator_relative_height + 0.5;
        else
            y = i * vertical_separator_relative_height + FONTSIZE / 2.0;

        /* Draw line */
        cairo_set_source_rgba (cr, 0, 0, 0, 0.75);
        cairo_move_to (cr,
                       INDENT,
                       i * vertical_separator_relative_height + 0.5);
        cairo_line_to (cr,
                       self->priv->draw_width - RIGHT_LABEL_MARGIN + 0.5 + 4,
                       i * vertical_separator_relative_height + 0.5);

        /* Draw caption */
        gdk_cairo_set_source_rgba (cr, &fg);
        caption = g_strdup_printf ("%d %%", max - i * (max / N_VERTICAL_SEPARATORS));
        pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);
        pango_layout_set_text (layout, caption, -1);
        pango_layout_get_extents (layout, NULL, &extents);
        cairo_move_to (cr,
                       self->priv->draw_width - INDENT - 23,
                       y - 1.0 * extents.height / PANGO_SCALE / 2);
        pango_cairo_show_layout (cr, layout);
        g_free (caption);
    }
    cairo_stroke (cr);

    /* Draw vertical lines (horizontal separators) */
    cairo_set_dash (cr, dash, 2, 1.5);
    for (i = 0; i <= N_HORIZONTAL_SEPARATORS; i++) {
        PangoRectangle extents;
        double x;
        gchar *caption;

        /* Draw line */
        x = i * (self->priv->draw_width - RIGHT_LABEL_MARGIN - INDENT) / N_HORIZONTAL_SEPARATORS;
        cairo_set_source_rgba (cr, 0, 0, 0, 0.75);
        cairo_move_to (cr, (ceil (x) + 0.5) + INDENT, 0.5);
        cairo_line_to (cr, (ceil (x) + 0.5) + INDENT, real_draw_height + 4.5);
        cairo_stroke (cr);

        /* Draw caption */
        caption = g_strdup_printf (i == 0 ? "%u seconds" : "%u", i * 10);
        pango_layout_set_text (layout, caption, -1);
        pango_layout_get_extents (layout, NULL, &extents);
        cairo_move_to (cr,
                       (ceil (x) + 0.5 + INDENT) - (1.0 * extents.width / PANGO_SCALE / 2),
                       self->priv->draw_height - 1.0 * extents.height / PANGO_SCALE);
        gdk_cairo_set_source_rgba (cr, &fg);
        pango_cairo_show_layout (cr, layout);
        g_free (caption);
    }
    cairo_stroke (cr);

    cairo_destroy (cr);
    g_object_unref (layout);
}

/*****************************************************************************/
/* Reconfigure draw */

static gboolean
graph_configure (GtkWidget *widget,
                 GdkEventConfigure *event,
                 MrmGraph *self)
{
    GtkAllocation allocation;

    gtk_widget_get_allocation (widget, &allocation);
    self->priv->draw_width  = allocation.width  - 2 * FRAME_WIDTH;
    self->priv->draw_height = allocation.height - 2 * FRAME_WIDTH;

    /* repaint */
    graph_background_clear (self);
    gtk_widget_queue_draw (self->priv->drawing_area);

    return TRUE;
}

/*****************************************************************************/
/* Graph draw */

static gboolean
graph_draw (GtkWidget *widget,
            cairo_t *cr,
            MrmGraph *self)
{
    GdkWindow *window;

    window = gtk_widget_get_window (self->priv->drawing_area);

    if (!self->priv->background) {
        cairo_pattern_t *pattern;

        graph_background_draw (self);
        pattern = cairo_pattern_create_for_surface (self->priv->background);
        gdk_window_set_background_pattern (window, pattern);
        cairo_pattern_destroy (pattern);
    }

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
    g_signal_connect (G_OBJECT (self->priv->drawing_area),
                      "configure-event",
                      G_CALLBACK (graph_configure),
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
