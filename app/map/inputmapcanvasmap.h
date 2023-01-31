/**************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
 * The source code forked from https://github.com/qgis/QGIS on 25th Nov 2022
 * File: qgsquickmapcanvasmap.h by (C) 2014 by Matthias Kuhn
 */

#ifndef INPUTMAPCANVASMAP_H
#define INPUTMAPCANVASMAP_H

#include "inputconfig.h"

#include "inputmapsettings.h"

#include <QFutureSynchronizer>
#include <QTimer>
#include <QtQuick/QQuickItem>
#include <qgsmapsettings.h>
#include <qgspoint.h>

#include <memory>

class QgsMapRendererParallelJob;
class QgsMapRendererCache;
class QgsLabelingResults;

/**
 * \brief This class implements a visual Qt Quick Item that does map rendering
 * according to the current map settings. Client code is expected to use
 * MapCanvas item rather than using this class directly.
 *
 * InputMapCanvasMap instance internally creates InputMapSettings in
 * constructor. The QgsProject should be attached to the InputMapSettings.
 * The map settings for other Input components should be initialized from
 * InputMapCanvasMap's mapSettings
 *
 * \note QML Type: MapCanvasMap
 *
 * \sa InputMapCanvas
 */
class InputMapCanvasMap : public QQuickItem
{
    Q_OBJECT

    /**
     * The mapSettings property contains configuration for rendering of the map.
     *
     * It should be used as a primary source of map settings (and project) for
     * all other components in the application.
     *
     * This is a readonly property.
     */
    Q_PROPERTY( InputMapSettings *mapSettings READ mapSettings )

    /**
     * When freeze property is set to TRUE, the map canvas does not refresh.
     * The value temporary changes during the rendering process.
     */
    Q_PROPERTY( bool freeze READ freeze WRITE setFreeze NOTIFY freezeChanged )

    /**
     * The isRendering property is set to TRUE while a rendering job is pending for this map canvas map.
     * It can be used to show a notification icon about an ongoing rendering job.
     * This is a readonly property.
     */
    Q_PROPERTY( bool isRendering READ isRendering NOTIFY isRenderingChanged )

    /**
     * Interval in milliseconds after which the map canvas will be updated while a rendering job is ongoing.
     * This only has an effect if incrementalRendering is activated.
     * Default is 250 [ms].
     */
    Q_PROPERTY( int mapUpdateInterval READ mapUpdateInterval WRITE setMapUpdateInterval NOTIFY mapUpdateIntervalChanged )

    /**
     * When the incrementalRendering property is set to TRUE, the automatic refresh of map canvas during rendering is allowed.
     */
    Q_PROPERTY( bool incrementalRendering READ incrementalRendering WRITE setIncrementalRendering NOTIFY incrementalRenderingChanged )

  public:
    //! Create map canvas map
    explicit InputMapCanvasMap( QQuickItem *parent = nullptr );
    ~InputMapCanvasMap();

    QSGNode *updatePaintNode( QSGNode *oldNode, QQuickItem::UpdatePaintNodeData * ) override;

    //! \copydoc InputMapCanvasMap::mapSettings
    InputMapSettings *mapSettings() const;

    //! \copydoc InputMapCanvasMap::freeze
    bool freeze() const;

    //! \copydoc InputMapCanvasMap::freeze
    void setFreeze( bool freeze );

    //! \copydoc InputMapCanvasMap::isRendering
    bool isRendering() const;

    //! \copydoc InputMapCanvasMap::mapUpdateInterval
    int mapUpdateInterval() const;

    //! \copydoc InputMapCanvasMap::mapUpdateInterval
    void setMapUpdateInterval( int mapUpdateInterval );

    //! \copydoc InputMapCanvasMap::incrementalRendering
    bool incrementalRendering() const;

    //! \copydoc InputMapCanvasMap::incrementalRendering
    void setIncrementalRendering( bool incrementalRendering );

  signals:

    /**
     * Signal is emitted when a rendering is starting
     */
    void renderStarting();

    /**
     * Signal is emitted when a canvas is refreshed
     */
    void mapCanvasRefreshed();

    //! \copydoc InputMapCanvasMap::freeze
    void freezeChanged();

    //! \copydoc InputMapCanvasMap::isRendering
    void isRenderingChanged();

    //!\copydoc InputMapCanvasMap::mapUpdateInterval
    void mapUpdateIntervalChanged();

    //!\copydoc InputMapCanvasMap::incrementalRendering
    void incrementalRenderingChanged();

  protected:
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    void geometryChanged( const QRectF &newGeometry, const QRectF &oldGeometry ) override;
#else
    void geometryChange( const QRectF &newGeometry, const QRectF &oldGeometry ) override;
#endif

  public slots:
    //! Stop map rendering
    void stopRendering();

    /**
     * Set map setting's extent (zoom the map) on the center by given scale
     */
    void zoom( QPointF center, qreal scale );

    /**
     * Set map setting's extent (pan the map) based on the difference of positions
     */
    void pan( QPointF oldPos, QPointF newPos );

    /**
     * Refresh the map canvas.
     * Does nothing when output size of map settings is not set
     */
    void refresh();

    /**
     * Clears rendering cache
     */
    void clearCache();

  private slots:
    void refreshMap();
    void renderJobUpdated();
    void renderJobFinished();
    void layerRepaintRequested( bool deferred );
    void onWindowChanged( QQuickWindow *window );
    void onScreenChanged( QScreen *screen );
    void onExtentChanged();
    void onLayersChanged();
    void onTemporalStateChanged();

  private:

    /**
     * Should only be called by stopRendering()!
     */
    void destroyJob( QgsMapRendererJob *job );
    QgsMapSettings prepareMapSettings() const;
    void updateTransform();
    void zoomToFullExtent();
    void clearTemporalCache();

    std::unique_ptr<InputMapSettings> mMapSettings;
    bool mPinching = false;
    QPoint mPinchStartPoint;
    QgsMapRendererParallelJob *mJob = nullptr;
    std::unique_ptr<QgsMapRendererCache> mCache;
    QgsLabelingResults *mLabelingResults = nullptr;
    QImage mImage;
    QgsMapSettings mImageMapSettings;
    QTimer mRefreshTimer;
    bool mDirty = false;
    bool mFreeze = false;
    QList<QMetaObject::Connection> mLayerConnections;
    QTimer mMapUpdateTimer;
    bool mIncrementalRendering = false;
    bool mSilentRefresh = false;
    bool mDeferredRefreshPending = false;

    QQuickWindow *mWindow = nullptr;
};

#endif // INPUTMAPCANVASMAP_H
