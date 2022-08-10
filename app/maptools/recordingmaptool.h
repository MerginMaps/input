﻿/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef RECORDINGMAPTOOL_H
#define RECORDINGMAPTOOL_H

#include "abstractmaptool.h"

#include <QObject>
#include <qglobal.h>

#include "qgsvertexid.h"
#include "qgsgeometry.h"

#include "inpututils.h"

class PositionKit;
class VariablesManager;
class QgsVectorLayer;

class Vertex
{
    Q_GADGET

    Q_PROPERTY( QgsVertexId vertexId READ vertexId WRITE setVertexId )
    Q_PROPERTY( QgsPoint coordinates READ coordinates WRITE setCoordinates )
    Q_PROPERTY( VertexType type READ type WRITE setType )

  public:

    enum VertexType
    {
      Existing = 0, // Existing vertex in geometry
      MidPoint, // Node between two existing vertices
      HandleStart, // Node before beginning of line
      HandleEnd // Node after end of line
    };
    Q_ENUM( VertexType );

    Vertex();
    Vertex( QgsVertexId id, QgsPoint coordinates, VertexType type );

    virtual ~Vertex();

    Q_INVOKABLE bool isValid() const;

    const QgsVertexId &vertexId() const;
    void setVertexId( const QgsVertexId &newVertexId );

    QgsPoint coordinates() const;
    void setCoordinates( QgsPoint newCoordinates );

    const VertexType &type() const;
    void setType( const VertexType &newType );

    bool operator==( const Vertex &other )
    {
      return other.vertexId() == mVertexId && InputUtils::equals( other.coordinates(), mCoordinates ) && other.type() == mType;
    }

    bool operator!=( const Vertex &other )
    {
      return !( *this == other );
    }

  private:
    QgsVertexId mVertexId;
    QgsPoint mCoordinates;
    VertexType mType = VertexType::Existing;
};

class RecordingMapTool : public AbstractMapTool
{
    Q_OBJECT

    Q_PROPERTY( bool centeredToGPS READ centeredToGPS WRITE setCenteredToGPS NOTIFY centeredToGPSChanged )
    Q_PROPERTY( RecordingType recordingType READ recordingType WRITE setRecordingType NOTIFY recordingTypeChanged )
    Q_PROPERTY( int recordingInterval READ recordingInterval WRITE setRecordingInterval NOTIFY recordingIntervalChanged )

    Q_PROPERTY( QgsVectorLayer *layer READ layer WRITE setLayer NOTIFY layerChanged )
    Q_PROPERTY( PositionKit *positionKit READ positionKit WRITE setPositionKit NOTIFY positionKitChanged )

    Q_PROPERTY( QgsGeometry recordedGeometry READ recordedGeometry WRITE setRecordedGeometry NOTIFY recordedGeometryChanged )
    Q_PROPERTY( QgsGeometry existingVertices READ existingVertices WRITE setExistingVertices NOTIFY existingVerticesChanged )
    Q_PROPERTY( QgsGeometry midPoints READ midPoints WRITE setMidPoints NOTIFY midPointsChanged )
    Q_PROPERTY( QgsGeometry handles READ handles WRITE setHandles NOTIFY handlesChanged )

    Q_PROPERTY( Vertex activeVertex READ activeVertex WRITE setActiveVertex NOTIFY activeVertexChanged )
    Q_PROPERTY( InsertPolicy insertPolicy READ insertPolicy WRITE setInsertPolicy NOTIFY insertPolicyChanged )

    // When editing geometry - set this as the geometry to start with
    Q_PROPERTY( QgsGeometry initialGeometry READ initialGeometry WRITE setInitialGeometry NOTIFY initialGeometryChanged )

    Q_PROPERTY( MapToolState state READ state WRITE setState NOTIFY stateChanged )

