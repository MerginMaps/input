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
#include "qgspolygon.h"
#include "qgsmultipolygon.h"
#include "qgsrendercontext.h"
#include "qgsvectorlayereditbuffer.h"

#include "position/positionkit.h"
#include "variablesmanager.h"

RecordingMapTool::RecordingMapTool( QObject *parent )
  : AbstractMapTool{parent}
{
  connect( this, &RecordingMapTool::initialGeometryChanged, this, &RecordingMapTool::prepareEditing );
  connect( this, &RecordingMapTool::recordedGeometryChanged, this, &RecordingMapTool::completeEditOperation );
  connect( this, &RecordingMapTool::recordedGeometryChanged, this, &RecordingMapTool::collectVertices );
  connect( this, &RecordingMapTool::activeVertexChanged, this, &RecordingMapTool::updateVisibleItems );
  connect( this, &RecordingMapTool::stateChanged, this, &RecordingMapTool::updateVisibleItems );
}

RecordingMapTool::~RecordingMapTool() = default;

void RecordingMapTool::addPoint( const QgsPoint &point )
{
  QgsPoint pointToAdd( point );

  if ( mPositionKit && ( mCenteredToGPS || mRecordingType == StreamMode ) )
  {
    // we want to use GPS point here instead of the point from map
    pointToAdd = mPositionKit->positionCoordinate();

    QgsPoint transformed = InputUtils::transformPoint(
                             PositionKit::positionCRS(),
                             mFeatureLayerPair.layer()->sourceCrs(),
                             mFeatureLayerPair.layer()->transformContext(),
                             pointToAdd
                           );

    pointToAdd.setX( transformed.x() );
    pointToAdd.setY( transformed.y() );
  }

  fixZ( pointToAdd );

  // start edit command
  mFeatureLayerPair.layer()->beginEditCommand( QStringLiteral( "Adding vertex" ) );

  QgsVertexId id( mActivePart, mActiveRing, 0 );
  if ( mRecordedGeometry.isEmpty() )
  {
    mRecordedGeometry = InputUtils::createGeometryForLayer( mFeatureLayerPair.layer() );
  }
  else
  {
    if ( mInsertPolicy == InsertPolicy::End )
    {
      id.vertex = mRecordedGeometry.constGet()->vertexCount( mActivePart, mActiveRing );
    }
  }

  if ( mRecordedGeometry.type() == QgsWkbTypes::PolygonGeometry )
  {
    // if it is a polygon and ring is not correctly defined yet (e.g. only
    // contains 1 point or not closed) we add point directly to the ring
    // and close it

    QgsLineString *r;
    const QgsPolygon *poly;

    if ( mRecordedGeometry.isMultipart() )
    {
      poly = qgsgeometry_cast<const QgsMultiPolygon *>( mRecordedGeometry.constGet() )->polygonN( mActivePart );
    }
    else
    {
      poly = qgsgeometry_cast<const QgsPolygon *>( mRecordedGeometry.constGet() );
    }

    if ( !poly )
    {
      return;
    }

    if ( mActiveRing == 0 )
    {
      r = qgsgeometry_cast<QgsLineString *>( poly->exteriorRing() );
    }
    else
    {
      // interior rings starts indexing from 0
      r = qgsgeometry_cast<QgsLineString *>( poly->interiorRing( mActiveRing - 1 ) );
    }

    if ( !r )
    {
      return;
    }

    // create part if all were removed and this is multipolygon geometry
    if ( mRecordedGeometry.isMultipart() && id.part >= mRecordedGeometry.constGet()->partCount() )
    {
      QgsLineString ring;
      ring.addVertex( pointToAdd );
      QgsPolygon poly( &ring );
      // ring will be closed automatically, bur we need to keep only one point,
      // so we remove end point
      QgsLineString *r = qgsgeometry_cast<QgsLineString *>( poly.exteriorRing() );
      if ( !r )
      {
        return;
      }
      QgsPointSequence points;
      r->points( points );
      points.removeLast();
      r->setPoints( points );
      mRecordedGeometry.addPart( poly.clone(), QgsWkbTypes::PolygonGeometry );
    }

    if ( r->nCoordinates() < 2 )
    {
      r->addVertex( pointToAdd );
      r->close();
      emit recordedGeometryChanged( mRecordedGeometry );
      return;
    }
    else
    {
      // as rings are closed, we need to insert before last vertex
      id.vertex = mRecordedGeometry.constGet()->vertexCount( mActivePart, mActiveRing ) - 1;
    }
  }

  if ( mRecordedGeometry.wkbType() == QgsWkbTypes::Point )
  {
    mRecordedGeometry.set( pointToAdd.clone() );
  }
  else if ( mRecordedGeometry.wkbType() == QgsWkbTypes::MultiPoint )
  {
    mRecordedGeometry.addPart( pointToAdd.clone() );
  }
  else
  {
    // create part if it does not exist
    if ( mRecordedGeometry.isMultipart() && id.part >= mRecordedGeometry.constGet()->partCount() )
    {
      QgsLineString line;
      line.addVertex( pointToAdd );
      mRecordedGeometry.addPart( line.clone(), QgsWkbTypes::LineGeometry );
    }
    else
    {
      mRecordedGeometry.get()->insertVertex( id, pointToAdd );
    }
  }

  emit recordedGeometryChanged( mRecordedGeometry );
}

