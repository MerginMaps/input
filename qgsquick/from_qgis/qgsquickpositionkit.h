/***************************************************************************
 qgsquickpositionkit.h
  --------------------------------------
  Date                 : Dec. 2017
  Copyright            : (C) 2017 Peter Petrik
  Email                : zilolv at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSQUICKPOSITIONKIT_H
#define QGSQUICKPOSITIONKIT_H

#include <QObject>
#include <QtPositioning>

#include "qgspoint.h"

#include "qgis_quick.h"
#include "qgsquickmapsettings.h"
#include "qgsquickcoordinatetransformer.h"

/**
 * \ingroup quick
 * Convenient set of tools to read GPS position and accuracy.
 *
 * Also, if one can use use_simulated_location to specify simulated position.
 * Simulated position source generates random points in circles around the selected
 * point and radius. Real GPS position is not used in this mode.
 *
 * By default is it disabled. Use enabled property to start updating the location
 *
 * \note QML Type: PositionKit
 *
 * \since QGIS 3.4
 */
class QUICK_EXPORT QgsQuickPositionKit : public QObject
{
    Q_OBJECT

    /**
     * GPS position in WGS84 coords.
     *
     * This is a readonly property.
     */
    Q_PROPERTY( QgsPoint position READ position NOTIFY positionChanged )

    /**
     * GPS position in map coords.
     *
     * This is a readonly property.
     */
    Q_PROPERTY( QgsPoint projectedPosition READ projectedPosition NOTIFY projectedPositionChanged )

    /**
     * GPS position in device coords (pixels).
     *
     * This is a readonly property.
     */
    Q_PROPERTY( QPointF screenPosition READ screenPosition NOTIFY screenPositionChanged )

    /**
     * GPS position is available (position property is a valid number).
     *
     * This is a readonly property.
     */
    Q_PROPERTY( bool hasPosition READ hasPosition NOTIFY hasPositionChanged )

    /**
     * GPS horizontal accuracy in accuracyUnits, -1 if not available.
     *
     * This is a readonly property.
     */
    Q_PROPERTY( double accuracy READ accuracy NOTIFY accuracyChanged )

    /**
     * Screen horizontal accuracy, 2 if not available or resolution is too small.
     *
     * This is a readonly property.
     */
    Q_PROPERTY( double screenAccuracy READ screenAccuracy NOTIFY screenAccuracyChanged )

    /**
     * GPS direction, bearing in degrees clockwise from north to direction of travel. -1 if not available
     *
     * This is a readonly property.
     */
    Q_PROPERTY( double direction READ direction NOTIFY directionChanged )

    /**
     * GPS position and accuracy is simulated (not real from GPS sensor). Default FALSE (use real GPS)
     *
     * This is a readonly property. To change to simulated position, see QgsQuickPositionKit::simulatePositionLongLatRad
     */
    Q_PROPERTY( bool isSimulated READ isSimulated NOTIFY isSimulatedChanged )

    /**
     * Associated map settings. Should be initialized before the first use from mapcanvas map settings.
     *
     * This is a readonly property.
     */
    Q_PROPERTY( QgsQuickMapSettings *mapSettings READ mapSettings WRITE setMapSettings NOTIFY mapSettingsChanged )

    /**
     * Uses of GPS and simulated position and sets its parameters
     *
     * Vector containing longitude, latitude and radius (meters) of simulated position e.g. [-97.36, 36.93, 2]
     * If empty vector is assigned, GPS source will be used.
     *
     * From QML context, also functions useSimulatedLocation() or useGpsLocation() could be used instead
     */
    Q_PROPERTY( QVector<double> simulatePositionLongLatRad READ simulatePositionLongLatRad WRITE setSimulatePositionLongLatRad NOTIFY simulatePositionLongLatRadChanged )

    /**
     * When enabled, start updates of the position source
     * When disabled, stop updates of the position source
     *
     * Note that on enabling the position source, it requests the
     * location permissions (Android/iOS) from user
     *
     * \since QGIS 3.18
     */
    Q_PROPERTY( bool enabled READ isEnabled WRITE setIsEnabled NOTIFY isEnabledChanged )

  public:
    //! Creates new position kit
    explicit QgsQuickPositionKit( QObject *parent = nullptr );

    //! \copydoc QgsQuickPositionKit::position
    bool hasPosition() const;

    //! \copydoc QgsQuickPositionKit::position
    QgsPoint position() const;

    //! \copydoc QgsQuickPositionKit::projectedPosition
    QgsPoint projectedPosition() const;

    //! \copydoc QgsQuickPositionKit::screenPosition
    QPointF screenPosition() const;

    //! \copydoc QgsQuickPositionKit::accuracy
    double accuracy() const;

