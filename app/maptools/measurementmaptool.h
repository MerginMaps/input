/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MEASUREMENTMAPTOOL_H
#define MEASUREMENTMAPTOOL_H

#include "abstractmaptool.h"
#include <qglobal.h>
#include "qgsdistancearea.h"
#include "qgsgeometry.h"
#include "inpututils.h"
#include "qgspolygon.h"
#include "qgsgeometry.h"
#include "qgsvectorlayer.h"
#include "qgsmultipoint.h"

const double CLOSE_THRESHOLD = 10.0; // in pixels

class MeasurementMapTool : public AbstractMapTool
{
    Q_OBJECT

    Q_PROPERTY( QgsGeometry recordedGeometry READ recordedGeometry WRITE setRecordedGeometry NOTIFY recordedGeometryChanged )
    Q_PROPERTY( QgsVectorLayer *activeLayer READ activeLayer WRITE setActiveLayer NOTIFY activeLayerChanged )
    Q_PROPERTY( QgsGeometry existingVertices READ existingVertices WRITE setExistingVertices NOTIFY existingVerticesChanged )

    Q_PROPERTY( double length READ length WRITE setLength NOTIFY lengthChanged )
    Q_PROPERTY( double perimeter READ perimeter WRITE setPerimeter NOTIFY perimeterChanged )
    Q_PROPERTY( double area READ area WRITE setArea NOTIFY areaChanged )

    Q_PROPERTY( bool canUndo READ canUndo WRITE setCanUndo NOTIFY canUndoChanged )
    Q_PROPERTY( bool canCloseShape READ canCloseShape WRITE setCanCloseShape NOTIFY canCloseShapeChanged )
    Q_PROPERTY( bool closeShapeDone READ closeShapeDone WRITE setCloseShapeDone NOTIFY closeShapeDoneChanged )

  public:
    explicit MeasurementMapTool( QObject *parent = nullptr );
    virtual ~MeasurementMapTool() override;

    /**
     * Adds point to the end of the recorded geometry; updates recordedGeometry afterwards
     * Passed point needs to be in map CRS
     */
    Q_INVOKABLE void addPoint( const QPointF &point );

    /**
     *  Removes last point from recorded geometry if there is at least one point
     *  Updates recordedGeometry afterwards
     */
    Q_INVOKABLE void removePoint();

    /**
     * If there are at least 3 points, forms a polygon from recorded points so far.
     * Updates recorded geometry and calculates area and perimeter of formed polygon.
    */
    Q_INVOKABLE void closeShape();

    /**
     * Repeats measurement process.
     * Clears all recorded points and rebuilds the geometry.
     */
    Q_INVOKABLE void repeat();

    // Getter and Setters
    double length() const;
    void setLength( const double &length );

    double perimeter() const;
    void setPerimeter( const double &perimeter );

    double area() const;
    void setArea( const double &area );

    bool canUndo() const;
    void setCanUndo( bool newCanUndo );

    bool canCloseShape() const;
    void setCanCloseShape( bool newCanCloseShape );

    bool closeShapeDone() const;
    void setCloseShapeDone( bool newCloseShapeDone );

    const QgsGeometry &recordedGeometry() const;
    void setRecordedGeometry( const QgsGeometry &newRecordedGeometry );

    QgsGeometry existingVertices() const;
    void setExistingVertices( const QgsGeometry &vertices );

    QgsVectorLayer *activeLayer() const;
    void setActiveLayer( QgsVectorLayer *newActiveLayer );

  signals:
    void lengthChanged( const double &length );
    void perimeterChanged( const double &perimeter );
    void areaChanged( const double &area );
    void canUndoChanged( bool canUndo );
    void canCloseShapeChanged( bool canUndo );
    void closeShapeDoneChanged( bool canUndo );
    void activeLayerChanged( QgsVectorLayer *activeLayer );
    void recordedGeometryChanged( const QgsGeometry &recordedGeometry );
    void existingVerticesChanged( const QgsGeometry &vertices );

  protected:
    void rebuildGeometry();

  public slots:
    void updateDistance( const QPointF &crosshairPoint );

  private:
    QVector<QgsPoint> mPoints;
    QgsGeometry mRecordedGeometry;
    QgsGeometry mExistingVertices;
    QgsVectorLayer *mActiveLayer = nullptr;
    double mLength = 0;
    double mPerimeter = 0;
    double mArea = 0;
    bool mCanUndo = false;
    bool mCanCloseShape = false;
    bool mCloseShapeDone = false;
};

#endif // MEASUREMENTMAPTOOL_H