void RecordingMapTool::addPointAtPosition( Vertex vertex, const QgsPoint &point )
{
  if ( vertex.isValid() )
  {
    // start edit command
    mFeatureLayerPair.layer()->beginEditCommand( QStringLiteral( "Adding vertex" ) );

    if ( mRecordedGeometry.get()->insertVertex( vertex.vertexId(), point ) )
    {
      emit recordedGeometryChanged( mRecordedGeometry );
    }
  }
}

void RecordingMapTool::removePoint()
{
  if ( mRecordedGeometry.isEmpty() )
  {
    return;
  }

  // start edit command
  mFeatureLayerPair.layer()->beginEditCommand( QStringLiteral( "Removing vertex" ) );

  if ( mState == MapToolState::Grab )
  {
    // we are removing existing vertex selected by ActiveVertex

    if ( !mActiveVertex.isValid() )
    {
      return;
    }

    QgsVertexId current = mActiveVertex.vertexId();

    if ( mRecordedGeometry.constGet()->vertexCount( current.part,  current.ring ) < 1 )
    {
      return;
    }

    if ( mRecordedGeometry.type() == QgsWkbTypes::PolygonGeometry )
    {
      QgsLineString *r;
      const QgsPolygon *poly;

      if ( mRecordedGeometry.isMultipart() )
      {
        poly = qgsgeometry_cast<const QgsMultiPolygon *>( mRecordedGeometry.constGet() )->polygonN( current.part );
      }
      else
      {
        poly = qgsgeometry_cast<const QgsPolygon *>( mRecordedGeometry.constGet() );
      }

      if ( !poly )
      {
        return;
      }

      if ( current.ring == 0 )
      {
        r = qgsgeometry_cast<QgsLineString *>( poly->exteriorRing() );
      }
      else
      {
        // interior rings starts indexing from 0
        r = qgsgeometry_cast<QgsLineString *>( poly->interiorRing( current.ring - 1 ) );
      }

      if ( !r )
      {
        return;
      }

      if ( r->nCoordinates() == 4 )
      {
        // this is the smallest possible closed ring (first and last vertex are equal),
        // we need to remove two last vertices in order to get correct linestring
        if ( current.vertex == 0 || current.vertex == r->nCoordinates() - 1 )
        {
          r->deleteVertex( QgsVertexId( 0, 0, 0 ) );
          r->deleteVertex( QgsVertexId( 0, 0, r->nCoordinates() - 1 ) );
        }
        else
        {
          r->deleteVertex( QgsVertexId( 0, 0, current.vertex ) );
          r->deleteVertex( QgsVertexId( 0, 0, r->nCoordinates() - 1 ) );
        }
      }
      else if ( r->nCoordinates() <= 2 )
      {
        // if we remove last vertex directly the geometry will be cleared
        // but we want to keep start point, so instead of removing vertex
        // from the linestring we remove item from the QgsPointSequence.
        QgsPointSequence points;
        r->points( points );

        points.removeAt( current.vertex );

        r->setPoints( points );
      }
      else
      {
        mRecordedGeometry.get()->deleteVertex( current );
      }
    }
    else if ( mRecordedGeometry.type() == QgsWkbTypes::LineGeometry )
    {
      QgsLineString *r;

      if ( mRecordedGeometry.isMultipart() )
      {
        QgsMultiLineString *ml = qgsgeometry_cast<QgsMultiLineString *>( mRecordedGeometry.get() );
        r = ml->lineStringN( current.part );
      }
      else
      {
        r = qgsgeometry_cast<QgsLineString *>( mRecordedGeometry.get() );
      }

      if ( !r )
      {
        return;
      }

      if ( r->nCoordinates() == 2 )
      {
        // if we remove second vertex directly the geometry will be cleared
        // but we want to keep start point, so instead of removing vertex
        // from the linestring we remove item from the QgsPointSequence.
        QgsPointSequence points;
        r->points( points );

        points.removeAt( current.vertex );
        r->setPoints( points );
      }
      else
      {
        mRecordedGeometry.get()->deleteVertex( current );
      }
    }
    else
    {
      // points / multipoints
      mRecordedGeometry.get()->deleteVertex( current );
    }

    emit recordedGeometryChanged( mRecordedGeometry );

    grabNextVertex();

  }
  else if ( mState == MapToolState::Record )
  {
    // select first/last existing vertex as active and change state to GRAB
    int nVertices = mRecordedGeometry.constGet()->vertexCount( mActivePart, mActiveRing );
    if ( nVertices < 1 )
    {
      return;
    }

    int vertexToGrab = nVertices - 1;

    if ( mRecordedGeometry.type() == QgsWkbTypes::PolygonGeometry )
    {
      if ( nVertices >= 4 )
      {
        // skip ring close vertex
        vertexToGrab = nVertices - 2;
      }
    }
    else if ( mInsertPolicy == InsertPolicy::Start )
    {
      vertexToGrab = 0;
    }

    QgsVertexId target( mActivePart, mActiveRing, vertexToGrab );
    QgsPoint targetPosition = mRecordedGeometry.constGet()->vertexAt( target );

    setActiveVertex( Vertex( target, targetPosition, Vertex::Existing ) );
    setState( MapToolState::Grab );
  }
}

