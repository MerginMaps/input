/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick
import QtCore
import QtQuick.Controls
import QtMultimedia
import QtQml.Models
import QtPositioning
import QtQuick.Dialogs
import QtQuick.Layouts

import mm 1.0 as MM

import "./map"
import "./dialogs"
import "./layers"
import "./components"
import "./project"
import "./settings"
import "./gps"
import "./form"

ApplicationWindow {
  id: window

  visible: true
  width:  __appwindowwidth
  height: __appwindowheight
  visibility: __appwindowvisibility
  flags: {
    if ( Qt.platform.os === "ios" ) {
      return Qt.Window | Qt.MaximizeUsingFullscreenGeometryHint
    }
    else if ( Qt.platform.os !== "ios" && Qt.platform.os !== "android" ) {
      return Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint |
          Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint
    }

    return Qt.Window
  }

  title: "Mergin Maps" // Do not translate

  readonly property bool isPortraitOrientation: ( Screen.primaryOrientation === Qt.PortraitOrientation
                                                 || Screen.primaryOrientation === Qt.InvertedPortraitOrientation )

  onIsPortraitOrientationChanged: recalculateSafeArea()

  Item {
    id: stateManager
    state: "map"

    states: [
      State {
        name: "map" // Working with map in an opened project - view, record, stakeout, form, ...
      },
      State {
        name: "projects" // Listing projects
      },
      State {
        name: "misc" // Settings, GPS panel, ..
      }
    ]

    onStateChanged: {
      if ( stateManager.state === "map" ) {
        map.state = "view"

        // Stop/Start sync animation when user goes to map
        syncButton.iconRotateAnimationRunning = ( __syncManager.hasPendingSync( __activeProject.projectFullName() ) )
      }
      else if ( stateManager.state === "projects" ) {
        projectController.openPanel()
      }

      if ( stateManager.state !== "map" ) {
        map.state = "inactive";
      }
    }
  }

  Settings {
    // start window where it was closed last time
    property alias x: window.x
    property alias y: window.y
    property alias width: window.width
    property alias height: window.height
  }

  function showProjError(message) {
    projDialog.detailedDescription = message
    projDialog.open()
  }

  function selectFeature( pair ) {
    let hasNullGeometry = pair.feature.geometry.isNull

    if ( hasNullGeometry ) {
      formsStackManager.openForm( pair, "readOnly", "form" )
    }
    else if ( pair.valid ) {
      map.highlightPair( pair )
      formsStackManager.openForm( pair, "readOnly", "preview")
    }
  }

  Component.onCompleted: {

    // load default project
    if ( __appSettings.defaultProject ) {
      let path = __appSettings.defaultProject

      if ( __localProjectsManager.projectIsValid( path ) && __activeProject.load( path ) ) {
        __appSettings.activeProject = path
      }
      else {
        // if default project load failed, delete default setting
        __appSettings.defaultProject = ""
        stateManager.state = "projects"
      }
    }
    else {
      stateManager.state = "projects"
    }

    // Catch back button click (if no other component catched it so far)
    // to prevent QT from quitting the APP immediately
    contentItem.Keys.released.connect( function( event ) {
      if ( event.key === Qt.Key_Back ) {
        event.accepted = true
        window.backButtonPressed()
      }
    } )

    console.log("Application initialized!")
  }

  MMMapController {
    id: map

    height: window.height - mapToolbar.height
    width: window.width

    mapExtentOffset: {
      // offset depends on what panels are visible.
      // we need to subtract mapToolbar's height from any visible panel
      // because panels start at the bottom of the screen, but map canvas's height is lowered
      // by mapToolbar's height.
      if ( stakeoutPanelLoader.active )
      {
        // if stakeout panel is opened
        return stakeoutPanelLoader.item.panelHeight - mapToolbar.height
      }
      else if ( measurePanelLoader.active )
      {
        return measurePanelLoader.item.panelHeight - mapToolbar.height
      }
      else if ( formsStackManager.takenVerticalSpace > 0 )
      {
        // if feature preview panel is opened
        return formsStackManager.takenVerticalSpace - mapToolbar.height
      }

      return 0
    }

    onFeatureIdentified: function( pair ) {
      formsStackManager.openForm( pair, "readOnly", "preview" );
    }

    onFeaturesIdentified: function( pairs ) {
      formsStackManager.closeDrawer()
      featurePairSelection.showPairs( pairs );
    }

    onNothingIdentified: {
      formsStackManager.closeDrawer()
    }

    onRecordingFinished: function( pair ) {
      formsStackManager.openForm( pair, "add", "form" )
      map.highlightPair( pair )
    }

    onEditingGeometryStarted: formsStackManager.geometryEditingStarted()
    onEditingGeometryFinished: function( pair ) {
      formsStackManager.geometryEditingFinished( pair )
    }
    onEditingGeometryCanceled: {
      formsStackManager.geometryEditingFinished( null, false )
    }

    onRecordInLayerFeatureStarted: formsStackManager.geometryEditingStarted()
    onRecordInLayerFeatureFinished: function( pair ) {
      formsStackManager.recordInLayerFinished( pair )
    }
    onRecordInLayerFeatureCanceled: {
      formsStackManager.recordInLayerFinished( null, false )
    }

    onSplittingStarted: formsStackManager.hideAll()
    onSplittingFinished: {
      formsStackManager.closeAll()
    }
    onSplittingCanceled: {
      formsStackManager.reopenAll()
    }
    onAccuracyButtonClicked: {
      gpsDataDrawerLoader.active = true
      gpsDataDrawerLoader.focus = true
    }

    onStakeoutStarted: function( pair ) {
      stakeoutPanelLoader.active = true
      stakeoutPanelLoader.focus = true
      stakeoutPanelLoader.item.targetPair = pair
    }

    onMeasureStarted: function( pair ) {
      measurePanelLoader.active = true
      measurePanelLoader.focus = true
    }

    onLocalChangesPanelRequested: {
      stateManager.state = "projects"
      projectController.openChangesPanel( __activeProject.projectFullName(), true )
    }

    onOpenTrackingPanel: {
      trackingPanelLoader.active = true
    }

    onOpenStreamingPanel: {
      streamingModeDialog.open()
    }

    Component.onCompleted: {
      __activeProject.mapSettings = map.mapSettings
      __iosUtils.positionKit = __positionKit
      __iosUtils.compass = map.compass
      __variablesManager.compass = map.compass
      __variablesManager.positionKit = __positionKit
    }
  }

  LocationPermission {
    id: locationPermission
    accuracy: LocationPermission.Precise
  }

  MMToolbar {
    id: mapToolbar

    anchors.bottom: parent.bottom
    visible: map.state === "view"

    model: ObjectModel {

      MMToolbarButton {
        id: syncButton

        text: qsTr("Sync")
        iconSource: __style.syncIcon
        onClicked: {
          __activeProject.requestSync()
        }
      }

      MMToolbarButton {
        text: qsTr("Add")
        iconSource: __style.addIcon
        visible: __activeProject.projectRole !== "reader"
        onClicked: {
          if ( __recordingLayersModel.rowCount() > 0 ) {
            stateManager.state = "map"
            map.record()
          }
          else {
            __notificationModel.addInfo( qsTr( "No editable layers found." ) )
          }
        }
      }

      MMToolbarButton {
        text: qsTr("Layers")
        iconSource: __style.layersIcon
        onClicked: {
          stateManager.state = "misc"
          let layerspanel = mapPanelsStackView.push( layersPanelComponent, {}, StackView.PushTransition )
        }
      }

      MMToolbarButton {
        text: qsTr("Projects")
        iconSource: __style.homeIcon
        onClicked: {
          stateManager.state = "projects"
        }
      }

      MMToolbarButton {
        text: qsTr("Zoom to project")
        iconSource: __style.zoomToProjectIcon
        onClicked: {
          map.centeredToGPS = false
          __inputUtils.zoomToProject( __activeProject.qgsProject, map.mapSettings )
        }
      }

      MMToolbarButton {
        text: qsTr("Map themes")
        iconSource: __style.mapThemesIcon
        onClicked: {
          mapThemesPanel.visible = true
          stateManager.state = "misc"
        }
      }

      MMToolbarButton {
        id: positionTrackingButton

        text: qsTr("Position tracking")
        iconSource: __style.positionTrackingIcon
        active: map.isTrackingPosition
        visible: __activeProject.positionTrackingSupported

        onClicked: {
          trackingPanelLoader.active = true
        }
      }

      MMToolbarButton {
        text: qsTr("Measure")
        iconSource: __style.measurementToolIcon
        onClicked: map.measure()
      }

      MMToolbarButton {
        text: qsTr("Local changes")
        iconSource: __style.localChangesIcon
        onClicked: {
          stateManager.state = "projects"
          projectController.openChangesPanel( __activeProject.projectFullName(), true )
        }
      }

      MMToolbarButton {
        text: qsTr("Settings")
        iconSource: __style.settingsIcon
        onClicked: {
          settingsController.open()
        }
      }
    }
  }

  MMSettingsController {
    id: settingsController

    onOpened: {
      stateManager.state = "misc"
    }

    onClosed: {
      stateManager.state = "map"
    }
  }

  MMProjectController {
    id: projectController

    height: window.height
    width: window.width

    activeProjectId: __activeProject.localProject.id() ?? ""

    onVisibleChanged: {
      if ( projectController.visible ) {
        projectController.forceActiveFocus()
      }
    }

    onOpenProjectRequested: function( projectPath ) {
      __activeProject.load( projectPath )
    }

    onClosed: stateManager.state = "map"
  }

  StackView {
    id: mapPanelsStackView

    //
    // View that can show panels on top of the map,
    // like layers panel, settings and similar
    //

    anchors.fill: parent

    pushEnter: Transition {
      YAnimator {
        to: 0
        from: mapPanelsStackView.height
        duration: 400
        easing.type: Easing.OutCubic
      }
    }

    pushExit: Transition {}

    popEnter: Transition {}

    popExit: Transition {
      YAnimator {
        to: mapPanelsStackView.height
        from: 0
        duration: 400
        easing.type: Easing.OutCubic
      }
    }
  }

  Component {
    id: layersPanelComponent

    MMLayersController {

      onClose: function() {
        mapPanelsStackView.clear( StackView.PopTransition )
        stateManager.state = "map"
      }

      onSelectFeature: function( featurePair ) {
        // close layers panel if the feature has geometry
        if ( __inputUtils.isSpatialLayer( featurePair.layer ) )
        {
          close()
        }

        window.selectFeature( featurePair )
      }

      onAddFeature: function( targetLayer ) {
        let newPair = __inputUtils.createFeatureLayerPair( targetLayer, __inputUtils.emptyGeometry(), __variablesManager )
        formsStackManager.openForm( newPair, "add", "form" )

        // If we start supporting addition of spatial features from the layer's list,
        // make sure to change the root state here to "map"
      }
    }
  }

  Component {
    id: gpsDataDrawerComponent

    MMGpsDataDrawer {
      id: gpsDataDrawer

      mapSettings: map.mapSettings

      // disable the receivers button when staking out
      showReceiversButton: !stakeoutPanelLoader.active

      onManageReceiversClicked: {
        gpsDataDrawer.close()
        settingsController.open( MMSettingsController.Pages.GPSConnection )
      }

      onClosed: {
        gpsDataDrawerLoader.active = false
      }
    }
  }

  Loader {
    id: gpsDataDrawerLoader

    asynchronous: true
    active: false
    focus: true

    sourceComponent: gpsDataDrawerComponent

    onActiveChanged: {
      if ( active )
      {
        gpsDataDrawerLoader.item?.open()
      }
    }
  }

  MMMapThemeDrawer {
    id: mapThemesPanel

    maxHeight: ( window.height / 2 )
    width: window.width
    edge: Qt.BottomEdge

    onClosed: stateManager.state = "map"
  }

  MMStreamingModeDialog {
    id: streamingModeDialog

    streamingActive: map.isStreaming

    onStreamingBtnClicked: {
      map.toggleStreaming()
    }
  }

  Loader {
    id: trackingPanelLoader

    focus: true
    active: false
    asynchronous: true

    sourceComponent: Component {

      MMPositionTrackingDialog {

        width: window.width

        trackingActive: map.isTrackingPosition

        distanceTraveled: trackingPrivate.getDistance()
        trackingStartedAt: trackingPrivate.getStartingTime()

        onTrackingBtnClicked: map.setTracking( !trackingActive )

        onClosed: {
          trackingPanelLoader.active = false
        }

        QtObject {
          id: trackingPrivate

          function getDistance() {
            if ( map.isTrackingPosition ) {
              return __inputUtils.geometryLengthAsString( map.trackingManager?.trackedGeometry )
            }
            return qsTr( "not tracking" )
          }

          function getStartingTime() {
            if ( map.isTrackingPosition )
            {
              return map.trackingManager?.elapsedTimeText
            }
            return qsTr( "not tracking" )
          }
        }
      }
    }

    onActiveChanged: {
      if ( active )
      {
        trackingPanelLoader.item?.open()
      }
    }
  }

  MMProjectIssuesPage {
    id: projectIssuesPage

    height: window.height
    width: window.width
    visible: false

    onVisibleChanged: {
      if (projectIssuesPage.visible)
        projectIssuesPage.focus = true; // get focus
    }
  }

  Loader {
    id: stakeoutPanelLoader

    focus: true
    active: false
    asynchronous: true

    sourceComponent: stakeoutPanelComponent
  }

  Component {
    id: stakeoutPanelComponent

    MMStakeoutDrawer {
      id: stakeoutPanel

      width: window.width

      mapCanvas: map

      onStakeoutFinished: {
        map.stopStakeout()
        formsStackManager.openForm( targetPair, "readOnly", "preview" )
        stakeoutPanelLoader.active = false
      }

      onPanelHeightUpdated: map.updatePosition()
    }
  }

  Loader {
    id: measurePanelLoader

    focus: true
    active: false
    asynchronous: true

    sourceComponent: measurePanelComponent
  }

  Component {
    id: measurePanelComponent

    MMMeasureDrawer {
      id: measurePanel

      width: window.width
      mapTool: map.mapToolComponent

      onMeasureFinished: {
        measurePanelLoader.active = false
        map.finishMeasure()
      }
    }
  }

  MMFormStackController {
    id: formsStackManager

    height: window.height
    width: window.width

    project: __activeProject.qgsProject

    onCreateLinkedFeatureRequested: function( targetLayer, parentPair )  {
      if ( __inputUtils.isNoGeometryLayer( targetLayer) ) {
        let newPair = __inputUtils.createFeatureLayerPair( targetLayer, __inputUtils.emptyGeometry(), __variablesManager )
        recordInLayerFinished( newPair, true )
      }
      else { // we will record geometry
        stateManager.state = "map"
        map.recordInLayer( targetLayer, parentPair )
      }
    }

    onEditGeometryRequested: function( pair ) {
      stateManager.state = "map"
      map.edit( pair )
    }

    onClosed: {
      if ( mapPanelsStackView.depth ) {
        // this must be layers panel as it is the only thing on the stackview currently
        const item = mapPanelsStackView.get( 0 )
        item.forceActiveFocus()
        stateManager.state = "misc"
      }
      else if ( gpsDataDrawerLoader.active )
      {
        stateManager.state = "misc"
        // do nothing, gps page already has focus
      }
      else {
        stateManager.state = "map"
      }

      map.hideHighlight()
    }

    onStakeoutFeature: function( feature ) {
      if ( !__inputUtils.isPointLayerFeature( feature ) )
        return;
      if ( !__positionKit.hasPosition )
      {
        __notificationModel.addWarning( qsTr( "Stake out is disabled because location is unavailable!" ) );
        return;
      }

      map.stakeout( feature )
      closeDrawer()
    }

    onPreviewPanelChanged: function( panelHeight ) {
      map.jumpToHighlighted( panelHeight - mapToolbar.height )
    }
  }

  MMProjectLoadingPage {
    id: projectLoadingPage

    anchors.fill: parent
    visible: false
  }

  MMStorageLimitDialog {
    id: storageLimitDialog

    dataUsing: "%1 / %2".arg(__inputUtils.bytesToHumanSize(__merginApi.workspaceInfo.diskUsage)).arg(__inputUtils.bytesToHumanSize(__merginApi.workspaceInfo.storageLimit))
    usedData: __merginApi.workspaceInfo.storageLimit > 0 ? __merginApi.workspaceInfo.diskUsage / __merginApi.workspaceInfo.storageLimit : 0
    apiSupportsSubscription: __merginApi.apiSupportsSubscriptions

    onManageAccountClicked: Qt.openUrlExternally(__inputHelp.merginSubscriptionLink)
  }

  MMProjectLimitDialog {
    id: projectLimitDialog

    apiSupportsSubscription: __merginApi.apiSupportsSubscriptions
    onManageAccountClicked: Qt.openUrlExternally(__inputHelp.merginSubscriptionLink)
  }

  MMProjErrorDialog {
    id: projDialog
  }

  MMOutOfDateCustomServerDialog{
    id: migrationDialog

    property string version

    onIgnoreClicked: {
      __appSettings.ignoreMigrateVersion = version
    }
  }

  MMProjectLoadErrorDialog {
    id: projectErrorDialog

    onClosed: {
      projectLoadingPage.visible = false
      projectController.openPanel()
    }
  }

  MMMigrateToMerginDialog {
    id: migrateToMerginDialog

    onMigrationRequested: __syncManager.migrateProjectToMergin( __activeProject.projectFullName() )
  }

  MMNoPermissionsDialog {
    id: noPermissionsDialog
  }

  MMSyncFailedDialog {
    id: syncFailedDialog
  }

  MMMissingAuthDialog {
    id: missingAuthDialog

    onSingInRequested: {
      stateManager.state = "projects"
      projectController.showLogin()
    }
  }

  MMNotificationView {}

  MMListDrawer {
    id: featurePairSelection

    drawerHeader.title: qsTr( "Select feature" )
    list.model: MM.FeaturesModel {}

    list. delegate: MMListDelegate {
      text: model.FeatureTitle
      secondaryText: model.LayerName
      leftContent: MMIcon { source: model.LayerIcon }
      onClicked: {
        let pair = model.FeaturePair
        featurePairSelection.close()
        map.highlightPair( pair )
        formsStackManager.openForm( pair, "readOnly", "preview" );
      }
    }

    function showPairs( pairs ) {
      if ( pairs.length > 0 )
      {
        list.model.populateStaticModel( pairs )
        open()
      }
    }
  }

  MMWelcomeToNewDesignDialog {
    id: welcomeToNewDesignDialog

    Component.onCompleted: {
      if ( __showWelcomeToNewDesignDialog )
        open()
    }
  }

  Connections {
    target: __syncManager
    enabled: stateManager.state === "map"

    function onSyncStarted( projectFullName )
    {
      if ( projectFullName === __activeProject.projectFullName() )
      {
        syncButton.iconRotateAnimationRunning = true
      }
    }

    function onSyncFinished( projectFullName, success )
    {
      if ( projectFullName === __activeProject.projectFullName() )
      {
        syncButton.iconRotateAnimationRunning = false

        if ( success )
        {
          __notificationModel.addSuccess( qsTr( "Successfully synchronised" ) )

          // refresh canvas
          map.refreshMap()
        }
      }
    }

    function onSyncCancelled( projectFullName )
    {
      if ( projectFullName === __activeProject.projectFullName() )
      {
        syncButton.iconRotateAnimationRunning = false
      }
    }

    function onSyncError( projectFullName, errorType, willRetry, errorMessage )
    {
      if ( projectFullName === __activeProject.projectFullName() )
      {
        if ( errorType === MM.SyncError.NotAMerginProject )
        {
          migrateToMerginDialog.open()
        }
        else if ( errorType === MM.SyncError.NoPermissions )
        {
          noPermissionsDialog.open()
        }
        else if ( errorType === MM.SyncError.AnotherProcessIsRunning && willRetry )
        {
          // just banner that we will try again
          __notificationModel.addInfo( qsTr( "Somebody else is syncing, we will try again later" ) )
        }
        else
        {
          syncFailedDialog.detailedText = qsTr( "Details" ) + ": " + errorMessage
          if ( willRetry )
          {
            __notificationModel.addError( qsTr( "There was an issue during synchronisation, we will try again. Click to learn more" ),
            MM.NotificationType.ShowSyncFailedDialog )
          }
          else
          {
            syncFailedDialog.open()
          }
        }
      }
    }
  }

  Connections {
    target: __merginApi
    function onNetworkErrorOccurred( message, topic, httpCode, projectFullName ) {
      if ( stateManager.state === "projects" )
      {
        var msg = message ? message : qsTr( "Failed to communicate with server. Try improving your network connection." )
        __notificationModel.addError( msg )
      }
    }

    function onStorageLimitReached( uploadSize ) {
      __merginApi.getUserInfo()
      if (__merginApi.apiSupportsSubscriptions) {
        __merginApi.getWorkspaceInfo()
      }
      storageLimitDialog.dataToSync = __inputUtils.bytesToHumanSize(uploadSize)
      storageLimitDialog.open()
    }

    function onProjectLimitReached( maxProjects, errorMsg ) {
      __merginApi.getUserInfo()
      if (__merginApi.apiSupportsSubscriptions) {
        __merginApi.getWorkspaceInfo()
      }
      projectLimitDialog.maxProjectNumber = maxProjects
      projectLimitDialog.open()

      syncButton.iconRotateAnimationRunning = false
    }

    function onProjectDataChanged( projectFullName ) {
      //! if current project has been updated, refresh canvas
      if ( projectFullName === projectController.activeProjectId ) {
        map.mapSettings.extentChanged()
      }
    }

    function onMigrationRequested( version ) {
      if( __appSettings.ignoreMigrateVersion !== version ) {
        migrationDialog.version = version
        migrationDialog.open()
      }
    }

    function onMissingAuthorizationError( projectFullName )
    {
      if ( projectFullName === __activeProject.projectFullName() )
      {
        missingAuthDialog.open()
      }
    }

    function onProjectAlreadyOnLatestVersion( projectFullName )
    {
      if ( projectFullName === __activeProject.projectFullName() )
      {
        __notificationModel.addSuccess( qsTr( "Up to date" ) )
      }
    }
  }

  Connections {
    target: __inputProjUtils
    function onProjError( message ) {
      showProjError(message)
    }
  }

  Connections {
    target: __notificationModel
    function onShowProjectIssuesActionClicked() {
      projectIssuesPage.projectLoadingLog = __activeProject.projectLoadingLog();
      projectIssuesPage.visible = true;
    }
    function onShowSwitchWorkspaceActionClicked() {
      stateManager.state = "projects"
      projectController.showSelectWorkspacePage()
    }
    function onShowSyncFailedDialogClicked() {
      syncFailedDialog.open()
    }
  }

  Connections {
    target: __activeProject

    function onLoadingStarted() {
      projectLoadingPage.visible = true;
      projectIssuesPage.clear();
    }

    function onLoadingFinished() {
      projectLoadingPage.visible = false

      if ( __activeProject.isProjectLoaded() )
      {
        projectController.hidePanel()
      }

      // check location permission
      if ( locationPermission.status === Qt.Undetermined ) {
        // This is the place where we actually request permissions.
        // When the system's request permissions dialog get closed,
        // we get a notification that our application is active again,
        // and PositionKit::appStateChanged() will try to start updates.
        locationPermission.request();
      }
      else if ( locationPermission.status === Qt.Denied ) {
        __inputUtils.log("Permissions", "Location permission is denied")
      }
    }

    function onLoadingErrorFound() {
      __notificationModel.addWarning(
        __inputUtils.htmlLink(qsTr( "There were issues loading the project. %1View details%2" ), __style.forestColor),
        MM.NotificationType.ShowProjectIssuesAction
      )
    }

    function onReportIssue( title, message ) {
      projectIssuesPage.reportIssue( title, message )
    }

    function onProjectReloaded( project ) {
      map.clear()

      __appSettings.defaultProject = __activeProject.localProject.qgisProjectFilePath ?? ""
      __appSettings.activeProject = __activeProject.localProject.qgisProjectFilePath ?? ""
    }

    function onProjectWillBeReloaded() {
      formsStackManager.reload()
    }

    function onProjectReadingFailed( message ) {
      projectErrorDialog.informativeText = qsTr( "Could not read the project file:" ) + "\n" + message
      projectErrorDialog.open()
    }

    function onPositionTrackingSupportedChanged() {
      positionTrackingButton.visible = __activeProject.positionTrackingSupported
      mapToolbar.recalculate()
    }
  }

  Timer {
    id: closeAppTimer

    interval: 3000
    running: false
    repeat: false
  }

  function backButtonPressed() {

    if ( closeAppTimer.running ) {
      __inputUtils.quitApp()
    }
    else {
      closeAppTimer.start()
      __notificationModel.addInfo( qsTr( "Press back again to quit the app" ) )
    }
  }

  function recalculateSafeArea() {
    let safeArea = []

    // Should be merged in future with the same code in main.cpp
    if ( Qt.platform.os === "ios" ) {
      safeArea = Array.from( __iosUtils.getSafeArea() )
    }
    else if ( Qt.platform.os === "android" ) {
      safeArea = Array.from( __androidUtils.getSafeArea() )

      // Values from Android API must be divided by dpr
      safeArea[0] = safeArea[0] / Screen.devicePixelRatio
      safeArea[1] = safeArea[1] / Screen.devicePixelRatio
      safeArea[2] = safeArea[2] / Screen.devicePixelRatio
      safeArea[3] = safeArea[3] / Screen.devicePixelRatio
    }

    if ( safeArea.length === 4 ) {
      __style.safeAreaTop = safeArea[0]
      __style.safeAreaRight = safeArea[1]
      __style.safeAreaBottom = safeArea[2]
      __style.safeAreaLeft = safeArea[3]
    }
  }
}
