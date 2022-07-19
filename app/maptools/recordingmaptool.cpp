/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "recordingmaptool.h"

#include "qgsvectorlayer.h"
#include "qgspolygon.h"
#include "qgsvectorlayerutils.h"
#include "qgsmultipoint.h"
#include "qgsmultilinestring.h"

#include "position/positionkit.h"
#include "variablesmanager.h"
#include "inpututils.h"

RecordingMapTool::RecordingMapTool( QObject *parent )
  : AbstractMapTool{parent}
{
}

RecordingMapTool::~RecordingMapTool() = default;

void RecordingMapTool::addPoint( const QgsPoint &point )
{
  QgsPoint pointToAdd( point );

  if ( mPositionKit && ( mCenteredToGPS || mRecordingType == StreamMode ) )
  {
    // we want to use GPS point here instead of the point from map
    pointToAdd = mPositionKit->positionCoordinate();

    QgsPointXY transformed = InputUtils::transformPoint(
                               PositionKit::positionCRS(),
                               mLayer->sourceCrs(),
                               mLayer->transformContext(),
                               pointToAdd
                             );

    pointToAdd.setX( transformed.x() );
    pointToAdd.setY( transformed.y() );
  }

  fixZ( pointToAdd );

  mPoints.push_back( pointToAdd );
  rebuildGeometry();
}

void RecordingMapTool::removePoint()
{
  if ( !mPoints.isEmpty() )
  {
    mPoints.pop_back();
    rebuildGeometry();
  }
}

bool RecordingMapTool::hasValidGeometry() const
{
  if ( mLayer )
  {
    if ( mLayer->geometryType() == QgsWkbTypes::PointGeometry )
    {
      return mPoints.count() == 1;
    }
    else if ( mLayer->geometryType() == QgsWkbTypes::LineGeometry )
    {
      return mPoints.count() >= 2;
    }
    else if ( mLayer->geometryType() == QgsWkbTypes::PolygonGeometry )
    {
      return mPoints.count() >= 3;
    }
  }
  return false;
}

void RecordingMapTool::rebuildGeometry()
{
  if ( !mLayer )
    return;

  QgsGeometry geometry;

  if ( mPoints.count() < 1 )
  {
    // pass
  }
  else if ( mLayer->geometryType() == QgsWkbTypes::PointGeometry )
  {
    geometry = QgsGeometry( mPoints[0].clone() );
  }
  else if ( mLayer->geometryType() == QgsWkbTypes::LineGeometry )
  {
    geometry = QgsGeometry::fromPolyline( mPoints );
  }
  else if ( mLayer->geometryType() == QgsWkbTypes::PolygonGeometry )
  {
    QgsLineString *linestring = new QgsLineString;

    Q_FOREACH ( const QgsPoint &pt, mPoints )
      linestring->addVertex( pt );

    QgsPolygon *polygon = new QgsPolygon();
    polygon->setExteriorRing( linestring );
    geometry = QgsGeometry( polygon );
  }

  setRecordedGeometry( geometry );
}

void RecordingMapTool::fixZ( QgsPoint &point ) const
{
  if ( !mLayer )
    return;

  bool layerIs3D = QgsWkbTypes::hasZ( mLayer->wkbType() );
  bool pointIs3D = QgsWkbTypes::hasZ( point.wkbType() );

  if ( layerIs3D )
  {
    if ( !pointIs3D )
    {
      point.addZValue();
    }
  }
  else /* !layerIs3D */
  {
    if ( pointIs3D )
    {
      point.dropZValue();
    }
  }
}

void RecordingMapTool::onPositionChanged()
{
  if ( mRecordingType != StreamMode )
    return;

  if ( !mPositionKit || !mPositionKit->hasPosition() )
    return;

  if ( mLastTimeRecorded.addSecs( mRecordingInterval ) <= QDateTime::currentDateTime() )
  {
    addPoint( QgsPoint() ); // addPoint will take point from GPS
    mLastTimeRecorded = QDateTime::currentDateTime();
  }
  else
  {
    if ( !mPoints.isEmpty() )
    {
      // update the last point of the geometry
      // so that it is placed on user's current position
      QgsPoint position = mPositionKit->positionCoordinate();

      QgsPointXY transformed = InputUtils::transformPoint(
                                 PositionKit::positionCRS(),
                                 mLayer->sourceCrs(),
                                 mLayer->transformContext(),
                                 position
                               );

      mPoints.last().setX( transformed.x() );
      mPoints.last().setY( transformed.y() );
      mPoints.last().setZ( position.z() );

      rebuildGeometry();
    }
  }
}

// Getters / setters
bool RecordingMapTool::centeredToGPS() const
{
  return mCenteredToGPS;
}

void RecordingMapTool::setCenteredToGPS( bool newCenteredToGPS )
{
  if ( mCenteredToGPS == newCenteredToGPS )
    return;
  mCenteredToGPS = newCenteredToGPS;
  emit centeredToGPSChanged( mCenteredToGPS );
}

const RecordingMapTool::RecordingType &RecordingMapTool::recordingType() const
{
  return mRecordingType;
}

void RecordingMapTool::setRecordingType( const RecordingType &newRecordingType )
{
  if ( mRecordingType == newRecordingType )
    return;
  mRecordingType = newRecordingType;
  emit recordingTypeChanged( mRecordingType );
}

int RecordingMapTool::recordingInterval() const
{
  return mRecordingInterval;
}