bool RecordingMapTool::hasValidGeometry() const
{
  if ( mFeatureLayerPair.isValid() )
  {
    if ( mFeatureLayerPair.layer()->geometryType() == QgsWkbTypes::PointGeometry )
    {
      return mRecordedGeometry.constGet()->nCoordinates() == 1;
    }
    else if ( mFeatureLayerPair.layer()->geometryType() == QgsWkbTypes::LineGeometry )
    {
      if ( mRecordedGeometry.isMultipart() )
      {
        const QgsAbstractGeometry *geom = mRecordedGeometry.constGet();
        for ( auto it = geom->const_parts_begin(); it != geom->const_parts_end(); ++it )
        {
          if ( ( *it )->nCoordinates() < 2 )
          {
            return false;
          }
        }
        return true;
      }
      else
      {
        return mRecordedGeometry.constGet()->nCoordinates() >= 2;
      }
    }
    else if ( mFeatureLayerPair.layer()->geometryType() == QgsWkbTypes::PolygonGeometry )
    {
      if ( mRecordedGeometry.isMultipart() )
      {
        const QgsAbstractGeometry *geom = mRecordedGeometry.constGet();
        for ( auto it = geom->const_parts_begin(); it != geom->const_parts_end(); ++it )
        {
          if ( ( *it )->nCoordinates() < 4 )
          {
            return false;
          }
        }
        return true;
      }
      else
      {
        return mRecordedGeometry.constGet()->nCoordinates() >= 4;
      }
    }
  }
  return false;
}

void RecordingMapTool::fixZ( QgsPoint &point ) const
{
  if ( !mFeatureLayerPair.isValid() )
    return;

  bool layerIs3D = QgsWkbTypes::hasZ( mFeatureLayerPair.layer()->wkbType() );
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
    if ( !mRecordedGeometry.isEmpty() )
    {
      // update the last point of the geometry
      // so that it is placed on user's current position
      QgsPoint position = mPositionKit->positionCoordinate();

      QgsPointXY transformed = InputUtils::transformPoint(
                                 PositionKit::positionCRS(),
                                 mFeatureLayerPair.layer()->sourceCrs(),
                                 mFeatureLayerPair.layer()->transformContext(),
                                 position
                               );
      QgsPoint p( transformed.x(), transformed.y(), position.z() );
      QgsVertexId id( mActivePart, mActiveRing, mRecordedGeometry.constGet()->vertexCount() - 1 );
      mRecordedGeometry.get()->moveVertex( id, p );
      emit recordedGeometryChanged( mRecordedGeometry );
    }
  }
}

void RecordingMapTool::prepareEditing()
{
  if ( !mInitialGeometry.isEmpty() )
  {
    setState( MapToolState::View );
    setRecordedGeometry( mInitialGeometry );
  }
}

