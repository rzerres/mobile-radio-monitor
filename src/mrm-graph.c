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
 * Graph drawing based on gnome-system-monitor's LoadGraph implementation.
 */


#include "mrm-graph.h"
#include "mrm-enum-types.h"
#include "mrm-color-icon.h"

#include <math.h>

/* Number of points to show in the graph */
#define NUM_POINTS 61

/* Number of horizontal separators in the graph */
#define N_HORIZONTAL_SEPARATORS 6

/* Width of the graph frame */
#define FRAME_WIDTH 2

/* Indentation of the actual graph within the drawing area */
#define INDENT 24.0

/* Font size */
#define FONTSIZE 8.0

/* Right margin; to account for the space needed to display the
 * horizontal line label */
#define RIGHT_LABEL_MARGIN (8 * FONTSIZE)

/* Bottom label vertical margin */
#define BOTTOM_LABEL_MARGIN 15

G_DEFINE_TYPE (MrmGraph, mrm_graph, GTK_TYPE_BOX)

enum {
    PROP_0,
    PROP_N_SERIES,
    PROP_Y_MIN,
    PROP_Y_MAX,
    PROP_Y_UNITS,
    PROP_Y_N_SEPARATORS,
    PROP_TITLE,
    PROP_LEGEND_POSITION,
    PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

typedef struct {
    gchar *text;
    GdkRGBA color;
    gdouble data[NUM_POINTS];
    GtkWidget *box;
    GtkWidget *box_icon;
    GtkWidget *box_label;
    GtkWidget *box_value;
} Series;

struct _MrmGraphPrivate {
    /* Properties */
    guint    n_series;
    gdouble  y_min;
    gdouble  y_max;
    gchar   *y_units;
    guint    y_n_separators;
    gchar   *title;
    MrmGraphLegendPosition legend_position;

    /* The series data block */
    Series *series;

    /* The current step index */
    guint step_index;

    /* Graph title label */
    GtkWidget *title_label;

    /* The drawing area */
    GtkWidget *drawing_area;

    /* Legend box */
    GtkWidget *legend_box;

    /* Actual width and height available for drawing in the widget,
     * changes whenever the widget allocation size changes */
    guint draw_width;
    guint draw_height;

    /* The background surface */
    cairo_pattern_t *background;

