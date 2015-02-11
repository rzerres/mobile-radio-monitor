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
 * Copyright (C) 2013-2015 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef __MRM_SIGNAL_TAB_H__
#define __MRM_SIGNAL_TAB_H__

#include <gtk/gtk.h>

#include "mrm-device.h"

G_BEGIN_DECLS

#define MRM_TYPE_SIGNAL_TAB         (mrm_signal_tab_get_type ())
#define MRM_SIGNAL_TAB(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MRM_TYPE_SIGNAL_TAB, MrmSignalTab))
#define MRM_SIGNAL_TAB_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), MRM_TYPE_SIGNAL_TAB, MrmSignalTabClass))
#define MRM_IS_SIGNAL_TAB(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MRM_TYPE_SIGNAL_TAB))
#define MRM_IS_SIGNAL_TAB_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MRM_TYPE_SIGNAL_TAB))
#define MRM_SIGNAL_TAB_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MRM_TYPE_SIGNAL_TAB, MrmSignalTabClass))

typedef struct _MrmSignalTab        MrmSignalTab;
typedef struct _MrmSignalTabClass   MrmSignalTabClass;
typedef struct _MrmSignalTabPrivate MrmSignalTabPrivate;

struct _MrmSignalTab {
    GtkBox parent_instance;
    MrmSignalTabPrivate *priv;
};

struct _MrmSignalTabClass {
    GtkBoxClass parent_class;
};

GType mrm_signal_tab_get_type (void) G_GNUC_CONST;

void mrm_signal_tab_change_current_device (MrmSignalTab *self,
                                           MrmDevice *new_device);

G_END_DECLS

#endif /* __MRM_SIGNAL_TAB_H__ */