void RecordingMapTool::collectVertices()
{
  mVertices.clear();

  if ( mRecordedGeometry.isEmpty() )
  {
    updateVisibleItems();
    return;
  }

  QgsPoint vertex;
  QgsVertexId vertexId;
  const QgsAbstractGeometry *geom = mRecordedGeometry.constGet();

  int startPart = -1;
  int endPart = -1;

  /**
   * Extracts existing geometry vertices and generates virtual vertices representing
   * midpoints (for lines and polygons) and start/end handles (for lines).
   *
   * For lines each part extracted in the following order: start handle, first vertex,
   * midPointN, vertexN, …, last vertex, end handle. So, for simple line containing
   * just 3 points we will get the following sequence of vertices (h — handle,
   * v — exisiting vertex, m — midpoint):
   * LINE: A -> B -> C
   * VERTICES: hS, vA, mA, vB, mB, vC, hE
   *
   * Similarly for multiparts:
   * LINE: part1: A -> B -> C | part 2: D -> E | part 3: X
   * VERTICES: hS1, vA, mA, vB, mB, vC, hE1, hS2, vD, mD, vE, hE2, vX
   *
   * For polygons each part extracted in the following order: first vertex, midPointN,
   * vertexN, …. Last closing vertex (which has the same coordinates as the first one)
   * is ignored. Interior rings (holes) are extracted in the same way and go right
   * after the correspoinding inrerior ring. So for simple polygon containing 4 points
   * (first and last point are the same) we will get following sequence of vertices:
   * POLYGON: A -> B -> C -> A
   * VERTICES vA, mA, vB, mB, vC, mC
   *
   * Similarly for multiparts:
   * POLYGON: part1: A -> B -> C -> A, ring1: D -> E -> F ->D | part2: X -> Y -> Z -> X | part3: G -> H | part 4: J
   * VERTICES: vA, mA, vB, mB, vC, mC, vD, mD, vE, mE, vF, mF, vX, mX, vY, mY, vZ, mZ, vG, mG, vH, vJ
   */
  while ( geom->nextVertex( vertexId, vertex ) )
  {
    int vertexCount = geom->vertexCount( vertexId.part, vertexId.ring );

    if ( mRecordedGeometry.type() == QgsWkbTypes::PolygonGeometry )
    {
      if ( vertexCount == 1 )
      {
        // Edge case! Sometimes we want to have invalid polygon, which is in fact
        // single point, in such case we keep this point
        mVertices.push_back( Vertex( vertexId, vertex, Vertex::Existing ) );
        continue;
      }

      if ( vertexId.vertex < vertexCount - 1 )
      {
        // actual vertex
        mVertices.push_back( Vertex( vertexId, vertex, Vertex::Existing ) );

        QgsVertexId id( vertexId.part, vertexId.ring, vertexId.vertex + 1 );
        QgsPoint midPoint = QgsGeometryUtils::midpoint( geom->vertexAt( vertexId ), geom->vertexAt( id ) );
        mVertices.push_back( Vertex( id, midPoint, Vertex::MidPoint ) );
      }
      else if ( vertexCount == 2 && vertexId.vertex == vertexCount - 1 )
      {
        // Edge case! Sometimes we want to have invalid polygon, which is in fact
        // just a line segment, in such case last vertex should be kept
        mVertices.push_back( Vertex( vertexId, vertex, Vertex::Existing ) );
      }
      // ignore the closing vertex in polygon
      else if ( vertexId.vertex == vertexCount - 1 )
      {
        continue;
      }
    }
    else if ( mRecordedGeometry.type() == QgsWkbTypes::LineGeometry )
    {
      // if this is firt point in line (or part) we add handle start point first
      if ( vertexId.vertex == 0 && vertexId.part != startPart && vertexCount >= 2 )
      {
        // next line point. needed to get calculate handle point coordinates
        QgsVertexId id( vertexId.part, vertexId.ring, 1 );

        // start handle point
        QgsPoint handlePoint = QgsGeometryUtils::interpolatePointOnLine( geom->vertexAt( vertexId ), geom->vertexAt( id ), -0.5 );
        mVertices.push_back( Vertex( vertexId, handlePoint, Vertex::HandleStart ) );
        startPart = vertexId.part;
      }

      // add actual vertex and midpoint if this is not the last vertex of the line
      if ( vertexId.vertex < vertexCount - 1 )
      {
        // actual vertex
        mVertices.push_back( Vertex( vertexId, vertex, Vertex::Existing ) );

        // midpoint
        QgsVertexId id( vertexId.part, vertexId.ring, vertexId.vertex + 1 );
        QgsPoint midPoint = QgsGeometryUtils::midpoint( geom->vertexAt( vertexId ), geom->vertexAt( id ) );
        mVertices.push_back( Vertex( id, midPoint, Vertex::MidPoint ) );
      }

      // if this is last point in line (or part) we save actual vertex first
      // and then handle end point
      if ( vertexId.vertex == vertexCount - 1 && vertexId.part != endPart )
      {
        // last vertex of the line
        mVertices.push_back( Vertex( vertexId, vertex, Vertex::Existing ) );

        if ( vertexCount >= 2 )
        {
          // previous line point. needed to get calculate handle point coordinates
          QgsVertexId id( vertexId.part, vertexId.ring, vertexCount - 2 );

          // end handle point
          QgsPoint handlePoint = QgsGeometryUtils::interpolatePointOnLine( geom->vertexAt( id ), geom->vertexAt( vertexId ), 1.5 );
          mVertices.push_back( Vertex( vertexId, handlePoint, Vertex::HandleEnd ) );
          endPart = vertexId.part;
        }
      }
    }
    else
    {
      // for points and multipoints we just add existing vertices
      mVertices.push_back( Vertex( vertexId, vertex, Vertex::Existing ) );
    }
  }
  updateVisibleItems();
}