    /* Useful coordinates updated whenever background is changed */
    gdouble plot_area_offset_x0;
    gdouble plot_area_offset_y0;
    gdouble plot_area_width;
    gdouble plot_area_height;
};

/*****************************************************************************/
/* Series management */

static void
free_series (MrmGraph *self)
{
    guint i;

    if (!self->priv->series)
        return;

    for (i = 0; i < self->priv->n_series; i++)
        g_free (self->priv->series[i].text);
    g_free (self->priv->series);
    self->priv->series = NULL;
}

void
mrm_graph_clear_series (MrmGraph *self,
                        guint series_index)
{
    guint i;

    for (i = 0; i < NUM_POINTS; i++)
        self->priv->series[series_index].data[i] = -G_MAXDOUBLE;

    g_free (self->priv->series[series_index].text);
    self->priv->series[series_index].text = NULL;

    if (self->priv->series[series_index].box) {
        if (self->priv->legend_box)
            gtk_container_remove (GTK_CONTAINER (self->priv->legend_box),
                                  self->priv->series[series_index].box);
        self->priv->series[series_index].box = NULL;
        self->priv->series[series_index].box_icon = NULL;
        self->priv->series[series_index].box_label = NULL;
        self->priv->series[series_index].box_value = NULL;
    }
}

static void
allocate_series (MrmGraph *self)
{
    guint i;

    if (!self->priv->n_series)
        return;

    self->priv->series = g_new0 (Series, self->priv->n_series);
    for (i = 0; i < self->priv->n_series; i++)
        mrm_graph_clear_series (self, i);
}

void
mrm_graph_setup_series (MrmGraph *self,
                        guint series_index,
                        const gchar *label,
                        guint8 color_red,
                        guint8 color_green,
                        guint8 color_blue)
{
    g_assert_cmpuint (series_index, <, self->priv->n_series);

    mrm_graph_clear_series (self, series_index);

    self->priv->series[series_index].text = g_strdup (label);
    self->priv->series[series_index].color.red = ((gdouble)color_red) / 255.0;
    self->priv->series[series_index].color.green = ((gdouble)color_green) / 255.0;
    self->priv->series[series_index].color.blue = ((gdouble)color_blue) / 255.0;
    self->priv->series[series_index].color.alpha = 1.0;

    self->priv->series[series_index].box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
    if (self->priv->legend_box)
        gtk_box_pack_start (GTK_BOX (self->priv->legend_box), self->priv->series[series_index].box, TRUE, TRUE, 0);
    gtk_widget_show (self->priv->series[series_index].box);

    self->priv->series[series_index].box_icon = mrm_color_icon_new (&self->priv->series[series_index].color);
    gtk_box_pack_start (GTK_BOX (self->priv->series[series_index].box), self->priv->series[series_index].box_icon, FALSE, TRUE, 0);
    gtk_widget_show (self->priv->series[series_index].box_icon);

    self->priv->series[series_index].box_label = gtk_label_new (label);
    gtk_box_pack_start (GTK_BOX (self->priv->series[series_index].box), self->priv->series[series_index].box_label, FALSE, TRUE, 0);
    gtk_widget_show (self->priv->series[series_index].box_label);

    self->priv->series[series_index].box_value = gtk_label_new ("N/A");
    gtk_box_pack_start (GTK_BOX (self->priv->series[series_index].box), self->priv->series[series_index].box_value, FALSE, TRUE, 0);
    gtk_widget_show (self->priv->series[series_index].box_value);
}

/*****************************************************************************/
/* Adding new values to the series */

void
mrm_graph_step_init (MrmGraph *self)
{
    guint i;

    for (i = 0; i < self->priv->n_series; i++)
        self->priv->series[i].data[self->priv->step_index] = -G_MAXDOUBLE;
}

void
mrm_graph_step_set_value (MrmGraph *self,
                          guint series_index,
                          gdouble value,
                          GtkLabel *additional_label)
{
    g_assert_cmpuint (series_index, <, self->priv->n_series);
    g_assert_cmpuint (self->priv->step_index, <, NUM_POINTS);

    self->priv->series[series_index].data[self->priv->step_index] =
        CLAMP (value, self->priv->y_min, self->priv->y_max);

    if (value < self->priv->y_min ||
        value > self->priv->y_max) {
        gtk_label_set_text (GTK_LABEL (self->priv->series[series_index].box_value), "N/A");
        if (additional_label)
            gtk_label_set_text (additional_label, "N/A");
    } else {
        gchar *str;

        str = g_strdup_printf ("%.2lf %s",
                               self->priv->series[series_index].data[self->priv->step_index],
                               self->priv->y_units);
        gtk_label_set_text (GTK_LABEL (self->priv->series[series_index].box_value), str);
        if (additional_label)
            gtk_label_set_text (additional_label, str);
        g_free (str);
    }
}

void
mrm_graph_step_finish (MrmGraph *self)
{
    /* Update step */
    self->priv->step_index++;
    if (self->priv->step_index == NUM_POINTS)
        self->priv->step_index = 0;

    g_assert_cmpuint (self->priv->step_index, <, NUM_POINTS);

    /* Repaint */
    gtk_widget_queue_draw (self->priv->drawing_area);
}

/*****************************************************************************/
/* Graph background management */

static void
graph_background_clear (MrmGraph *self)
{
    if (!self->priv->background)
        return;

    cairo_pattern_destroy (self->priv->background);
    self->priv->background = NULL;
}

static void
graph_background_pattern_create (MrmGraph *self)
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
    cairo_surface_t *background;

    /* Create surface ad cairo context */
    gtk_widget_get_allocation (self->priv->drawing_area, &allocation);
    background = gdk_window_create_similar_surface (gtk_widget_get_window (self->priv->drawing_area),
                                                    CAIRO_CONTENT_COLOR_ALPHA,
                                                    allocation.width,
                                                    allocation.height);
    cr = cairo_create (background);

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
    pango_font_description_set_size (font_desc, FONTSIZE * PANGO_SCALE);
    pango_layout_set_font_description (layout, font_desc);
    pango_font_description_free (font_desc);

    /* Compute actual plotting area (i.e. area where the lines will be drawn).
     * The plot area height is computed based on the number of separators and the
     * fixed size of each separator. */
    vertical_separator_relative_height = (guint)(((gdouble)(self->priv->draw_height - BOTTOM_LABEL_MARGIN)) /
                                                 ((gdouble)self->priv->y_n_separators));
    self->priv->plot_area_height = vertical_separator_relative_height * self->priv->y_n_separators;
    self->priv->plot_area_width = self->priv->draw_width - INDENT - RIGHT_LABEL_MARGIN;
    self->priv->plot_area_offset_x0 = FRAME_WIDTH + INDENT + 1;
    self->priv->plot_area_offset_y0 = FRAME_WIDTH + self->priv->plot_area_height - 1;

