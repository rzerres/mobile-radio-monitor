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

#include <config.h>
#include <locale.h>
#include <gtk/gtk.h>

#include "mrm-app.h"

static void
activate_cb (GtkApplication *app)
{
    /* This is the primary instance */
    mrm_app_start (MRM_APP (app));
}

gint
main (gint argc, gchar **argv)
{
    MrmApp *app;
    gint status;

    setlocale (LC_ALL, "");

    app = mrm_app_new ();
    g_signal_connect (app, "activate", G_CALLBACK (activate_cb), NULL);

    /* Set it as the default application */
    g_application_set_default (G_APPLICATION (app));

    /* And run the GtkApplication */
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}