void RecordingMapTool::updateVisibleItems()
{
  QgsMultiPoint *existingVertices = new QgsMultiPoint();
  mExistingVertices.set( existingVertices );

  QgsMultiPoint *midPoints = new QgsMultiPoint();
  mMidPoints.set( midPoints );

  QgsMultiLineString *handles = new QgsMultiLineString();
  mHandles.set( handles );

  if ( mRecordedGeometry.isEmpty() )
  {
    emit existingVerticesChanged( mExistingVertices );
    emit midPointsChanged( mMidPoints );
    emit handlesChanged( mHandles );
    return;
  }

  Vertex v;
  for ( int i = 0; i < mVertices.count(); i++ )
  {
    v = mVertices.at( i );

    if ( v.type() == Vertex::Existing && v != mActiveVertex )
    {
      // show existing vertex if it is not an active one
      existingVertices->addGeometry( v.coordinates().clone() );
    }
    else if ( v.type() == Vertex::MidPoint )
    {
      midPoints->addGeometry( v.coordinates().clone() );
    }
    else if ( v.type() == Vertex::HandleStart )
    {
      // start handle is visible if we are not recording from start and first vertex is not active
      Vertex lineStart = mVertices.at( i + 1 );
      if ( !( mState == MapToolState::Record && mInsertPolicy == InsertPolicy::Start ) && mActiveVertex != lineStart )
      {
        // start handle point
        midPoints->addGeometry( v.coordinates().clone() );

        // start handle line
        QgsLineString handle( v.coordinates(), lineStart.coordinates() );
        handles->addGeometry( handle.clone() );
      }
    }
    else if ( v.type() == Vertex::HandleEnd )
    {
      // end handle is visible if we are not recording from end and last vertex is not active
      Vertex lineEnd = mVertices.at( i - 1 );
      if ( !( mState == MapToolState::Record && mInsertPolicy == InsertPolicy::End ) && mActiveVertex != lineEnd )
      {
        // end handle point
        midPoints->addGeometry( v.coordinates().clone() );

        // end handle line
        QgsLineString handle( lineEnd.coordinates(), v.coordinates() );
        handles->addGeometry( handle.clone() );
      }
    }
  }

  emit existingVerticesChanged( mExistingVertices );
  emit midPointsChanged( mMidPoints );
  emit handlesChanged( mHandles );
}

void RecordingMapTool::lookForVertex( const QPointF &clickedPoint, double searchRadius )
{
  double minDistance = std::numeric_limits<double>::max();
  double currentDistance = 0;
  double searchDistance = pixelsToMapUnits( searchRadius );

  QgsPoint pnt = mapSettings()->screenToCoordinate( clickedPoint );

  if ( mRecordedGeometry.isEmpty() )
  {
    return;
  }

  int idx = -1;
  for ( int i = 0; i < mVertices.count(); i++ )
  {
    QgsPoint vertex( mVertices.at( i ).coordinates() );
    vertex.transform( mapSettings()->mapSettings().layerTransform( mFeatureLayerPair.layer() ) );

    currentDistance = pnt.distance( vertex );
    if ( currentDistance < minDistance && currentDistance <= searchDistance )
    {
      minDistance = currentDistance;
      idx = i;
    }
  }

  // Update the previously grabbed point's position
  if ( mState == MapToolState::Grab )
  {
    updateVertex( mActiveVertex, mRecordPoint );
  }

  if ( idx >= 0 )
  {
    // we found a point
    Vertex clickedVertex = mVertices.at( idx );

    mActiveVertex = Vertex();

    if ( clickedVertex.type() == Vertex::Existing )
    {
      setActiveVertex( mVertices.at( idx ) );
      setState( MapToolState::Grab );
    }
    else if ( clickedVertex.type() == Vertex::MidPoint )
    {
      // We need to invalidate activeVertex so that
      // next construction of midpoints and handles contains all points
      addPointAtPosition( clickedVertex, clickedVertex.coordinates() );

      // After adding new point to the position, we need to
      // search again, because mVertices now includes more points.
      // Search should find the created vertex as Existing.
      return lookForVertex( clickedPoint, searchRadius );
    }
    else if ( clickedVertex.type() == Vertex::HandleStart )
    {
      setInsertPolicy( InsertPolicy::Start );
      setActivePartAndRing( clickedVertex.vertexId().part, clickedVertex.vertexId().ring );
      setState( MapToolState::Record );
    }
    else if ( clickedVertex.type() == Vertex::HandleEnd )
    {
      setInsertPolicy( InsertPolicy::End );
      setActivePartAndRing( clickedVertex.vertexId().part, clickedVertex.vertexId().ring );
      setState( MapToolState::Record );
    }

    emit activeVertexChanged( mActiveVertex );
  }
  else
  {
    // nothing found
    setState( MapToolState::View );
    setActivePartAndRing( 0, 0 );
    setActiveVertex( Vertex() );
  }
}

