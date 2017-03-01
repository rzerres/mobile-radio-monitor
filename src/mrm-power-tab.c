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

#ifndef CMAKE_BUILD
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "mrm-power-tab.h"
#include "mrm-graph.h"
#include "mrm-color-icon.h"

struct _MrmPowerTabPrivate {
    MrmDevice *current;

    guint act_updated_id;

    GtkWidget *legend_gsm_box;
    GtkWidget *legend_gsm_icon;
    GtkWidget *legend_gsm_rx0_value_label;
    GtkWidget *legend_gsm_rx1_value_label;
    GtkWidget *legend_gsm_tx_value_label;

    GtkWidget *legend_umts_box;
    GtkWidget *legend_umts_icon;
    GtkWidget *legend_umts_rx0_value_label;
    GtkWidget *legend_umts_rx1_value_label;
    GtkWidget *legend_umts_tx_value_label;

    GtkWidget *legend_lte_box;
    GtkWidget *legend_lte_icon;
    GtkWidget *legend_lte_rx0_value_label;
    GtkWidget *legend_lte_rx1_value_label;
    GtkWidget *legend_lte_tx_value_label;

    GtkWidget *legend_cdma_box;
    GtkWidget *legend_cdma_icon;
    GtkWidget *legend_cdma_rx0_value_label;
    GtkWidget *legend_cdma_rx1_value_label;
    GtkWidget *legend_cdma_tx_value_label;

    GtkWidget *legend_evdo_box;
    GtkWidget *legend_evdo_icon;
    GtkWidget *legend_evdo_rx0_value_label;
    GtkWidget *legend_evdo_rx1_value_label;
    GtkWidget *legend_evdo_tx_value_label;

    GtkWidget *rx0_graph;
    GtkWidget *rx0_graph_frame;
    guint rx0_graph_updated_id;

    GtkWidget *rx1_graph;
    GtkWidget *rx1_graph_frame;
    guint rx1_graph_updated_id;

    GtkWidget *tx_graph;
    GtkWidget *tx_graph_frame;
    guint tx_graph_updated_id;
};

G_DEFINE_TYPE_WITH_PRIVATE (MrmPowerTab, mrm_power_tab, GTK_TYPE_BOX)

/******************************************************************************/

static void
act_updated (MrmDevice *device,
             MrmDeviceAct act,
             MrmPowerTab *self)
{
    gtk_widget_set_sensitive (self->priv->rx0_graph_frame, act != 0);
    gtk_widget_set_sensitive (self->priv->rx1_graph_frame, act != 0);
    gtk_widget_set_sensitive (self->priv->tx_graph_frame, act != 0);

    gtk_widget_set_sensitive (self->priv->legend_gsm_box,
                              (act & MRM_DEVICE_ACT_GSM));
    gtk_widget_set_sensitive (self->priv->legend_umts_box,
                              (act & MRM_DEVICE_ACT_UMTS));
    gtk_widget_set_sensitive (self->priv->legend_lte_box,
                              (act & MRM_DEVICE_ACT_LTE));
    gtk_widget_set_sensitive (self->priv->legend_cdma_box,
                              (act & MRM_DEVICE_ACT_CDMA));
    gtk_widget_set_sensitive (self->priv->legend_evdo_box,
                              (act & MRM_DEVICE_ACT_EVDO));
}

typedef enum {
    SERIES_RX0_GSM  = 0,
    SERIES_RX0_UMTS = 1,
    SERIES_RX0_LTE  = 2,
    SERIES_RX0_CDMA = 3,
    SERIES_RX0_EVDO = 4,
} SeriesRx0;

