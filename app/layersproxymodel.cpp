/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsvectorlayer.h"
#include "layersproxymodel.h"

#include "qgsproject.h"
#include "qgslayertree.h"

LayersProxyModel::LayersProxyModel( LayersModel *model, LayerModelTypes modelType ) :
  mModelType( modelType ),
  mModel( model )
{
  setSourceModel( mModel );

  switch ( mModelType )
  {
    case ActiveLayerSelection:
      filterFunction = [this]( QgsMapLayer * layer ) { return recordingAllowed( layer ); };
      break;
    default:
      filterFunction = []( QgsMapLayer * ) { return true; };
      break;
  }

  QObject::connect( this, &LayersProxyModel::rowsInserted, this, &LayersProxyModel::countChanged );
  QObject::connect( this, &LayersProxyModel::rowsRemoved, this, &LayersProxyModel::countChanged );
}

bool LayersProxyModel::filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const
{
  if ( !QgsMapLayerProxyModel::filterAcceptsRow( source_row, source_parent ) )
    return false;

  // get layer from row and parent index
  QModelIndex index = mModel->index( source_row, 0, source_parent );
  QgsMapLayer *layer = mModel->layerFromIndex( index );

  return filterFunction( layer );
}

bool layerHasGeometry( const QgsVectorLayer *layer )
{
  if ( !layer || !layer->isValid() )
    return false;
  return layer->wkbType() != Qgis::WkbType::NoGeometry && layer->wkbType() != Qgis::WkbType::Unknown;
}

bool LayersProxyModel::recordingAllowed( QgsMapLayer *layer ) const
{
  if ( !layer || !layer->isValid() )
    return false;

  QgsVectorLayer *vectorLayer = qobject_cast<QgsVectorLayer *>( layer );
  return ( vectorLayer && !vectorLayer->readOnly() && layerHasGeometry( vectorLayer ) && layerVisible( layer ) && !isPositionTrackingLayer( layer ) );
}

bool LayersProxyModel::layerVisible( QgsMapLayer *layer ) const
{
  QgsLayerTree *root = QgsProject::instance()->layerTreeRoot();
  QgsLayerTreeLayer *layerTree = root->findLayer( layer );

  if ( layerTree )
    return layerTree->isVisible();

  return false;
}

QList<QgsMapLayer *> LayersProxyModel::layers() const
{
  QList<QgsMapLayer *> filteredLayers;

  if ( !mModel )
    return filteredLayers;

  QList<QgsMapLayer *> allLayers = mModel->layers();
  int layersCount = allLayers.size();

  for ( int i = 0; i < layersCount; i++ )
  {
    if ( filterFunction( allLayers.at( i ) ) )
      filteredLayers << allLayers.at( i );
  }
  return filteredLayers;
}

void LayersProxyModel::refreshData()
{
  invalidate();
}

QgsMapLayer *LayersProxyModel::firstUsableLayer() const
{
  QList<QgsMapLayer *> filteredLayers = layers();

  if ( filteredLayers.size() > 0 )
  {
    return filteredLayers.at( 0 );
  }

  return nullptr;
}

QModelIndex LayersProxyModel::indexFromLayerId( QString layerId ) const
{
  if ( layerId.isEmpty() )
    return QModelIndex();

  QgsVectorLayer *layer = layerFromLayerId( layerId );

  return mModel->indexFromLayer( layer ); // return source model index to skip converting indexes in proxy model
}

QgsVectorLayer *LayersProxyModel::layerFromLayerId( QString layerId ) const
{
  QList<QgsMapLayer *> filteredLayers = layers();

  for ( int i = 0; i < filteredLayers.count(); i++ )
  {
    if ( filteredLayers.at( i )->id() == layerId )
    {
      QgsVectorLayer *layer = qobject_cast<QgsVectorLayer *>( filteredLayers.at( i ) );
      if ( layer )
        return layer;
    }
  }
  return nullptr;
}

QgsVectorLayer *LayersProxyModel::layerFromLayerName( const QString &layerName ) const
{
  QList<QgsMapLayer *> filteredLayers = layers();

  for ( int i = 0; i < filteredLayers.count(); i++ )
  {
    if ( filteredLayers.at( i )->name() == layerName )
    {
      QgsVectorLayer *layer = qobject_cast<QgsVectorLayer *>( filteredLayers.at( i ) );
      if ( layer )
        return layer;
    }
  }
  return nullptr;
}

QVariant LayersProxyModel::getData( QModelIndex index, int role ) const
{
  return sourceModel()->data( index, role );
}

bool LayersProxyModel::isPositionTrackingLayer( QgsMapLayer *layer ) const
{
  if ( !layer )
    return false;

  QgsProject *project = QgsProject::instance();

  if ( !project )
    return false;

  QString trackingLayerId = project->readEntry( QStringLiteral( "Mergin" ), QStringLiteral( "PositionTracking/TrackingLayer" ), QString() );
  return layer->id() == trackingLayerId;
}
