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

#include "mrm-device.h"
#include "mrm-error.h"
#include "mrm-error-types.h"

static void async_initable_iface_init (GAsyncInitableIface *iface);

G_DEFINE_TYPE_EXTENDED (MrmDevice, mrm_device, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, async_initable_iface_init))

enum {
    PROP_0,
    PROP_FILE,
    PROP_QMI_DEVICE,
    PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

struct _MrmDevicePrivate {
    /* QMI device */
    GFile *file;
    QmiDevice *qmi_device;

    /* Device IDs */
    gchar *name;
    gchar *manufacturer;
    gchar *model;
    gchar *revision;

    /* NAS client */
    QmiClient *nas;
};

/*****************************************************************************/
/* Start NAS service monitoring */

typedef struct {
    MrmDevice *self;
    GSimpleAsyncResult *result;
} StartNasContext;

static void
start_nas_context_complete_and_free (StartNasContext *ctx)
{
    g_simple_async_result_complete_in_idle (ctx->result);
    g_object_unref (ctx->result);
    g_object_unref (ctx->self);
    g_slice_free (StartNasContext, ctx);
}

gboolean
mrm_device_start_nas_finish (MrmDevice *self,
                             GAsyncResult *res,
                             GError **error)
{
    return !g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error);
}

static void
qmi_device_allocate_client_nas_ready (QmiDevice *device,
                                      GAsyncResult *res,
                                      StartNasContext *ctx)
{
    GError *error = NULL;
    QmiMessageNasRegisterIndicationsInput *input;

    ctx->self->priv->nas = qmi_device_allocate_client_finish (device, res, &error);
    if (!ctx->self->priv->nas) {
        g_prefix_error (&error, "Cannot allocate NAS client: ");
        g_simple_async_result_take_error (ctx->result, error);
        start_nas_context_complete_and_free (ctx);
        return;
    }

    g_simple_async_result_set_op_res_gboolean (ctx->result, TRUE);
    start_nas_context_complete_and_free (ctx);
}

void
mrm_device_start_nas (MrmDevice *self,
                      GAsyncReadyCallback callback,
                      gpointer user_data)
{
    StartNasContext *ctx;

    ctx = g_slice_new (StartNasContext);
    ctx->self = g_object_ref (self);
    ctx->result = g_simple_async_result_new (G_OBJECT (self),
                                             callback,
                                             user_data,
                                             mrm_device_start_nas);

    /* If already started, error */
    if (self->priv->nas) {
        g_simple_async_result_set_error (ctx->result,
                                         MRM_CORE_ERROR,
                                         MRM_CORE_ERROR_FAILED,
                                         "NAS service already started");
        start_nas_context_complete_and_free (ctx);
        return;
    }

    qmi_device_allocate_client (self->priv->qmi_device,
                                QMI_SERVICE_NAS,
                                QMI_CID_NONE,
                                5,
                                NULL,
                                (GAsyncReadyCallback) qmi_device_allocate_client_nas_ready,
                                ctx);
}

/*****************************************************************************/

QmiDevice *
mrm_device_peek_qmi_device (MrmDevice *self)
{
    g_return_val_if_fail (MRM_IS_DEVICE (self), NULL);

    return self->priv->qmi_device;
}

const gchar *
mrm_device_get_manufacturer (MrmDevice *self)
{
    g_return_val_if_fail (MRM_IS_DEVICE (self), NULL);

    return self->priv->manufacturer;
}

const gchar *
mrm_device_get_model (MrmDevice *self)
{
    g_return_val_if_fail (MRM_IS_DEVICE (self), NULL);

    return self->priv->model;
}

const gchar *
mrm_device_get_revision (MrmDevice *self)
{
    g_return_val_if_fail (MRM_IS_DEVICE (self), NULL);

    return self->priv->revision;
}

const gchar *
mrm_device_get_name (MrmDevice *self)
{
    g_return_val_if_fail (MRM_IS_DEVICE (self), NULL);

    return self->priv->name;
}

/*****************************************************************************/
/* New MRM device */

MrmDevice *
mrm_device_new_finish (GAsyncResult *res,
                       GError **error)
{
  GObject *ret;
  GObject *source_object;

  source_object = g_async_result_get_source_object (res);
  ret = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object), res, error);
  g_object_unref (source_object);

  return (ret ? MRM_DEVICE (ret) : NULL);
}

void
mrm_device_new (GFile *file,
                GCancellable *cancellable,
                GAsyncReadyCallback callback,
                gpointer user_data)
{
    g_async_initable_new_async (MRM_TYPE_DEVICE,
                                G_PRIORITY_DEFAULT,
                                cancellable,
                                callback,
                                user_data,
                                "file", file,
                                NULL);
}

/*****************************************************************************/
/* Async init */

typedef struct {
    MrmDevice *self;
    GSimpleAsyncResult *result;
    GCancellable *cancellable;
    QmiClient *dms;
} InitContext;