void RecordingMapTool::releaseVertex( const QgsPoint &point )
{
  if ( !mActiveVertex.isValid() )
  {
    return;
  }

  // start edit command
  mFeatureLayerPair.layer()->beginEditCommand( QStringLiteral( "Moving vertex" ) );

  int vertexCount = mRecordedGeometry.constGet()->vertexCount( mActiveVertex.vertexId().part, mActiveVertex.vertexId().ring );

  if ( mRecordedGeometry.type() == QgsWkbTypes::PolygonGeometry )
  {
    QgsPolygon *polygon;
    QgsLineString *ring;

    if ( mRecordedGeometry.isMultipart() )
    {
      QgsMultiPolygon *multiPolygon = qgsgeometry_cast<QgsMultiPolygon *>( mRecordedGeometry.get() );
      polygon = multiPolygon->polygonN( mActiveVertex.vertexId().part );
    }
    else
    {
      polygon = qgsgeometry_cast<QgsPolygon *>( mRecordedGeometry.get() );
    }

    if ( mActiveVertex.vertexId().ring == 0 )
    {
      ring = qgsgeometry_cast<QgsLineString *>( polygon->exteriorRing() );
    }
    else
    {
      ring = qgsgeometry_cast<QgsLineString *>( polygon->interiorRing( mActiveVertex.vertexId().ring - 1 ) );
    }

    if ( !ring )
    {
      return;
    }

    if ( vertexCount == 2 )
    {
      ring->close();
      emit recordedGeometryChanged( mRecordedGeometry );

      updateVertex( mActiveVertex, point );
      setState( MapToolState::Record );
      setActivePartAndRing( mActiveVertex.vertexId().part, mActiveVertex.vertexId().ring );
      setActiveVertex( Vertex() );
      return;
    }
    else if ( vertexCount == 1 )
    {
      updateVertex( mActiveVertex, point );
      setState( MapToolState::Record );
      setActivePartAndRing( mActiveVertex.vertexId().part, mActiveVertex.vertexId().ring );
      setActiveVertex( Vertex() );
      return;
    }
  }

  updateVertex( mActiveVertex, point );

  // if it is a first or last vertex of the line we go to the recording mode
  if ( mRecordedGeometry.type() == QgsWkbTypes::LineGeometry )
  {
    if ( mActiveVertex.type() == Vertex::Existing && mActiveVertex.vertexId().vertex == 0 )
    {
      // Note: Order matters - we rebuild visible geometry when active vertex is changed
      setInsertPolicy( InsertPolicy::Start );
      setState( MapToolState::Record );
      setActivePartAndRing( mActiveVertex.vertexId().part, mActiveVertex.vertexId().ring );
      setActiveVertex( Vertex() );
      return;
    }
    else if ( mActiveVertex.type() == Vertex::Existing && mActiveVertex.vertexId().vertex == vertexCount - 1 )
    {
      // Note: Order matters - we rebuild visible geometry when active vertex is changed
      setInsertPolicy( InsertPolicy::End );
      setState( MapToolState::Record );
      setActivePartAndRing( mActiveVertex.vertexId().part, mActiveVertex.vertexId().ring );
      setActiveVertex( Vertex() );
      return;
    }
  }

  setState( MapToolState::View );
  setActivePartAndRing( 0, 0 );
  setActiveVertex( Vertex() );
}

void RecordingMapTool::updateVertex( const Vertex &vertex, const QgsPoint &point )
{
  if ( vertex.isValid() )
  {
    if ( mRecordedGeometry.get()->moveVertex( vertex.vertexId(), point ) )
    {
      emit recordedGeometryChanged( mRecordedGeometry );
    }
  }
}

