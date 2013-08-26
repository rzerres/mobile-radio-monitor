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

G_DEFINE_TYPE (MrmGraph, mrm_graph, G_TYPE_OBJECT)

struct _MrmGraphPrivate {
    gpointer dummy;
};

/*****************************************************************************/
/* New MRM graph */

MrmGraph *
mrm_graph_new (void)
{
    return MRM_GRAPH (g_object_new (MRM_TYPE_GRAPH, NULL));
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