    Q_PROPERTY( QgsPoint recordPoint READ recordPoint WRITE setRecordPoint NOTIFY recordPointChanged )
    Q_PROPERTY( int activePart READ activePart NOTIFY activePartChanged )
    Q_PROPERTY( int activeRing READ activeRing NOTIFY activeRingChanged )

    Q_PROPERTY( FeatureLayerPair featureLayerPair READ featureLayerPair WRITE setFeatureLayerPair NOTIFY featureLayerPairChanged )

  public:

    enum RecordingType
    {
      StreamMode = 0,
      Manual
    };
    Q_ENUM( RecordingType );

    enum MapToolState
    {
      Record = 0, // No point selected, show crosshair and guideline
      Grab, // Existing point selected, show crosshair and guideline
      View // No point selected, hide crosshair and guideline [start of editing]
    };
    Q_ENUM( MapToolState );

    enum InsertPolicy
    {
      End = 0, // Default, new vertices are appended to the end of geometry
      Start // Vertices will be added to the beggining of geometry (when clicked on continue line from start)
    };
    Q_ENUM( InsertPolicy );

    explicit RecordingMapTool( QObject *parent = nullptr );
    virtual ~RecordingMapTool();

    /**
     * Adds point to the end of the recorded geometry; updates recordedGeometry afterwards
     * Passed point needs to be in active vector layer CRS
     */
    Q_INVOKABLE void addPoint( const QgsPoint &point );

    void addPointAtPosition( Vertex vertex, const QgsPoint &point );

    /**
     *  Removes last point from recorded geometry if there is at least one point
     *  Updates recordedGeometry afterwards
     */
    Q_INVOKABLE void removePoint();

    //! Returns true if the captured geometry has enought points for the specified layer
    Q_INVOKABLE bool hasValidGeometry() const;

    /**
     * Finds vertex id which matches given screen coordinates. Search radius defined in pixels
     */
    Q_INVOKABLE void lookForVertex( const QPointF &clickedPoint, double searchRadius = 3 );

    /**
     * Updates vertex at the active vertex position and updates recordedGeometry
     * Passed point needs to be in active vector layer CRS
     */
    Q_INVOKABLE void releaseVertex( const QgsPoint &point );

    /**
     * Reverts last change from the layer undo stack.
     */
    Q_INVOKABLE void undo();

    void updateVertex( const Vertex &vertex, const QgsPoint &point );

    /**
     * Returns coordinates of the active vertex in map CRS
     */
    Q_INVOKABLE QgsPoint vertexMapCoors( const Vertex &vertex ) const;

    Q_INVOKABLE void cancelGrab();

    // Getters / setters
    bool centeredToGPS() const;
    void setCenteredToGPS( bool newCenteredToGPS );

    const RecordingType &recordingType() const;
    void setRecordingType( const RecordingType &newRecordingType );

    int recordingInterval() const;
    void setRecordingInterval( int newRecordingInterval );

    PositionKit *positionKit() const;
    void setPositionKit( PositionKit *newPositionKit );

    QgsVectorLayer *layer() const;
    void setLayer( QgsVectorLayer *newLayer );

    const QgsGeometry &recordedGeometry() const;
    void setRecordedGeometry( const QgsGeometry &newRecordedGeometry );

    const QgsGeometry &initialGeometry() const;
    // Fills mPoints array with points from the geometry
    void setInitialGeometry( const QgsGeometry &newInitialGeometry );

    const QgsGeometry &existingVertices() const;
    void setExistingVertices( const QgsGeometry &newExistingVertices );

    const QgsGeometry &midPoints() const;
    void setMidPoints( const QgsGeometry &newMidPoints );

    const QgsGeometry &handles() const;
    void setHandles( const QgsGeometry &newHandles );

    const QVector< Vertex > &collectedVertices() const;

    MapToolState state() const;
    void setState( const MapToolState &newState );

    QPointF crosshairPosition() const;
    void setCrosshairPosition( QPointF newCrosshairPosition );