    /* Draw background rectangle */
    cairo_translate (cr, FRAME_WIDTH, FRAME_WIDTH);
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    cairo_rectangle (cr,
                     INDENT, 0,
                     self->priv->plot_area_width, self->priv->plot_area_height);
    cairo_fill (cr);

    /* Draw horizontal lines (vertical separators)  */
    cairo_set_line_width (cr, 1.0);
    cairo_set_dash (cr, dash, 2, 0);
    for (i = 0; i <= self->priv->y_n_separators; ++i) {
        PangoRectangle extents;
        gdouble y;
        gchar *caption;

        if (i == 0)
            y = 0.5 + FONTSIZE / 2.0;
        else if (i == self->priv->y_n_separators)
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
        caption = g_strdup_printf ("%d %s",
                                   (gint)(self->priv->y_max - i * ((self->priv->y_max - self->priv->y_min) / self->priv->y_n_separators)),
                                   self->priv->y_units);
        pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);
        pango_layout_set_text (layout, caption, -1);
        pango_layout_get_extents (layout, NULL, &extents);
        cairo_move_to (cr,
                       self->priv->draw_width - INDENT - 30,
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
        cairo_line_to (cr, (ceil (x) + 0.5) + INDENT, self->priv->plot_area_height + 4.5);
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

    self->priv->background = cairo_pattern_create_for_surface (background);
    cairo_surface_destroy (background);
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
            cairo_t *context,
            MrmGraph *self)
{
    GdkWindow *window;
    gdouble sample_width;
    cairo_t *cr;
    guint i;
    gdouble x_ratio;
    gdouble y_ratio;
    guint current_step_index;

    window = gtk_widget_get_window (self->priv->drawing_area);

    if (!self->priv->background)
        graph_background_pattern_create (self);

    cr = gdk_cairo_create (window);
    cairo_set_source (cr, self->priv->background);
    cairo_rectangle (cr,
                     0, 0,
                     self->priv->draw_width, self->priv->draw_height);
    cairo_fill (cr);
    cairo_set_line_width (cr, 1);
    cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
    cairo_rectangle (cr,
                     self->priv->plot_area_offset_x0,
                     self->priv->plot_area_offset_y0 - self->priv->plot_area_height,
                     self->priv->plot_area_width,
                     self->priv->plot_area_height);
    cairo_clip (cr);

    /* Compute ratios to convert from values to pixels */
    x_ratio = ((gdouble)self->priv->plot_area_width) / ((gdouble)NUM_POINTS - 1);
    y_ratio = ((gdouble)self->priv->plot_area_height) / ((gdouble)(self->priv->y_max - self->priv->y_min));

    /* Mark which is the current point */
    if (self->priv->step_index == 0)
        current_step_index = NUM_POINTS - 1;
    else
        current_step_index = self->priv->step_index - 1;

    /* Print series */
    for (i = 0; i < self->priv->n_series; i++) {
        guint j;
        guint seconds;
        gdouble x1;
        gdouble y1;
        gdouble x2;
        gdouble y2;
        gdouble x3;
        gdouble y3;

        /* Set series color */
        gdk_cairo_set_source_rgba (cr, &(self->priv->series[i].color));

        /* Convert the current (seconds,value) to the corresponding amount of pixels */
        x3 = 0;
        y3 = (self->priv->series[i].data[current_step_index] - self->priv->y_min) * y_ratio;

        cairo_move_to (cr,
                       self->priv->plot_area_offset_x0 - x3,
                       self->priv->plot_area_offset_y0 - y3);

        seconds = 0;
        j = current_step_index;
        do {


            seconds++;
            if (j == 0)
                j = NUM_POINTS - 1;
            else
                j--;

            if (self->priv->series[i].data[j] < self->priv->y_min)
                continue;

            /* Convert the previous (seconds,value) to the corresponding amount of pixels */
            x3 = seconds * x_ratio;
            y3 = (self->priv->series[i].data[j] - self->priv->y_min) * y_ratio;

            /* Additional control points for the bezier spline */
            x1 = ((gdouble)seconds - 1.0f + 0.5f) * x_ratio;
            y1 = (self->priv->series[i].data[j == (NUM_POINTS - 1) ? 0 : j + 1] - self->priv->y_min) * y_ratio;
            x2 = ((gdouble)seconds - 0.5f) * x_ratio;
            y2 = (self->priv->series[i].data[j] - self->priv->y_min) * y_ratio;

            cairo_curve_to (cr,
                            self->priv->plot_area_offset_x0 + x1,
                            self->priv->plot_area_offset_y0 - y1,
                            self->priv->plot_area_offset_x0 + x2,
                            self->priv->plot_area_offset_y0 - y2,
                            self->priv->plot_area_offset_x0 + x3,
                            self->priv->plot_area_offset_y0 - y3);

        } while (j != self->priv->step_index);

        cairo_stroke (cr);
    }

    cairo_destroy (cr);

    return TRUE;
}

/*****************************************************************************/
/* Update graph title */

static void
update_graph_title (MrmGraph *self)
{
    gchar *str;

    if (!self->priv->title_label)
        return;

    str = (self->priv->title ?
           g_strdup_printf ("<span weight=\"bold\">%s</span>", self->priv->title) :
           g_strdup (""));
    gtk_label_set_markup (GTK_LABEL (self->priv->title_label), str);
    g_free (str);
}

/*****************************************************************************/
/* New MRM graph */

GtkWidget *
mrm_graph_new (void)
{
    MrmGraph *self;
    GtkWidget *drawing_area;

    self = MRM_GRAPH (g_object_new (MRM_TYPE_GRAPH, NULL));

    return GTK_WIDGET (self);
}

/*****************************************************************************/

static void
mrm_graph_init (MrmGraph *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MRM_TYPE_GRAPH, MrmGraphPrivate);