static void
rx0_updated (MrmDevice *device,
             gdouble gsm_rx0,
             gdouble umts_rx0,
             gdouble lte_rx0,
             gdouble cdma_rx0,
             gdouble evdo_rx0,
             MrmPowerTab *self)
{
    mrm_graph_step_init (MRM_GRAPH (self->priv->rx0_graph));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->rx0_graph),
                              SERIES_RX0_GSM,
                              gsm_rx0,
                              GTK_LABEL (self->priv->legend_gsm_rx0_value_label));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->rx0_graph),
                              SERIES_RX0_UMTS,
                              umts_rx0,
                              GTK_LABEL (self->priv->legend_umts_rx0_value_label));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->rx0_graph),
                              SERIES_RX0_LTE,
                              lte_rx0,
                              GTK_LABEL (self->priv->legend_lte_rx0_value_label));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->rx0_graph),
                              SERIES_RX0_CDMA,
                              cdma_rx0,
                              GTK_LABEL (self->priv->legend_cdma_rx0_value_label));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->rx0_graph),
                              SERIES_RX0_EVDO,
                              evdo_rx0,
                              GTK_LABEL (self->priv->legend_cdma_rx0_value_label));
    mrm_graph_step_finish (MRM_GRAPH (self->priv->rx0_graph));
}

typedef enum {
    SERIES_RX1_GSM  = 0,
    SERIES_RX1_UMTS = 1,
    SERIES_RX1_LTE  = 2,
    SERIES_RX1_CDMA = 3,
    SERIES_RX1_EVDO = 4,
} SeriesRx1;

static void
rx1_updated (MrmDevice *device,
             gdouble gsm_rx1,
             gdouble umts_rx1,
             gdouble lte_rx1,
             gdouble cdma_rx1,
             gdouble evdo_rx1,
             MrmPowerTab *self)
{
    mrm_graph_step_init (MRM_GRAPH (self->priv->rx1_graph));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->rx1_graph),
                              SERIES_RX1_GSM,
                              gsm_rx1,
                              GTK_LABEL (self->priv->legend_gsm_rx1_value_label));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->rx1_graph),
                              SERIES_RX1_UMTS,
                              umts_rx1,
                              GTK_LABEL (self->priv->legend_umts_rx1_value_label));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->rx1_graph),
                              SERIES_RX1_LTE,
                              lte_rx1,
                              GTK_LABEL (self->priv->legend_lte_rx1_value_label));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->rx1_graph),
                              SERIES_RX1_CDMA,
                              cdma_rx1,
                              GTK_LABEL (self->priv->legend_cdma_rx1_value_label));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->rx1_graph),
                              SERIES_RX1_EVDO,
                              evdo_rx1,
                              GTK_LABEL (self->priv->legend_cdma_rx1_value_label));
    mrm_graph_step_finish (MRM_GRAPH (self->priv->rx1_graph));
}

typedef enum {
    SERIES_TX_GSM  = 0,
    SERIES_TX_UMTS = 1,
    SERIES_TX_LTE  = 2,
    SERIES_TX_CDMA = 3,
    SERIES_TX_EVDO = 4,
} SeriesTx;

static void
tx_updated (MrmDevice *device,
            gdouble gsm_tx,
            gdouble umts_tx,
            gdouble lte_tx,
            gdouble cdma_tx,
            gdouble evdo_tx,
            MrmPowerTab *self)
{
    mrm_graph_step_init (MRM_GRAPH (self->priv->tx_graph));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->tx_graph),
                              SERIES_TX_GSM,
                              gsm_tx,
                              GTK_LABEL (self->priv->legend_gsm_tx_value_label));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->tx_graph),
                              SERIES_TX_UMTS,
                              umts_tx,
                              GTK_LABEL (self->priv->legend_umts_tx_value_label));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->tx_graph),
                              SERIES_TX_LTE,
                              lte_tx,
                              GTK_LABEL (self->priv->legend_lte_tx_value_label));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->tx_graph),
                              SERIES_TX_CDMA,
                              cdma_tx,
                              GTK_LABEL (self->priv->legend_cdma_tx_value_label));
    mrm_graph_step_set_value (MRM_GRAPH (self->priv->tx_graph),
                              SERIES_TX_EVDO,
                              evdo_tx,
                              GTK_LABEL (self->priv->legend_cdma_tx_value_label));
    mrm_graph_step_finish (MRM_GRAPH (self->priv->tx_graph));
}