static void
init_context_complete_and_free (InitContext *ctx)
{
    g_simple_async_result_complete_in_idle (ctx->result);
    if (ctx->cancellable)
        g_object_unref (ctx->cancellable);
    if (ctx->dms) {
        qmi_device_release_client (ctx->self->priv->qmi_device,
                                   ctx->dms,
                                   QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
                                   5,
                                   NULL,
                                   NULL,
                                   NULL);
        g_object_unref (ctx->dms);
    }
    g_object_unref (ctx->result);
    g_object_unref (ctx->self);
    g_slice_free (InitContext, ctx);
}

static gboolean
initable_init_finish (GAsyncInitable  *initable,
                      GAsyncResult    *result,
                      GError         **error)
{
    return !g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error);
}

static void
qmi_client_dms_get_revision_ready (QmiClientDms *dms,
                                   GAsyncResult *res,
                                   InitContext *ctx)
{
    QmiMessageDmsGetRevisionOutput *output;
    GError *error = NULL;
    const gchar *str;

    output = qmi_client_dms_get_revision_finish (dms, res, &error);
    if (!output ||
        !qmi_message_dms_get_revision_output_get_result (output, &error) ||
        !qmi_message_dms_get_revision_output_get_revision (output, &str, &error)) {
        g_prefix_error (&error, "Cannot get revision: ");
        g_simple_async_result_take_error (ctx->result, error);
    } else {
        ctx->self->priv->revision = g_strdup (str);
        /* All done! */
        g_simple_async_result_set_op_res_gboolean (ctx->result, TRUE);
    }

    if (output)
        qmi_message_dms_get_revision_output_unref (output);

    init_context_complete_and_free (ctx);
}

static void
qmi_client_dms_get_model_ready (QmiClientDms *dms,
                                GAsyncResult *res,
                                InitContext *ctx)
{
    QmiMessageDmsGetModelOutput *output;
    GError *error = NULL;
    const gchar *str;

    output = qmi_client_dms_get_model_finish (dms, res, &error);
    if (!output ||
        !qmi_message_dms_get_model_output_get_result (output, &error) ||
        !qmi_message_dms_get_model_output_get_model (output, &str, &error)) {
        g_prefix_error (&error, "Cannot get model: ");
        g_simple_async_result_take_error (ctx->result, error);
        init_context_complete_and_free (ctx);
    } else {
        ctx->self->priv->model = g_strdup (str);
        qmi_client_dms_get_revision (QMI_CLIENT_DMS (ctx->dms),
                                     NULL,
                                     5,
                                     ctx->cancellable,
                                     (GAsyncReadyCallback) qmi_client_dms_get_revision_ready,
                                     ctx);
    }

    if (output)
        qmi_message_dms_get_model_output_unref (output);
}

static void
qmi_client_dms_get_manufacturer_ready (QmiClientDms *dms,
                                       GAsyncResult *res,
                                       InitContext *ctx)
{
    QmiMessageDmsGetManufacturerOutput *output;
    GError *error = NULL;
    const gchar *str;

    output = qmi_client_dms_get_manufacturer_finish (dms, res, &error);
    if (!output ||
        !qmi_message_dms_get_manufacturer_output_get_result (output, &error) ||
        !qmi_message_dms_get_manufacturer_output_get_manufacturer (output, &str, &error)) {
        g_prefix_error (&error, "Cannot get manufacturer: ");
        g_simple_async_result_take_error (ctx->result, error);
        init_context_complete_and_free (ctx);
    } else {
        ctx->self->priv->manufacturer = g_strdup (str);
        qmi_client_dms_get_model (QMI_CLIENT_DMS (ctx->dms),
                                  NULL,
                                  5,
                                  ctx->cancellable,
                                  (GAsyncReadyCallback) qmi_client_dms_get_model_ready,
                                  ctx);
    }

    if (output)
        qmi_message_dms_get_manufacturer_output_unref (output);
}

static void
qmi_device_allocate_client_ready (QmiDevice *device,
                                  GAsyncResult *res,
                                  InitContext *ctx)
{
    GError *error = NULL;

    ctx->dms = qmi_device_allocate_client_finish (device, res, &error);
    if (!ctx->dms) {
        g_prefix_error (&error, "Cannot allocate DMS client: ");
        g_simple_async_result_take_error (ctx->result, error);
        init_context_complete_and_free (ctx);
        return;
    }

    g_debug ("DMS client at '%s' correctly allocated", qmi_device_get_path_display (device));

    qmi_client_dms_get_manufacturer (QMI_CLIENT_DMS (ctx->dms),
                                     NULL,
                                     5,
                                     ctx->cancellable,
                                     (GAsyncReadyCallback) qmi_client_dms_get_manufacturer_ready,
                                     ctx);
}

static void
qmi_device_open_ready (QmiDevice *device,
                       GAsyncResult *res,
                       InitContext *ctx)
{
    GError *error = NULL;

    if (!qmi_device_open_finish (device, res, &error)) {
        g_prefix_error (&error, "Cannot open QMI device file: ");
        g_simple_async_result_take_error (ctx->result, error);
        init_context_complete_and_free (ctx);
        return;
    }

    g_debug ("QMI device at '%s' correctly opened", qmi_device_get_path_display (device));

    qmi_device_allocate_client (device,
                                QMI_SERVICE_DMS,
                                QMI_CID_NONE,
                                5,
                                ctx->cancellable,
                                (GAsyncReadyCallback) qmi_device_allocate_client_ready,
                                ctx);
}

