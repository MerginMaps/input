/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "layertreesortfiltermodel.h"
#include "qgslayertree.h"

LayerTreeSortFilterModel::LayerTreeSortFilterModel( QObject *parent )
  : QSortFilterProxyModel{parent}
{
  setFilterCaseSensitivity( Qt::CaseInsensitive );
}

bool LayerTreeSortFilterModel::filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const
{
  if ( mLayerTreeModel )
  {
    QModelIndex modelIndex = mLayerTreeModel->index( source_row, 0, source_parent );

    QgsLayerTreeNode *node = mLayerTreeModel->index2node( modelIndex );

    if ( node && QgsLayerTree::isLayer( node ) )
    {
      QgsLayerTreeLayer *layerLeaf = QgsLayerTree::toLayer( node );

      if ( layerLeaf )
      {
        QgsMapLayer *layer = layerLeaf->layer();

        if ( layer )
        {
          bool isPrivate = layer->flags() & QgsMapLayer::LayerFlag::Private;

          if ( isPrivate )
          {
            return false;
          }
        }
      }
    }
  }

  return QSortFilterProxyModel::filterAcceptsRow( source_row, source_parent );
}

LayerTreeModel *LayerTreeSortFilterModel::layerTreeModel() const
{
  return mLayerTreeModel;
}

void LayerTreeSortFilterModel::setLayerTreeModel( LayerTreeModel *newLayerTreeModel )
{
  if ( mLayerTreeModel )
  {
    disconnect( mLayerTreeModel );
  }

  if ( mLayerTreeModel != newLayerTreeModel )
  {
    mLayerTreeModel = newLayerTreeModel;
    emit layerTreeModelChanged( mLayerTreeModel );
  }

  if ( mLayerTreeModel )
  {
    setSourceModel( mLayerTreeModel );
    connect( mLayerTreeModel, &LayerTreeModel::modelInitialized, this, &LayerTreeSortFilterModel::onSourceModelInitialized );
  }
}

LayerTreeSortFilterModel::~LayerTreeSortFilterModel() = default;

const QString &LayerTreeSortFilterModel::searchExpression() const
{
  return mSearchExpression;
}

void LayerTreeSortFilterModel::setSearchExpression( const QString &newSearchExpression )
{
  if ( mSearchExpression == newSearchExpression )
    return;
  mSearchExpression = newSearchExpression;
  emit searchExpressionChanged( mSearchExpression );

  setFilterFixedString( mSearchExpression );
}

QModelIndex LayerTreeSortFilterModel::getModelIndex( int row, int column, const QModelIndex &parent ) const
{
  return index( row, column, parent );
}

QModelIndex LayerTreeSortFilterModel::node2index( QgsLayerTreeNode *node ) const
{
  if ( !node )
  {
    return QModelIndex();
  }

  QModelIndex srcIndex = mLayerTreeModel->node2index( node );

  if ( !srcIndex.isValid() )
  {
    return QModelIndex();
  }

  return mapFromSource( srcIndex );
}

void LayerTreeSortFilterModel::onSourceModelInitialized()
{
  sort( 0 );
}