    /* Defaults */
    self->priv->n_series = 0;
    self->priv->y_min = 0.0;
    self->priv->y_max = 100.0;
    self->priv->y_units = g_strdup ("%");
    self->priv->y_n_separators = 5;
    self->priv->legend_position = MRM_GRAPH_LEGEND_POSITION_BOTTOM;
}

static void
constructed (GObject *object)
{
    MrmGraph *self = MRM_GRAPH (object);

    /* Additional property defaults */
    g_object_set (object,
                  "orientation",   GTK_ORIENTATION_VERTICAL,
                  "spacing",       6,
                  "margin-left",   8,
                  "margin-right",  8,
                  "margin-top",    8,
                  "margin-bottom", 8,
                  NULL);

    /* Setup title */
    self->priv->title_label = gtk_label_new ("");
    gtk_widget_set_halign (self->priv->title_label, GTK_ALIGN_START);
    gtk_widget_show (self->priv->title_label);
    update_graph_title (self);

    /* Setup drawing area */
    self->priv->drawing_area = gtk_drawing_area_new ();
    gtk_widget_set_events (self->priv->drawing_area, GDK_EXPOSURE_MASK);
    g_signal_connect (G_OBJECT (self->priv->drawing_area),
                      "draw",
                      G_CALLBACK (graph_draw),
                      self);
    g_signal_connect (G_OBJECT (self->priv->drawing_area),
                      "configure-event",
                      G_CALLBACK (graph_configure),
                      self);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start (self->priv->drawing_area, 8);
#else
    gtk_widget_set_margin_left (self->priv->drawing_area, 8);
#endif

    gtk_widget_set_margin_top (self->priv->drawing_area, 4);
    gtk_widget_set_margin_bottom (self->priv->drawing_area, 4);

    gtk_widget_show (self->priv->drawing_area);

    if (self->priv->legend_position == MRM_GRAPH_LEGEND_POSITION_NONE) {
        /* No legend, just title and graph */
        gtk_box_pack_start (GTK_BOX (self), self->priv->title_label, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (self), self->priv->drawing_area, TRUE, TRUE, 0);
    } else {
        /* Legend box */
        self->priv->legend_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_widget_set_hexpand (self->priv->legend_box, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start (self->priv->legend_box, 16);
#else
        gtk_widget_set_margin_left (self->priv->legend_box, 16);
#endif
        gtk_widget_set_margin_top (self->priv->legend_box, 4);
        gtk_widget_set_margin_bottom (self->priv->legend_box, 4);
        gtk_widget_show (self->priv->legend_box);

        if (self->priv->legend_position == MRM_GRAPH_LEGEND_POSITION_BOTTOM) {
            gtk_box_pack_start (GTK_BOX (self), self->priv->title_label, FALSE, TRUE, 0);
            gtk_box_pack_start (GTK_BOX (self), self->priv->drawing_area, TRUE, TRUE, 0);
            gtk_box_pack_start (GTK_BOX (self), self->priv->legend_box, FALSE, TRUE, 0);
        } else {
            GtkWidget *titlebox;

            /* Setup titlebox */
            titlebox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 20);
            gtk_box_pack_start (GTK_BOX (titlebox), self->priv->title_label, FALSE, TRUE, 0);
            gtk_box_pack_start (GTK_BOX (titlebox), self->priv->legend_box, FALSE, TRUE, 0);
            gtk_widget_show (titlebox);

            gtk_box_pack_start (GTK_BOX (self), titlebox, FALSE, TRUE, 0);
            gtk_box_pack_start (GTK_BOX (self), self->priv->drawing_area, TRUE, TRUE, 0);
        }
    }

    G_OBJECT_CLASS (mrm_graph_parent_class)->constructed (object);
}

