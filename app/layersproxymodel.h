/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/


#ifndef LAYERSPROXYMODEL_H
#define LAYERSPROXYMODEL_H

#include <QObject>

#include "inputconfig.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerproxymodel.h"
#include "qgsvectorlayer.h"
#include "inpututils.h"

#include "layersmodel.h"

enum LayerModelTypes
{
  ActiveLayerSelection,
  AllLayers
};

class LayersProxyModel : public QgsMapLayerProxyModel
{
    Q_OBJECT

    Q_PROPERTY( int count READ rowCount NOTIFY countChanged )

  public:
    LayersProxyModel( LayersModel *model, LayerModelTypes modelType = LayerModelTypes::AllLayers );

    bool filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const override;

    //! Helper methods that convert layer to/from index/name
    Q_INVOKABLE QModelIndex indexFromLayerId( QString layerId ) const;
    Q_INVOKABLE QgsVectorLayer *layerFromLayerId( QString layerId ) const;

    QgsVectorLayer *layerFromLayerName( const QString &layerName ) const;

    //! Helper method to get data from source model to skip converting indexes
    Q_INVOKABLE QVariant getData( QModelIndex index, int role ) const;

    //! Returns first layer from proxy model's layers list (filtered with filter function)
    Q_INVOKABLE QgsMapLayer *firstUsableLayer() const;

    //! Returns true if the layer is the position tracking layer
    Q_INVOKABLE bool isPositionTrackingLayer( QgsMapLayer *layer ) const;

    /**
     * @brief layers method return layers from source model filtered with filter function
     */
    QList<QgsMapLayer *> layers() const;

  signals:
    void countChanged();

  public slots:
    void refreshData();

  private:

    //! returns if input layer is capable of recording new features
    bool recordingAllowed( QgsMapLayer *layer ) const;

    //! filters if input layer is visible in current map theme
    bool layerVisible( QgsMapLayer *layer ) const;

    LayerModelTypes mModelType;
    LayersModel *mModel;

    /**
     * @brief filterFunction method takes layer and outputs if \bold this model type accepts layer.
     * Returns true for proxy model built without specific type or AllLayers type.
     *
     * In future will allow dependency injection of custom filter functions.
     */
    std::function<bool( QgsMapLayer * )> filterFunction;
};

#endif // LAYERSPROXYMODEL_H