QgsPoint RecordingMapTool::vertexMapCoors( const Vertex &vertex ) const
{
  if ( vertex.isValid() && mFeatureLayerPair.isValid() && mapSettings() )
  {
    return InputUtils::transformPoint( mFeatureLayerPair.layer()->crs(), mapSettings()->destinationCrs(), mFeatureLayerPair.layer()->transformContext(), vertex.coordinates() );
  }

  return QgsPoint();
}

void RecordingMapTool::cancelGrab()
{
  QgsPoint activeVertexPosition = vertexMapCoors( mActiveVertex );
  if ( !activeVertexPosition.isEmpty() )
  {
    mapSettings()->setCenter( activeVertexPosition );
  }

  setState( MapToolState::View );
  setActivePartAndRing( 0, 0 );
  setActiveVertex( Vertex() );
}

double RecordingMapTool::pixelsToMapUnits( double numPixels )
{
  QgsRenderContext context = QgsRenderContext::fromMapSettings( mapSettings()->mapSettings() );
  return numPixels * context.scaleFactor() * context.mapToPixel().mapUnitsPerPixel();
}

bool RecordingMapTool::shouldBeVisible( const QgsPoint point )
{
  return !mActiveVertex.isValid() || ( mActiveVertex.isValid() && !InputUtils::equals( point, mActiveVertex.coordinates(), 1e-16 ) );
}

void RecordingMapTool::grabNextVertex()
{
  if ( !mActiveVertex.isValid() )
  {
    return;
  }

  // mActiveVertex is pointing to removed vertex
  QgsVertexId current = mActiveVertex.vertexId();

  // if there are some remaining points in the current ring&part, grab one of them!
  if ( mRecordedGeometry.constGet()->vertexCount( current.part, current.ring ) )
  {
    int nextId = 0;

    bool isNotFirst = ( current.vertex > 0 );
    if ( isNotFirst )
    {
      nextId = current.vertex - 1;
    }

    QgsVertexId next( current.part, current.ring, nextId );
    QgsPoint positionOfNext = mRecordedGeometry.constGet()->vertexAt( next );

    setActiveVertex( Vertex( next, positionOfNext, Vertex::Existing ) );
    setState( MapToolState::Grab );
  }
  else
  {
    // jump to other part if there is any
    if ( mRecordedGeometry.constGet()->partCount() > 1 )
    {
      QgsVertexId nextRingVertex( 0, 0, 0 );
      QgsPoint nextRingVertexPosition = mRecordedGeometry.constGet()->vertexAt( nextRingVertex );
      setActiveVertex( Vertex( nextRingVertex, nextRingVertexPosition, Vertex::Existing ) );
      setState( MapToolState::Grab );
    }
    else
    {
      // no more points in this ring/part, start recording
      setActivePartAndRing( 0, current.ring );
      setState( MapToolState::Record );
      setActiveVertex( Vertex() );
    }
  }
}

void RecordingMapTool::completeEditOperation()
{
  if ( mFeatureLayerPair.isValid() && mFeatureLayerPair.layer()->isEditCommandActive() )
  {
    mFeatureLayerPair.layer()->changeGeometry( mFeatureLayerPair.feature().id(), mRecordedGeometry );
    mFeatureLayerPair.layer()->endEditCommand();
    mFeatureLayerPair.layer()->triggerRepaint();
  }
}

void RecordingMapTool::undo()
{
  if ( mFeatureLayerPair.isValid() && mFeatureLayerPair.layer()->undoStack() )
  {
    mFeatureLayerPair.layer()->undoStack()->undo();
    QgsGeometry geom = mFeatureLayerPair.layer()->editBuffer()->changedGeometries()[ mFeatureLayerPair.feature().id() ];
    if ( !geom.isEmpty() )
    {
      setRecordedGeometry( geom );
    }
    else
    {
      setRecordedGeometry( mFeatureLayerPair.feature().geometry() );
    }
    mFeatureLayerPair.layer()->triggerRepaint();
  }
}

Vertex::Vertex()
{

}

Vertex::Vertex( QgsVertexId id, QgsPoint coordinates, VertexType type )
  : mVertexId( id )
  , mCoordinates( coordinates )
  , mType( type )
{

}

Vertex::~Vertex()
{

}

bool Vertex::isValid() const
{
  return mVertexId.isValid();
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
  setRecordedGeometry( QgsGeometry() );
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

  emit initialGeometryChanged( mInitialGeometry );
}