static void
set_property (GObject *object,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
    MrmGraph *self = MRM_GRAPH (object);

    switch (prop_id) {
    case PROP_N_SERIES:
        free_series (self);
        self->priv->n_series = g_value_get_uint (value);
        allocate_series (self);
        break;
    case PROP_Y_MIN:
        self->priv->y_min = g_value_get_double (value);
        break;
    case PROP_Y_MAX:
        self->priv->y_max = g_value_get_double (value);
        break;
    case PROP_Y_UNITS:
        g_free (self->priv->y_units);
        self->priv->y_units = g_value_dup_string (value);
        break;
    case PROP_Y_N_SEPARATORS:
        self->priv->y_n_separators = g_value_get_uint (value);
        break;
    case PROP_TITLE:
        g_free (self->priv->title);
        self->priv->title = g_value_dup_string (value);
        update_graph_title (self);
        break;
    case PROP_LEGEND_POSITION:
        self->priv->legend_position = g_value_get_enum (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject *object,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
    MrmGraph *self = MRM_GRAPH (object);

    switch (prop_id) {
    case PROP_N_SERIES:
        g_value_set_uint (value, self->priv->n_series);
        break;
    case PROP_Y_MIN:
        g_value_set_double (value, self->priv->y_min);
        break;
    case PROP_Y_MAX:
        g_value_set_double (value, self->priv->y_max);
        break;
    case PROP_Y_UNITS:
        g_value_set_string (value, self->priv->y_units);
        break;
    case PROP_Y_N_SEPARATORS:
        g_value_set_uint (value, self->priv->y_n_separators);
        break;
    case PROP_TITLE:
        g_value_set_string (value, self->priv->title);
        break;
    case PROP_LEGEND_POSITION:
        g_value_set_enum (value, self->priv->legend_position);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
finalize (GObject *object)
{
    MrmGraph *self = MRM_GRAPH (object);

    if (self->priv->series)
        free_series (self);
    g_free (self->priv->y_units);
    g_free (self->priv->title);

    G_OBJECT_CLASS (mrm_graph_parent_class)->finalize (object);
}

static void
mrm_graph_class_init (MrmGraphClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (MrmGraphPrivate));

    object_class->constructed = constructed;
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->finalize = finalize;

    properties[PROP_N_SERIES] =
        g_param_spec_uint ("n-series",
                           "Number of series",
                           "Number of series to handle in the graph",
                           0,
                           G_MAXUINT,
                           0,
                           G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_N_SERIES, properties[PROP_N_SERIES]);

    properties[PROP_Y_MIN] =
        g_param_spec_double ("y-min",
                             "Minumum Y value",
                             "Minimum value in the Y axis",
                             -G_MAXDOUBLE,
                             G_MAXDOUBLE,
                             0.0,
                             G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_Y_MIN, properties[PROP_Y_MIN]);

    properties[PROP_Y_MAX] =
        g_param_spec_double ("y-max",
                             "Maximum Y value",
                             "Maximum value in the Y axis",
                             -G_MAXDOUBLE,
                             G_MAXDOUBLE,
                             100.0,
                             G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_Y_MAX, properties[PROP_Y_MAX]);

    properties[PROP_Y_UNITS] =
        g_param_spec_string ("y-units",
                             "Y units",
                             "Units in the Y axis",
                             "%",
                             G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_Y_UNITS, properties[PROP_Y_UNITS]);

    properties[PROP_Y_N_SEPARATORS] =
        g_param_spec_uint ("y-n-separators",
                           "Number of vertical separators",
                           "Number of horizontal lines splitting the graph in the Y axis",
                           1,
                           G_MAXUINT,
                           5,
                           G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_Y_N_SEPARATORS, properties[PROP_Y_N_SEPARATORS]);

    properties[PROP_TITLE] =
        g_param_spec_string ("title",
                             "Title",
                             "Graph title",
                             "",
                             G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_TITLE, properties[PROP_TITLE]);

    properties[PROP_LEGEND_POSITION] =
        g_param_spec_enum ("legend-position",
                           "Legend position",
                           "Whether the legend should be displayed, and where",
                           MRM_TYPE_GRAPH_LEGEND_POSITION,
                           MRM_GRAPH_LEGEND_POSITION_BOTTOM,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property (object_class, PROP_LEGEND_POSITION, properties[PROP_LEGEND_POSITION]);
}
