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

#ifndef _MRM_ERROR_H_
#define _MRM_ERROR_H_

/* Prefixes for errors registered in DBus */
#define MRM_DBUS_ERROR_PREFIX          "es.aleksander.mrm.Error"
#define MRM_CORE_ERROR_DBUS_PREFIX     MRM_DBUS_ERROR_PREFIX ".Core"

typedef enum { /*< underscore_name=mrm_core_error >*/
    MRM_CORE_ERROR_FAILED           = 0, /*< nick=Failed >*/
    MRM_CORE_ERROR_INVALID_ARGS     = 1, /*< nick=InvalidArgs >*/
    MRM_CORE_ERROR_UNSUPPORTED      = 2  /*< nick=Unsupported >*/
} MrmCoreError;

#endif /* _MRM_ERROR_H_ */
