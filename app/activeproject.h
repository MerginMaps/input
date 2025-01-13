/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef ACTIVEPROJECT_H
#define ACTIVEPROJECT_H

#include <QObject>

#include "qgsproject.h"

#include "inputconfig.h"
#include "appsettings.h"
#include "activelayer.h"
#include "layersproxymodel.h"
#include "localprojectsmanager.h"
#include "autosynccontroller.h"
#include "inputmapsettings.h"
#include "merginapi.h"

/**
 * \brief The ActiveProject class can load a QGIS project and holds its data.
 */
class ActiveProject: public QObject
{
    Q_OBJECT
    Q_PROPERTY( LocalProject localProject READ localProject NOTIFY localProjectChanged ) // LocalProject instance of active project, changes when project is loaded
    Q_PROPERTY( QgsProject *qgsProject READ qgsProject NOTIFY qgsProjectChanged ) // QgsProject instance of active project, never changes
    Q_PROPERTY( AutosyncController *autosyncController READ autosyncController NOTIFY autosyncControllerChanged )
    Q_PROPERTY( InputMapSettings *mapSettings READ mapSettings WRITE setMapSettings NOTIFY mapSettingsChanged )
    Q_PROPERTY( QString projectRole READ projectRole WRITE setProjectRole NOTIFY projectRoleChanged )

    Q_PROPERTY( QString mapTheme READ mapTheme WRITE setMapTheme NOTIFY mapThemeChanged )
    Q_PROPERTY( bool positionTrackingSupported READ positionTrackingSupported NOTIFY positionTrackingSupportedChanged )

  public:
    explicit ActiveProject(
      AppSettings &appSettings
      , ActiveLayer &activeLayer
      , LayersProxyModel &recordingLayerPM
      , LocalProjectsManager &localProjectsManager
      , MerginApi *mMerginApi
      , QObject *parent = nullptr );

    virtual ~ActiveProject();

    //! Returns active project's QgsProject instance to do QGIS API magic
    QgsProject *qgsProject();

    //! Returns Input related info about active project
    LocalProject localProject();

    Q_INVOKABLE QString projectFullName() const;

    /**
     * Loads a .qgz/.qgs project file specified by filePath.
     * \param filePath Path to project file.
     */
    Q_INVOKABLE bool load( const QString &filePath );

    /**
     * Applies map theme with 'name' to currently loaded QGIS project
     * Invalidates active layer if it is no longer visible
     */
    Q_INVOKABLE void setMapTheme( const QString &name );

    /**
     * setActiveLayer sets active layer from layer
     */
    Q_INVOKABLE void setActiveLayer( QgsMapLayer *layer ) const;

    /**
     * Switches visibility of node from off to on or vice versa
     * Invalidates current map theme in case layer's visibility is not aligned with it
     * Invalidates active layer in case active layer is no longer visible
     */
    Q_INVOKABLE void switchLayerTreeNodeVisibility( QgsLayerTreeNode *node );

    //! Returns boolean if any project is currently loaded
    Q_INVOKABLE bool isProjectLoaded() const;

    /**
     * mapSettings method returns mapsettings pointer
     */
    InputMapSettings *mapSettings() const;

    //! Returns autosyncController instance if autosync is allowed, otherwise returns nullptr
    AutosyncController *autosyncController() const;

    /**
     * setMapSettings method sets mapSettings
     * Method also reloads the layer list
     */
    void setMapSettings( InputMapSettings *mapSettings );

    /**
     * setMapSettingsLayers reloads layer list from current project
     */
    void updateMapSettingsLayers() const;

    /**
     * layerVisible returns boolean if input layer is visible within current project
     */
    bool layerVisible( QgsMapLayer *layer );

    /**
     * Return the QGIS log recorded during the loading phase of the project
     */
    Q_INVOKABLE QString projectLoadingLog() const;

    //! A File on this path represents that project is loading and exists only during the process.
    static const QString LOADING_FLAG_FILE_PATH;

    const QString &mapTheme() const;

    bool positionTrackingSupported() const;

    /**
     * Returns role/permission level of current user for this project
     */
    Q_INVOKABLE QString projectRole() const;

    void setProjectRole( const QString &role );

    /**
     * Calls Mergin API to update current project’s role
     */
    Q_INVOKABLE void updateUserRoleInActiveProject();

  signals:
    void qgsProjectChanged();
    void localProjectChanged( LocalProject project );

    void projectWillBeReloaded();
    void projectReloaded( QgsProject *project );
    void loadingStarted();
    void loadingFinished();

    void projectReadingFailed( QString error );
    void reportIssue( QString title, QString message );
    void loadingErrorFound();
    void qgisLogChanged();

    void autosyncControllerChanged( AutosyncController *controller );

    void mapSettingsChanged();

    void syncActiveProject( const LocalProject &project );

    void mapThemeChanged( const QString &mapTheme );

    void positionTrackingSupportedChanged();

    // Emited when the app (UI) should show tracking because there is a running tracking service
    void startPositionTracking();

    void projectRoleChanged();

  public slots:
    // Reloads project if current project path matches given path (its the same project)
    bool reloadProject( QString projectDir );

    void setAutosyncEnabled( bool enabled );

    void requestSync();

  private:

    /**
     * Build up warning list from loaded project
     * Emits reportIssue for each issue found
     * Returns true if there were errors found
     */
    bool validateProject();

    //! Tries to match current visible layers with some theme and if it fails, invalidates current map theme
    void updateMapTheme();

    /** Checks whether active (recording) layer is still visible,
     *  if not, sets first available layer as active;
     *  sets nullptr if there are no other available layers
     */
    void updateActiveLayer();

    //! Reloads layers in 'recoring layers model'
    void updateRecordingLayers();

    QgsProject *mQgsProject = nullptr;
    LocalProject mLocalProject;

    AppSettings &mAppSettings;
    ActiveLayer &mActiveLayer;
    LayersProxyModel &mRecordingLayerPM;
    LocalProjectsManager &mLocalProjectsManager;
    InputMapSettings *mMapSettings = nullptr;
    MerginApi *mMerginApi = nullptr;

    std::unique_ptr<AutosyncController> mAutosyncController;

    QString mProjectLoadingLog;
    QString mProjectRole;

    /**
    * Reloads project.
    * \param filePath Path to project file.
    * \param force If true, reloads mProject on given path withload loading signals - suppose to be called internally,
    * otherwise used only for loading a new projects (evoked by a user).
    */
    bool forceLoad( const QString &filePath, bool force );
    QString mMapTheme;
};

#endif // ACTIVEPROJECT_H