void
mrm_power_tab_change_current_device (MrmPowerTab *self,
                                     MrmDevice *new_device)
{
    if (self->priv->current) {
        /* If same device, nothing else needed */
        if (new_device &&
            (self->priv->current == new_device ||
             g_str_equal (mrm_device_get_name (self->priv->current), mrm_device_get_name (new_device))))
            return;

        /* Changing current device, cleanup */
        if (self->priv->act_updated_id) {
            g_signal_handler_disconnect (self->priv->current, self->priv->act_updated_id);
            self->priv->act_updated_id = 0;
        }

        if (self->priv->rx0_graph_updated_id) {
            g_signal_handler_disconnect (self->priv->current, self->priv->rx0_graph_updated_id);
            self->priv->rx0_graph_updated_id = 0;
        }

        if (self->priv->rx1_graph_updated_id) {
            g_signal_handler_disconnect (self->priv->current, self->priv->rx1_graph_updated_id);
            self->priv->rx1_graph_updated_id = 0;
        }

        if (self->priv->tx_graph_updated_id) {
            g_signal_handler_disconnect (self->priv->current, self->priv->tx_graph_updated_id);
            self->priv->tx_graph_updated_id = 0;
        }

        g_clear_object (&self->priv->current);

        /* Clear graphs */

        mrm_graph_clear_series (MRM_GRAPH (self->priv->rx0_graph), SERIES_RX0_GSM);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->rx0_graph), SERIES_RX0_UMTS);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->rx0_graph), SERIES_RX0_LTE);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->rx0_graph), SERIES_RX0_CDMA);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->rx0_graph), SERIES_RX0_EVDO);

        mrm_graph_clear_series (MRM_GRAPH (self->priv->rx1_graph), SERIES_RX1_GSM);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->rx1_graph), SERIES_RX1_UMTS);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->rx1_graph), SERIES_RX1_LTE);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->rx1_graph), SERIES_RX1_CDMA);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->rx1_graph), SERIES_RX1_EVDO);

        mrm_graph_clear_series (MRM_GRAPH (self->priv->tx_graph), SERIES_TX_GSM);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->tx_graph), SERIES_TX_UMTS);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->tx_graph), SERIES_TX_LTE);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->tx_graph), SERIES_TX_CDMA);
        mrm_graph_clear_series (MRM_GRAPH (self->priv->tx_graph), SERIES_TX_EVDO);
    }

    if (new_device) {
        /* Keep a ref to current device */
        self->priv->current = g_object_ref (new_device);
        self->priv->act_updated_id = g_signal_connect (new_device,
                                                       "act-updated",
                                                       G_CALLBACK (act_updated),
                                                       self);
        self->priv->rx0_graph_updated_id = g_signal_connect (new_device,
                                                             "rx0-updated",
                                                             G_CALLBACK (rx0_updated),
                                                             self);
        self->priv->rx1_graph_updated_id = g_signal_connect (new_device,
                                                             "rx1-updated",
                                                             G_CALLBACK (rx1_updated),
                                                             self);
        self->priv->tx_graph_updated_id = g_signal_connect (new_device,
                                                            "tx-updated",
                                                            G_CALLBACK (tx_updated),
                                                            self);
    }
}

/******************************************************************************/

