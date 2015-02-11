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
 */

#include <gtk/gtk.h>

#include "mrm-graph.h"

static GtkWidget *window;
static GtkWidget *graph;

static gboolean
update (gpointer unused)
{
    static gdouble value = -113.0;

    mrm_graph_step_init (MRM_GRAPH (graph));
    mrm_graph_step_set_value (MRM_GRAPH (graph), 0, value, NULL);
    mrm_graph_step_finish (MRM_GRAPH (graph));

    value += 1.0;
    if (value > -49.0)
        value = -113.0;
}

gint
main (gint argc, gchar **argv)
{
    gtk_init (&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    graph = mrm_graph_new ();
    g_object_set (graph,
                  "y-max",          -49.0,
                  "y-min",          -113.0,
                  "y-n-separators", 4,
                  "y-units",        "dBs",
                  "n-series",       1,
                  NULL);
    gtk_widget_show (graph);
    gtk_container_add (GTK_CONTAINER (window), graph);
    gtk_widget_show (window);

    mrm_graph_setup_series (MRM_GRAPH (graph), 0, "RSSI", 255, 0, 0);

    g_timeout_add_seconds (1, (GSourceFunc) update, NULL);

    gtk_main ();

    return 0;
}
