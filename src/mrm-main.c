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

#include <gtk/gtk.h>

#include "mrm-graph.h"

gint main (gint argc, gchar **argv)
{
    GtkWidget *window;
    GtkWidget *graph;

    gtk_init (&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    graph = mrm_graph_new ();
    g_object_set (graph,
                  "y-max",          -49.0,
                  "y-min",          -113.0,
                  "y-n-separators", 4,
                  "y-units",        "dBs",
                  NULL);
    gtk_widget_show (graph);
    gtk_container_add (GTK_CONTAINER (window), graph);
    gtk_widget_show (window);

    gtk_main ();

    return 0;
}