static void
qmi_device_new_ready (GObject *source,
                      GAsyncResult *res,
                      InitContext *ctx)
{
    GError *error = NULL;

    ctx->self->priv->qmi_device = qmi_device_new_finish (res, &error);
    if (!ctx->self->priv->qmi_device) {
        g_prefix_error (&error, "Cannot access QMI device file: ");
        g_simple_async_result_take_error (ctx->result, error);
        init_context_complete_and_free (ctx);
        return;
    }

    g_debug ("QMI device at '%s' correctly created",
             qmi_device_get_path_display (ctx->self->priv->qmi_device));

    qmi_device_open (ctx->self->priv->qmi_device,
                     (QMI_DEVICE_OPEN_FLAGS_PROXY |
                      QMI_DEVICE_OPEN_FLAGS_VERSION_INFO),
                     5,
                     ctx->cancellable,
                     (GAsyncReadyCallback) qmi_device_open_ready,
                     ctx);
}


static void
initable_init_async (GAsyncInitable *initable,
                     int io_priority,
                     GCancellable *cancellable,
                     GAsyncReadyCallback callback,
                     gpointer user_data)
{
    InitContext *ctx;

    ctx = g_slice_new0 (InitContext);
    ctx->self = g_object_ref (initable);
    if (cancellable)
        ctx->cancellable = g_object_ref (cancellable);
    ctx->result = g_simple_async_result_new (G_OBJECT (initable),
                                             callback,
                                             user_data,
                                             initable_init_async);

    /* We need a proper file to initialize */
    if (!ctx->self->priv->file) {
        g_simple_async_result_set_error (ctx->result,
                                         MRM_CORE_ERROR,
                                         MRM_CORE_ERROR_INVALID_ARGS,
                                         "Cannot initialize MRM device: No file given");
        init_context_complete_and_free (ctx);
        return;
    }

    /* Create QMI device */
    qmi_device_new (ctx->self->priv->file,
                    ctx->cancellable,
                    (GAsyncReadyCallback) qmi_device_new_ready,
                    ctx);
}

/*****************************************************************************/

static void
set_property (GObject *object,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
    MrmDevice *self = MRM_DEVICE (object);

    switch (prop_id) {
    case PROP_FILE:
        g_assert (self->priv->file == NULL);
        g_assert (self->priv->qmi_device == NULL);
        self->priv->file = g_value_dup_object (value);
        if (self->priv->file)
            self->priv->name = g_file_get_basename (self->priv->file);
        break;
    case PROP_QMI_DEVICE:
        g_assert_not_reached ();
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
    MrmDevice *self = MRM_DEVICE (object);

    switch (prop_id) {
    case PROP_FILE:
        g_value_set_object (value, self->priv->file);
        break;
    case PROP_QMI_DEVICE:
        g_value_set_object (value, self->priv->qmi_device);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
mrm_device_init (MrmDevice *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MRM_TYPE_DEVICE, MrmDevicePrivate);
}

static void
dispose (GObject *object)
{
    MrmDevice *self = MRM_DEVICE (object);

    if (self->priv->nas) {
        qmi_device_release_client (self->priv->qmi_device,
                                   self->priv->nas,
                                   QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
                                   5,
                                   NULL,
                                   NULL,
                                   NULL);
        g_clear_object (&self->priv->nas);
    }

    g_clear_object (&self->priv->file);
    if (self->priv->qmi_device)
        qmi_device_close (self->priv->qmi_device, NULL);
    g_clear_object (&self->priv->qmi_device);

    G_OBJECT_CLASS (mrm_device_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
    MrmDevice *self = MRM_DEVICE (object);

    g_free (self->priv->name);
    g_free (self->priv->manufacturer);
    g_free (self->priv->model);
    g_free (self->priv->revision);

    G_OBJECT_CLASS (mrm_device_parent_class)->finalize (object);
}

static void
async_initable_iface_init (GAsyncInitableIface *iface)
{
    iface->init_async = initable_init_async;
    iface->init_finish = initable_init_finish;
}

static void
mrm_device_class_init (MrmDeviceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (MrmDevicePrivate));

    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->finalize = finalize;
    object_class->dispose = dispose;

    properties[PROP_FILE] =
        g_param_spec_object ("file",
                             "File",
                             "File to the underlying QMI port",
                             G_TYPE_FILE,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property (object_class, PROP_FILE, properties[PROP_FILE]);

    properties[PROP_QMI_DEVICE] =
        g_param_spec_object ("qmi-device",
                             "QMI Device",
                             "QMI Device to the underlying QMI port",
                             QMI_TYPE_DEVICE,
                             G_PARAM_READABLE);
    g_object_class_install_property (object_class, PROP_QMI_DEVICE, properties[PROP_QMI_DEVICE]);
}