    //! \copydoc QgsQuickPositionKit::screenAccuracy
    double screenAccuracy() const;

    /**
     * Returns last known position
     *
     * \since QGIS 3.18
     */
    QGeoPositionInfo lastKnownPosition( bool fromSatellitePositioningMethodsOnly = false ) const;

    /**
     * GPS horizontal accuracy units - meters (constant)
     */
    QgsUnitTypes::DistanceUnit accuracyUnits() const;

    //! \copydoc QgsQuickPositionKit::direction
    double direction() const;

    //! \copydoc QgsQuickPositionKit::isSimulated
    bool isSimulated() const;

    //! \copydoc QgsQuickPositionKit::mapSettings
    void setMapSettings( QgsQuickMapSettings *mapSettings );

    //! \copydoc QgsQuickPositionKit::mapSettings
    QgsQuickMapSettings *mapSettings() const;

    //! \copydoc QgsQuickPositionKit::simulatePositionLongLatRad
    QVector<double> simulatePositionLongLatRad() const;

    //! \copydoc QgsQuickPositionKit::simulatePositionLongLatRad
    void setSimulatePositionLongLatRad( const QVector<double> &simulatePositionLongLatRad );

    /**
     * Coordinate reference system of position - WGS84 (constant)
     */
    Q_INVOKABLE QgsCoordinateReferenceSystem positionCRS() const;

    /**
     * Use simulated GPS source.
     *
     * Simulated GPS source emulates point on circle around defined point in specified radius
     *
     * We do not want to have the origin point as property
     * We basically want to set it once based on project/map cente and keep
     * it that way regardless of mapsettings change (e.g. zoom etc)
     *
     * \param longitude longitude of the centre of the emulated points
     * \param latitude latitude of the centre of the emulated points
     * \param radius distance of emulated points from the centre (in degrees WSG84)
     */
    Q_INVOKABLE void useSimulatedLocation( double longitude, double latitude, double radius );

    /**
     * Use real GPS source (not simulated)
     */
    Q_INVOKABLE void useGpsLocation();

    //! \copydoc QgsQuickPositionKit::enabled
    bool isEnabled() const;

    //! \copydoc QgsQuickPositionKit::enabled
    void setIsEnabled( bool isEnabled );

  signals:
    //! \copydoc QgsQuickPositionKit::position
    void positionChanged();

    //! \copydoc QgsQuickPositionKit::projectedPosition
    void projectedPositionChanged();

    //! \copydoc QgsQuickPositionKit::screenPosition
    void screenPositionChanged();

    //! hasPosition changed
    void hasPositionChanged();

    //! \copydoc QgsQuickPositionKit::accuracy
    double accuracyChanged() const;

    //! \copydoc QgsQuickPositionKit::screenAccuracy
    double screenAccuracyChanged() const;

    //! \copydoc QgsQuickPositionKit::accuracyUnits
    Q_INVOKABLE QString accuracyUnitsChanged() const;

    //! \copydoc QgsQuickPositionKit::direction
    double directionChanged() const;

    //! \copydoc QgsQuickPositionKit::isSimulated
    void isSimulatedChanged();

    //! \copydoc QgsQuickPositionKit::mapSettings
    void mapSettingsChanged();

    //! \copydoc QgsQuickPositionKit::simulatePositionLongLatRad
    void simulatePositionLongLatRadChanged( QVector<double> simulatePositionLongLatRad );

    //! Emitted when the internal source of GPS location data has been replaced.
    void sourceChanged();

    //! \copydoc QgsQuickPositionKit::enabled (INTERNAL)
    void isEnabledChanged();

  private slots:
    void onPositionUpdated( const QGeoPositionInfo &info );
    void onMapSettingsUpdated();
    void onUpdateTimeout();
    void onSimulatePositionLongLatRadChanged( QVector<double> simulatePositionLongLatRad );

  private:
    void replacePositionSource( QGeoPositionInfoSource *source );

    QString calculateStatusLabel();
    double calculateScreenAccuracy();
    void updateProjectedPosition();
    void updateScreenPosition();
    void updateScreenAccuracy();

    QGeoPositionInfoSource *gpsSource();
    QGeoPositionInfoSource *simulatedSource( double longitude, double latitude, double radius );

    QgsPoint mPosition;
    QgsPoint mProjectedPosition;
    QPointF mScreenPosition;
    double mAccuracy = -1;
    double mScreenAccuracy = 2;
    double mDirection = -1;
    bool mHasPosition = false;
    bool mIsSimulated = false;
    QVector<double> mSimulatePositionLongLatRad;
    bool mIsEnabled = false;

    std::unique_ptr<QGeoPositionInfoSource> mSource;

    QgsQuickMapSettings *mMapSettings = nullptr; // not owned
};

#endif // QGSQUICKPOSITIONKIT_H
