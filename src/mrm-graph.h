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

#ifndef _MRM_GRAPH_H_
#define _MRM_GRAPH_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MRM_TYPE_GRAPH            (mrm_graph_get_type ())
#define MRM_GRAPH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MRM_TYPE_GRAPH, MrmGraph))
#define MRM_GRAPH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MRM_TYPE_GRAPH, MrmGraphClass))
#define MRM_IS_GRAPH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MRM_TYPE_GRAPH))
#define MRM_IS_GRAPH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MRM_TYPE_GRAPH))
#define MRM_GRAPH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MRM_TYPE_GRAPH, MrmGraphClass))

typedef struct _MrmGraph MrmGraph;
typedef struct _MrmGraphClass MrmGraphClass;
typedef struct _MrmGraphPrivate MrmGraphPrivate;

/**
 * MrmGraph:
 *
 * The #MrmGraph structure contains private data and should only be accessed
 * using the provided API.
 */
struct _MrmGraph {
    /*< private >*/
    GtkBox parent;
    MrmGraphPrivate *priv;
};

struct _MrmGraphClass {
    /*< private >*/
    GtkBoxClass parent;
};

GType mrm_graph_get_type (void);

GtkWidget *mrm_graph_new (void);

G_END_DECLS

#endif /* _MRM_GRAPH_H_ */