void RecordingMapTool::setRecordingInterval( int newRecordingInterval )
{
  if ( mRecordingInterval == newRecordingInterval )
    return;
  mRecordingInterval = newRecordingInterval;
  emit recordingIntervalChanged( mRecordingInterval );
}

PositionKit *RecordingMapTool::positionKit() const
{
  return mPositionKit;
}

void RecordingMapTool::setPositionKit( PositionKit *newPositionKit )
{
  if ( mPositionKit == newPositionKit )
    return;

  if ( mPositionKit )
    disconnect( mPositionKit, nullptr, this, nullptr );

  mPositionKit = newPositionKit;

  if ( mPositionKit )
    connect( mPositionKit, &PositionKit::positionChanged, this, &RecordingMapTool::onPositionChanged );

  emit positionKitChanged( mPositionKit );
}

QgsVectorLayer *RecordingMapTool::layer() const
{
  return mLayer;
}

void RecordingMapTool::setLayer( QgsVectorLayer *newLayer )
{
  if ( mLayer == newLayer )
    return;
  mLayer = newLayer;
  emit layerChanged( mLayer );

  // we need to clear all recorded points and recalculate the geometry
  mPoints.clear();
  rebuildGeometry();
}

const QgsGeometry &RecordingMapTool::recordedGeometry() const
{
  return mRecordedGeometry;
}

void RecordingMapTool::setRecordedGeometry( const QgsGeometry &newRecordedGeometry )
{
  if ( mRecordedGeometry.equals( newRecordedGeometry ) )
    return;
  mRecordedGeometry = newRecordedGeometry;
  emit recordedGeometryChanged( mRecordedGeometry );
}

const QgsGeometry &RecordingMapTool::initialGeometry() const
{
  return mInitialGeometry;
}

void RecordingMapTool::setInitialGeometry( const QgsGeometry &newInitialGeometry )
{
  if ( mInitialGeometry.equals( newInitialGeometry ) )
    return;

  mInitialGeometry = newInitialGeometry;

  mPoints.clear();
  for ( auto pointIt = mInitialGeometry.vertices_begin(); pointIt != mInitialGeometry.vertices_end(); ++pointIt )
  {
    mPoints.push_back( QgsPoint( *pointIt ) );
  }

  emit initialGeometryChanged( mInitialGeometry );
}


QgsGeometry RecordingMapTool::extractGeometryVertices( const QgsGeometry &geometry )
{
  QgsMultiPoint *multiPoint = new QgsMultiPoint();
  QgsGeometry outputGeom( multiPoint );
  for ( auto pointIt = geometry.vertices_begin(); pointIt != geometry.vertices_end(); ++pointIt )
    multiPoint->addGeometry( ( *pointIt ).clone() );
  return outputGeom;
}

QgsGeometry RecordingMapTool::extractMidSegmentVertices( const QgsGeometry &geometry )
{
  if ( geometry.type() == QgsWkbTypes::PointGeometry )
  {
    return QgsGeometry();
  }

  QgsMultiPoint *multiPoint = new QgsMultiPoint();
  QgsGeometry outputGeom( multiPoint );
  QgsPoint p;

  const QVector< QgsLineString * > lines = QgsGeometryUtils::extractLineStrings( geometry.constGet() );
  for ( QgsLineString *line : lines )
  {
    for ( int i = 0; i < line->numPoints() - 1; ++i )
    {
      p = QgsGeometryUtils::midpoint( line->pointN( i ), line->pointN( i + 1 ) );
      multiPoint->addGeometry( p.clone() );
    }

    // if input geometry is a line we need to add virtual nodes and the beginning
    // and at the end of the line. They will be used to extend line
    if ( geometry.type() == QgsWkbTypes::LineGeometry && line->numPoints() >= 2 )
    {
      p = QgsGeometryUtils::interpolatePointOnLine( line->pointN( 0 ), line->pointN( 1 ), -0.1 );
      multiPoint->insertGeometry( p.clone(), 0 );
      p = QgsGeometryUtils::interpolatePointOnLine( line->pointN( line->numPoints() - 2 ), line->pointN( line->numPoints() - 1 ), 1.1 );
      multiPoint->addGeometry( p.clone() );
    }

    delete line;
  }
  return outputGeom;
}

QgsGeometry RecordingMapTool::createHandles( const QgsGeometry &geometry )
{
  if ( geometry.type() != QgsWkbTypes::LineGeometry )
  {
    return QgsGeometry();
  }

  QgsMultiLineString *multiLine = new QgsMultiLineString();
  QgsGeometry outputGeom( multiLine );
  QgsPoint p;
  QgsLineString handle;

  const QVector< QgsLineString * > lines = QgsGeometryUtils::extractLineStrings( geometry.constGet() );
  for ( QgsLineString *line : lines )
  {
    if ( line->numPoints() >= 2 )
    {
      p = QgsGeometryUtils::interpolatePointOnLine( line->pointN( 0 ), line->pointN( 1 ), -0.1 );
      handle = QgsLineString( p, line->pointN( 0 ) );
      multiLine->addGeometry( handle.clone() );

      p = QgsGeometryUtils::interpolatePointOnLine( line->pointN( line->numPoints() - 2 ), line->pointN( line->numPoints() - 1 ), 1.1 );
      handle = QgsLineString( line->pointN( line->numPoints() - 1 ), p );
      multiLine->addGeometry( handle.clone() );
    }
    delete line;
  }

  return outputGeom;
}
