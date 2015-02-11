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

#ifndef __MRM_POWER_TAB_H__
#define __MRM_POWER_TAB_H__

#include <gtk/gtk.h>

#include "mrm-device.h"

G_BEGIN_DECLS

#define MRM_TYPE_POWER_TAB         (mrm_power_tab_get_type ())
#define MRM_POWER_TAB(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MRM_TYPE_POWER_TAB, MrmPowerTab))
#define MRM_POWER_TAB_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), MRM_TYPE_POWER_TAB, MrmPowerTabClass))
#define MRM_IS_POWER_TAB(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MRM_TYPE_POWER_TAB))
#define MRM_IS_POWER_TAB_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MRM_TYPE_POWER_TAB))
#define MRM_POWER_TAB_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MRM_TYPE_POWER_TAB, MrmPowerTabClass))

typedef struct _MrmPowerTab        MrmPowerTab;
typedef struct _MrmPowerTabClass   MrmPowerTabClass;
typedef struct _MrmPowerTabPrivate MrmPowerTabPrivate;

struct _MrmPowerTab {
    GtkBox parent_instance;
    MrmPowerTabPrivate *priv;
};

struct _MrmPowerTabClass {
    GtkBoxClass parent_class;
};

GType mrm_power_tab_get_type (void) G_GNUC_CONST;

void mrm_power_tab_change_current_device (MrmPowerTab *self,
                                          MrmDevice *new_device);

G_END_DECLS

#endif /* __MRM_POWER_TAB_H__ */