    QgsPoint recordPoint() const;
    void setRecordPoint( QgsPoint newRecordPoint );

    const Vertex &activeVertex() const;
    void setActiveVertex( const Vertex &newActiveVertex );

    const InsertPolicy &insertPolicy() const;
    void setInsertPolicy( const InsertPolicy &insertPolicy );

    int activePart() const;
    int activeRing() const;
    void setActivePartAndRing( int newActivePart, int newActiveRing );

    const FeatureLayerPair &featureLayerPair() const;
    void setFeatureLayerPair( const FeatureLayerPair &newFeatureLayerPair );

  signals:
    void layerChanged( QgsVectorLayer *layer );
    void centeredToGPSChanged( bool centeredToGPS );
    void positionKitChanged( PositionKit *positionKit );
    void recordedGeometryChanged( const QgsGeometry &recordedGeometry );
    void recordingIntervalChanged( int lineRecordingInterval );
    void recordingTypeChanged( const RecordingMapTool::RecordingType &recordingType );

    void initialGeometryChanged( const QgsGeometry &initialGeometry );
    void existingVerticesChanged( const QgsGeometry &existingVertices );
    void midPointsChanged( const QgsGeometry &midPoints );
    void handlesChanged( const QgsGeometry &handles );
    void stateChanged( const RecordingMapTool::MapToolState &state );

    void recordPointChanged( QgsPoint recordPoint );

    void activeVertexChanged( const Vertex &activeVertex );

    void insertPolicyChanged( const RecordingMapTool::InsertPolicy &insertPolicy );

    void activePartChanged( int activePart );
    void activeRingChanged( int activeRing );

    void featureLayerPairChanged( const FeatureLayerPair &featureLayerPair );

  public slots:
    void onPositionChanged();

  private slots:
    void prepareEditing();

    /**
     * Creates nodes index. Extracts existing geometry vertices and generates virtual
     * vertices representing midpoints (for lines and polygons) and start/end points
     * (for lines).
     */
    void collectVertices();

    /**
     * Creates geometries represeinting existing nodes, midpoints (for lines and polygons),
     * start/end points and "handles" (for lines) from the nodes index.
     */
    void updateVisibleItems();

    /**
     * Grabs next vertex after the removal of the currently selected vertex
     */
    void grabNextVertex();

    /**
     * Ends layer editing command and puts it into layer undo stack
     */
    void completeEditOperation();

  protected:
    //! Unifies Z coordinate of the point with current layer - drops / adds it
    void fixZ( QgsPoint &point ) const;

  private:
    double pixelsToMapUnits( double numPixels );

    /**
     * Check whether given point should be used for creating markers/handles
     */
    bool shouldBeVisible( QgsPoint point );

    QgsGeometry mRecordedGeometry;
    QgsGeometry mInitialGeometry;

    bool mCenteredToGPS = false;
    RecordingType mRecordingType = Manual;
    int mRecordingInterval;  // in seconds for the StreamingMode

    QDateTime mLastTimeRecorded;

    QgsVectorLayer *mLayer = nullptr; // not owned
    PositionKit *mPositionKit = nullptr; // not owned

    QgsGeometry mExistingVertices;
    QgsGeometry mMidPoints;
    QgsGeometry mHandles;
    QgsGeometry mHiddenHandle;

    MapToolState mState = MapToolState::Record;
    InsertPolicy mInsertPolicy = InsertPolicy::End;

    QVector< Vertex > mVertices;

    QgsPoint mRecordPoint;

    // ActiveVertex is set only when we grab a point,
    // it is the grabbed point, contains its coordinates and index
    Vertex mActiveVertex;

    // ActiveRing and ActivePart are set only when we record,
    // in order to know where to insert the points
    int mActiveRing = 0;
    int mActivePart = 0;

    FeatureLayerPair mFeatureLayerPair;
};

#endif // RECORDINGMAPTOOL_H