const QgsGeometry &RecordingMapTool::existingVertices() const
{
  return mExistingVertices;
}

void RecordingMapTool::setExistingVertices( const QgsGeometry &newExistingVertices )
{
  if ( mExistingVertices.equals( newExistingVertices ) )
    return;
  mExistingVertices = newExistingVertices;
  emit existingVerticesChanged( mExistingVertices );
}

const QgsGeometry &RecordingMapTool::midPoints() const
{
  return mMidPoints;
}

void RecordingMapTool::setMidPoints( const QgsGeometry &newMidPoints )
{
  if ( mMidPoints.equals( newMidPoints ) )
    return;
  mMidPoints = newMidPoints;
  emit midPointsChanged( mMidPoints );
}

const QgsGeometry &RecordingMapTool::handles() const
{
  return mHandles;
}

void RecordingMapTool::setHandles( const QgsGeometry &newHandles )
{
  if ( mHandles.equals( newHandles ) )
    return;
  mHandles = newHandles;
  emit handlesChanged( mHandles );
}

RecordingMapTool::MapToolState RecordingMapTool::state() const
{
  return mState;
}

void RecordingMapTool::setState( const MapToolState &newState )
{
  if ( mState == newState )
    return;
  mState = newState;
  emit stateChanged( mState );
}

QgsPoint RecordingMapTool::recordPoint() const
{
  return mRecordPoint;
}

void RecordingMapTool::setRecordPoint( QgsPoint newRecordPoint )
{
  if ( mRecordPoint == newRecordPoint )
    return;
  mRecordPoint = newRecordPoint;
  emit recordPointChanged( mRecordPoint );
}

const Vertex &RecordingMapTool::activeVertex() const
{
  return mActiveVertex;
}

void RecordingMapTool::setActiveVertex( const Vertex &newActiveVertex )
{
  if ( mActiveVertex == newActiveVertex )
    return;
  mActiveVertex = newActiveVertex;
  emit activeVertexChanged( mActiveVertex );
}


const QgsVertexId &Vertex::vertexId() const
{
  return mVertexId;
}

void Vertex::setVertexId( const QgsVertexId &newVertexId )
{
  if ( mVertexId == newVertexId )
    return;
  mVertexId = newVertexId;
}

QgsPoint Vertex::coordinates() const
{
  return mCoordinates;
}

void Vertex::setCoordinates( QgsPoint newCoordinates )
{
  if ( mCoordinates == newCoordinates )
    return;
  mCoordinates = newCoordinates;
}

const Vertex::VertexType &Vertex::type() const
{
  return mType;
}

void Vertex::setType( const VertexType &newType )
{
  if ( mType == newType )
    return;
  mType = newType;
}

const RecordingMapTool::InsertPolicy &RecordingMapTool::insertPolicy() const
{
  return mInsertPolicy;
}

void RecordingMapTool::setInsertPolicy( const InsertPolicy &insertPolicy )
{
  if ( mInsertPolicy == insertPolicy )
    return;
  mInsertPolicy = insertPolicy;
  emit insertPolicyChanged( mInsertPolicy );
}

int RecordingMapTool::activePart() const
{
  return mActivePart;
}

void RecordingMapTool::setActivePartAndRing( int newActivePart, int newActiveRing )
{
  bool partChanged = false, ringChanged = false;

  if ( mActivePart != newActivePart )
  {
    mActivePart = newActivePart;
    partChanged = true;
  }

  if ( mActiveRing != newActiveRing )
  {
    mActiveRing = newActiveRing;
    ringChanged = true;
  }

  if ( partChanged )
  {
    emit activePartChanged( mActivePart );
  }
  if ( ringChanged )
  {
    emit activeRingChanged( mActiveRing );
  }
}

const QVector< Vertex > &RecordingMapTool::collectedVertices() const
{
  return mVertices;
}

int RecordingMapTool::activeRing() const
{
  return mActiveRing;
}

const FeatureLayerPair &RecordingMapTool::featureLayerPair() const
{
  return mFeatureLayerPair;
}

void RecordingMapTool::setFeatureLayerPair( const FeatureLayerPair &newFeatureLayerPair )
{
  if ( mFeatureLayerPair == newFeatureLayerPair )
    return;

  if ( mFeatureLayerPair.isValid() && mFeatureLayerPair.layer()->isEditable() )
  {
    // rollback any changes and stop editing of the previously used layer
    mFeatureLayerPair.layer()->rollBack();
  }

  mFeatureLayerPair = newFeatureLayerPair;

  // start editing
  mFeatureLayerPair.layer()->startEditing();

  emit featureLayerPairChanged( mFeatureLayerPair );
}