static void
mrm_power_tab_init (MrmPowerTab *self)
{
    self->priv = mrm_power_tab_get_instance_private (self);

    /* Ensure we register the MrmGraph and MrmColorIcon before initiating
     * the template */
    g_warn_if_fail (mrm_color_icon_get_type ());
    g_warn_if_fail (mrm_graph_get_type ());

    gtk_widget_init_template (GTK_WIDGET (self));

#define GSM_RGB  76,  153, 0   /* green */
#define UMTS_RGB 204, 0,   0   /* red */
#define LTE_RGB  0,   76,  153 /* blue */
#define CDMA_RGB 153, 153, 0   /* yellow */
#define EVDO_RGB 153, 0,   153 /* purple */

    /* Main legend box */
    mrm_color_icon_set_color (MRM_COLOR_ICON (self->priv->legend_gsm_icon),  GSM_RGB);
    mrm_color_icon_set_color (MRM_COLOR_ICON (self->priv->legend_umts_icon), UMTS_RGB);
    mrm_color_icon_set_color (MRM_COLOR_ICON (self->priv->legend_lte_icon),  LTE_RGB);
    mrm_color_icon_set_color (MRM_COLOR_ICON (self->priv->legend_cdma_icon), CDMA_RGB);
    mrm_color_icon_set_color (MRM_COLOR_ICON (self->priv->legend_evdo_icon), EVDO_RGB);

    /* RX0 graph */
    mrm_graph_setup_series (MRM_GRAPH (self->priv->rx0_graph), SERIES_RX0_GSM,  "GSM",  GSM_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->rx0_graph), SERIES_RX0_UMTS, "UMTS", UMTS_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->rx0_graph), SERIES_RX0_LTE,  "LTE",  LTE_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->rx0_graph), SERIES_RX0_CDMA, "CDMA", CDMA_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->rx0_graph), SERIES_RX0_EVDO, "EVDO", EVDO_RGB);

    /* RX1 graph */
    mrm_graph_setup_series (MRM_GRAPH (self->priv->rx1_graph), SERIES_RX1_GSM,  "GSM",  GSM_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->rx1_graph), SERIES_RX1_UMTS, "UMTS", UMTS_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->rx1_graph), SERIES_RX1_LTE,  "LTE",  LTE_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->rx1_graph), SERIES_RX1_CDMA, "CDMA", CDMA_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->rx1_graph), SERIES_RX1_EVDO, "EVDO", EVDO_RGB);

    /* TX graph */
    mrm_graph_setup_series (MRM_GRAPH (self->priv->tx_graph), SERIES_TX_GSM,  "GSM",  GSM_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->tx_graph), SERIES_TX_UMTS, "UMTS", UMTS_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->tx_graph), SERIES_TX_LTE,  "LTE",  LTE_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->tx_graph), SERIES_TX_CDMA, "CDMA", CDMA_RGB);
    mrm_graph_setup_series (MRM_GRAPH (self->priv->tx_graph), SERIES_TX_EVDO, "EVDO", EVDO_RGB);
}

static void
dispose (GObject *object)
{
    MrmPowerTab *self = MRM_POWER_TAB (object);

    g_clear_object (&self->priv->current);

    G_OBJECT_CLASS (mrm_power_tab_parent_class)->dispose (object);
}

static void
mrm_power_tab_class_init (MrmPowerTabClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->dispose = dispose;

    /* Bind class to template */
    gtk_widget_class_set_template_from_resource  (widget_class, "/es/aleksander/mrm/mrm-power-tab.ui");
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, rx0_graph);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, rx1_graph);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, tx_graph);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, rx0_graph_frame);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, rx1_graph_frame);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, tx_graph_frame);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_gsm_box);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_umts_box);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_lte_box);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_cdma_box);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_evdo_box);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_gsm_icon);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_umts_icon);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_lte_icon);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_cdma_icon);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_evdo_icon);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_gsm_rx0_value_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_umts_rx0_value_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_lte_rx0_value_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_cdma_rx0_value_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_evdo_rx0_value_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_gsm_rx1_value_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_umts_rx1_value_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_lte_rx1_value_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_cdma_rx1_value_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_evdo_rx1_value_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_gsm_tx_value_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_umts_tx_value_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_lte_tx_value_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_cdma_tx_value_label);
    gtk_widget_class_bind_template_child_private (widget_class, MrmPowerTab, legend_evdo_tx_value_label);

}
