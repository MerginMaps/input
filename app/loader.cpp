/***************************************************************************
  app.h
  --------------------------------------
  Date                 : Nov 2017
  Copyright            : (C) 2017 by Peter Petrik
  Email                : peter.petrik@lutraconsulting.co.uk
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "loader.h"
#include "qgsvectorlayer.h"
#include <QDebug>

Loader::Loader(QObject* parent):QObject(parent)
{}

QgsProject* Loader::project() {
    return &mProject;
}

void Loader::setPositionKit(QgsQuickPositionKit *kit)
{
  mPositionKit = kit;
  emit positionKitChanged();
}

void Loader::setRecording(bool isRecordingOn)
{
    if (mRecording != isRecordingOn) {
        mRecording = isRecordingOn;
        emit recordingChanged();
    }
}

bool Loader::load(const QString& filePath) {
    qDebug() << "Loading " << filePath;
    bool res = true;
    if (mProject.fileName() != filePath) {
        res = mProject.read(filePath);
        emit projectReloaded();
    }

    return res;
}

void Loader::zoomToProject(QgsQuickMapSettings *mapSettings)
{
    if (!mapSettings) {
        qDebug() << "Cannot zoom to layers extent, mapSettings is not defined";
        return;
    }

    const QVector<QgsMapLayer*> layers = mProject.layers<QgsMapLayer*>();
    QgsRectangle extent;
    for (const QgsMapLayer* layer: layers) {
        QgsRectangle layerExtent = mapSettings->mapSettings().layerExtentToOutputExtent(layer, layer->extent());
        extent.combineExtentWith(layerExtent);
    }
    extent.scale(1.05);
    mapSettings->setExtent(extent);
}

QStringList Loader::mapTip(QgsQuickFeatureLayerPair pair)
{
    QgsExpressionContext context( QgsExpressionContextUtils::globalProjectLayerScopes( pair.layer() ) );
    QString mapTip = pair.layer()->mapTipTemplate();
    int LIMIT = 2;
    QStringList previewFields;

    QStringList fields;
    for (QgsField field: pair.layer()->fields()) {
        if (pair.layer()->displayField() != field.name()) {
            fields << field.name();
        }
    }

    for  (QString line: mapTip.split("\n")) {
        if (fields.indexOf(line) != -1) {
            previewFields << line;
        }
        if (previewFields.length() == LIMIT) return previewFields;
    }

    if (previewFields.empty()) {
        for (QgsField field: pair.layer()->fields()) {
            if (pair.layer()->displayField() != field.name()) {
                previewFields << field.name();
            }
            if (previewFields.length() == LIMIT) return previewFields;
        }
    }
    return previewFields;
}

void Loader::appStateChanged(Qt::ApplicationState state)
{
#if VERSION_INT >= 30500 // depends on https://github.com/qgis/QGIS/pull/8622
    if (!mRecording) {
        if (state == Qt::ApplicationActive) {
            mPositionKit->source()->startUpdates();
        } else {
            mPositionKit->source()->stopUpdates();
        }
    }
#else
    Q_UNUSED(state);
#endif
}
