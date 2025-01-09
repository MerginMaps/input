/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Shapes

import mm 1.0 as MM

import "../components"
import "./components"
import "../dialogs"

Item {
  id: root

  // mapExtentOffset represents a height (or a portion) of canvas which is occupied by some other component
  // like preview panel or stakeout panel. Map extent thus must be calculated regarding to this
  // offset in order to not highlight features in the occupied area, but rather move canvas
  property real mapExtentOffset

  readonly property alias gpsIndicatorColor: gpsStateGroup.indicatorColor

  readonly property alias mapSettings: mapCanvas.mapSettings
  readonly property alias compass: deviceCompass

  property bool isTrackingPosition: trackingManager?.isTrackingPosition ?? false
  property bool isStreaming: recordingToolsLoader.active ? recordingToolsLoader.item.recordingMapTool.recordingType === MM.RecordingMapTool.StreamMode : false
  property bool centeredToGPS: false

  property var mapToolComponent: {
    measurementToolsLoader.active ? measurementToolsLoader.item.mapTool : null
  }

  property MM.PositionTrackingManager trackingManager: tracking.item?.manager ?? null

  signal featureIdentified( var pair )
  signal featuresIdentified( var pairs )
  signal nothingIdentified()

  signal recordingStarted()
  signal recordingFinished( var pair )
  signal recordingCanceled()

  signal editingGeometryStarted()
  signal editingGeometryFinished( var pair )
  signal editingGeometryCanceled()

  signal recordInLayerFeatureStarted()
  signal recordInLayerFeatureFinished( var pair )
  signal recordInLayerFeatureCanceled()

  signal splittingStarted()
  signal splittingFinished()
  signal splittingCanceled()

  signal stakeoutStarted( var pair )
  signal accuracyButtonClicked()

  signal measureStarted()

  signal localChangesPanelRequested()

  signal openTrackingPanel()
  signal openStreamingPanel()

  states: [
    State {
      name: "view"
    },
    State {
      name: "record"
    },
    State {
      // recording feature in specific layer without option to change the recording layer,
      // used to create linked features in relations and value relations
      name: "recordInLayer"
    },
    State {
      name: "edit" // of existing feature
    },
    State {
      name: "split" // of existing feature
    },
    State {
      name: "stakeout"
    },
    State {
      name: "measure"
    },
    State {
      name: "inactive" // ignores touch input
    }
  ]

  onStateChanged: {
    // We call this first because when previous state is 'view' and `centeredToGPS` is true
    // the map may still not be centered. It's only centered after a certain threshold is exceeded.
    updatePosition()

    switch ( state ) {
      case "record": {
        root.showInfoTextMessage( qsTr( "Mark the geometry on the map and click record" ) )

        if ( __appSettings.autolockPosition ) { // center to GPS
          if ( gpsStateGroup.state === "unavailable" ) {
            __notificationModel.addError( "GPS currently unavailable." )
          }
          else {
            root.centeredToGPS = true
            updatePosition()
          }
        }

        root.recordingStarted()
        break
      }

      case "recordInLayer": {
        root.recordInLayerFeatureStarted()
        root.hideHighlight()
        break
      }

      case "edit": {
        root.editingGeometryStarted()
        root.hideHighlight()
        break
      }

      case "split": {
        root.showInfoTextMessage( qsTr( "Create line to split the selected feature" ) )
        root.splittingStarted()
        break
      }

      case "view": {
        root.hideHighlight()
        break
      }

      case "stakeout": {
        root.hideHighlight()
        root.stakeoutStarted( internal.stakeoutTarget )
        break
      }

      case "measure": {
        root.showInfoTextMessage( qsTr( "Add points to measure distance, close the shape to measure area" ) )
        root.hideHighlight()
        root.measureStarted()
        break
      }

      case "inactive": {
        break
      }
    }
  }

  state: "view"

  // GPS states, different from above parent.state!
  StateGroup {
    id: gpsStateGroup

    property color indicatorColor: __style.negativeColor

    states: [
      State {
        name: "good" // GPS provides position AND horizontal accuracy is below set tolerance (threshold)
        when: __positionKit.hasPosition && __positionKit.horizontalAccuracy > 0 && __positionKit.horizontalAccuracy <= __appSettings.gpsAccuracyTolerance
        PropertyChanges {
          target: gpsStateGroup
          indicatorColor: __style.positiveColor
        }
      },
      State {
        name: "low" // below accuracy tolerance OR GPS does not provide horizontal accuracy
        when: __positionKit.hasPosition &&  (__positionKit.horizontalAccuracy < 0 || __positionKit.horizontalAccuracy > __appSettings.gpsAccuracyTolerance )
        PropertyChanges {
          target: gpsStateGroup
          indicatorColor: __style.warningColor
        }
      },
      State {
        name: "unavailable" // GPS does not provide position
        when: !__positionKit.hasPosition
        PropertyChanges {
          target: gpsStateGroup
          indicatorColor: __style.negativeColor
        }
      }
    ]
  }

  // map background
  Rectangle {
    anchors.fill: parent
    color: __style.polarColor
  }

  MMMapCanvas {
    id: mapCanvas

    anchors.fill: parent

    mapSettings.project: __activeProject.qgsProject

    MM.IdentifyKit {
      id: identifyKit

      mapSettings: mapCanvas.mapSettings
      identifyMode: MM.IdentifyKit.TopDownAll
    }

    onClicked: function( point ) {
      if ( root.state === "view" )
      {
        let screenPoint = Qt.point( point.x, point.y )
        let pair = identifyKit.identifyOne( screenPoint )

        if ( pair.valid )
        {
          root.highlightPair( pair )
          root.featureIdentified( pair )
        }
        else
        {
          root.hideHighlight()
          root.nothingIdentified()
        }
      }
    }

    onUserInteractedWithMap: root.centeredToGPS = false

    onLongPressed: function( point ) {
      // Alter position of simulated provider
      if ( __positionKit.positionProvider && __positionKit.positionProvider.id() === "simulated" )
      {
        __positionKit.positionProvider.setPosition( __inputUtils.mapPointToGps( Qt.point( point.x, point.y ), mapCanvas.mapSettings ) )
      }

      if ( root.state === "view" )
      {
        let screenPoint = Qt.point( point.x, point.y )
        let pairs = identifyKit.identify( screenPoint )

        if ( !pairs.isEmpty )
        {
          root.featuresIdentified( pairs )
        }
        else
        {
          root.hideHighlight()
          root.nothingIdentified()
        }
      }
    }

    onIsRenderingChanged: {
      if ( root.state === "inactive" ) {
        fadeInAnimation.stop()
        fadeOutAnimation.start()
      }

      if ( isRendering ) {
        fadeOutAnimation.stop()
        fadeInAnimationTimer.start()
      }
      else {
        fadeInAnimation.stop()
        fadeOutAnimation.start()
      }
    }
  }

  MMHighlight {
    id: identifyHighlight

    visible: root.state === "view"
    anchors.fill: mapCanvas

    mapSettings: mapCanvas.mapSettings
  }

  Loader {
    id: stakeoutLoader

    anchors.fill: mapCanvas

    asynchronous: true
    active: root.state === "stakeout"

    sourceComponent: stakeoutToolsComponent
  }

  Loader {
    id: tracking

    anchors.fill: mapCanvas
    asynchronous: true
    active: false

    sourceComponent: Component {
      Item {
        property alias manager: trackingManager

        MM.PositionTrackingManager {
          id: trackingManager

          variablesManager: __variablesManager
          qgsProject: __activeProject.qgsProject

          onAbort: () => root.setTracking( false )
          onTrackingErrorOccured: ( message ) => __notificationModel.addError( message )
        }

        MM.PositionTrackingHighlight {
          id: trackingHighlight

          mapPosition: mapPositionSource.mapPosition
          trackedGeometry: __inputUtils.transformGeometryToMapWithCRS( trackingManager.trackedGeometry, trackingManager.crs(), mapCanvas.mapSettings )
        }

        MMHighlight {
          height: mapCanvas.height
          width: mapCanvas.width

          markerColor: __style.sunsetColor
          lineColor: __style.sunsetColor
          lineWidth: MMHighlight.LineWidths.Narrow

          mapSettings: mapCanvas.mapSettings
          geometry: trackingHighlight.highlightGeometry
        }

        Component.onCompleted: {
          trackingManager.trackingBackend = trackingManager.constructTrackingBackend( __activeProject.qgsProject, __positionKit )
        }

        Connections {
          target: __activeProject

          function onProjectWillBeReloaded() {
            // simply stop tracking
            root.setTracking( false )
          }
        }
      }
    }
  }

  MMPositionMarker {
    id: positionMarker

    xPos: mapPositionSource.screenPosition.x
    yPos: mapPositionSource.screenPosition.y
    hasDirection: positionDirectionSource.hasDirection

    direction: positionDirectionSource.direction
    hasPosition: __positionKit.hasPosition

    horizontalAccuracy: __positionKit.horizontalAccuracy
    accuracyRingSize: mapPositionSource.screenAccuracy

    trackingMode: root.state !== "inactive" && tracking.active
  }

  Loader {
    id: recordingToolsLoader

    anchors.fill: mapCanvas

    asynchronous: true
    active: internal.isInRecordState

    sourceComponent: recordingToolsComponent
  }

  Loader {
    id: splittingLoader

    anchors.fill: mapCanvas

    asynchronous: true
    active: root.state === "split"

    sourceComponent: splittingToolsComponent
  }

  Loader {
    id: measurementToolsLoader

    anchors.fill: mapCanvas

    asynchronous: true
    active: root.state === "measure"

    sourceComponent: measurementToolsComponent
  }

  // map available content within safe area
  Item {
    anchors {
      fill: parent
      leftMargin: __style.safeAreaLeft + __style.mapButtonsMargin
      topMargin: __style.safeAreaTop + __style.margin10
      rightMargin: __style.safeAreaRight + __style.mapButtonsMargin
      bottomMargin: __style.mapButtonsMargin
    }

    Item {
      // top buttons group
      width: parent.width
      height: topContentLayout.implicitHeight

      Column {
        id: topContentLayout

        width: parent.width

        spacing: __style.margin20

        RowLayout {
          width: parent.width

          spacing: __style.margin20

          visible: internal.isInRecordState || root.state === "split"

          MMMapButton {
            id: backButton

            visible: internal.isInRecordState || root.state === "split"
            iconSource: __style.backIcon

            onClicked: {
              if ( internal.isInRecordState ) {
                if ( recordingToolsLoader.item.hasChanges() ) {
                  cancelEditDialog.open()
                }
                else {
                  recordingToolsLoader.item.discardChanges()
                }
              }
              else if ( root.state === "split" ) {
                root.hideInfoTextMessage()
                root.splittingCanceled()
                root.state = "view"
              }
            }
          }

          MMMapPicker {

            Layout.preferredHeight: __style.mapItemHeight
            Layout.preferredWidth: parent.width - parent.spacing - backButton.width
            Layout.maximumWidth: 500 * __dp

            visible: root.state === "record"

            text: __activeLayer.layerName
            leftIconSource: __inputUtils.loadIconFromLayer( __activeLayer.layer )

            onClicked: activeLayerPanel.open()
          }

          Item {
            id: mapBlurInfoBoxHorizontalGroup

            property bool useHorizontalInfoBox: width > 400 * __dp && root.state === "record"

            Layout.preferredHeight: __style.row40
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: true

            MMMapHidingLabel {
              id: mapBlurInfoBoxHorizontal

              width: Math.min( parent.width, 500 * __dp)
              anchors.right: parent.right
            }
          }
        }

        Item {
          id: mapBlurInfoBoxVerticalGroup

          width: parent.width
          height: mapBlurInfoBoxVertical.visible ? mapBlurInfoBoxVertical.height : 0

          visible: !mapBlurInfoBoxHorizontalGroup.useHorizontalInfoBox

          MMMapHidingLabel {
            id: mapBlurInfoBoxVertical

            width: parent.width
          }
        }

        MMMapScaleBar {
          id: scaleBar

          x: parent.width / 2 - width / 2

          mapSettings: mapCanvas.mapSettings
          preferredWidth: 100 * __dp
        }
      }
    }

    Item {
      // bottom buttons group
      width: parent.width

      anchors.bottom: parent.bottom

      anchors.bottomMargin: root.state === "stakeout" || root.state === "measure" ? root.mapExtentOffset : 0

      visible: {
        if ( root.state === "stakeout" || root.state === "measure" )
          return true
        else
          return root.mapExtentOffset > 0 ? false : true
      }

      Column {

        spacing: __style.margin20
        width: parent.width - __style.margin20 - bottomRightButtonsGroup.width

        anchors {
          left: parent.left
          bottom: parent.bottom
        }

        MMMapLabel {
          visible: root.state !== "inactive" && root.isStreaming
          iconSource: __style.streamingIcon

          text: qsTr( "streaming" )
          textBgColorInverted: true

          onClicked: root.openStreamingPanel()
        }

        MMMapLabel {
          visible: root.state !== "inactive" && tracking.active
          iconSource: __style.positionTrackingIcon

          text: {
            if (visible) {
              return root.trackingManager?.elapsedTimeText ?? ""
            }
            else
              return ""
          }
          textBgColorInverted: true

          onClicked: root.openTrackingPanel()
        }

        MMMapLabel {
          maxWidth: implicitWidth > parent.width ? parent.width : implicitWidth

          onClicked: root.accuracyButtonClicked()

          iconSource: __style.satelliteIcon

          bgColor: {
            if ( gpsStateGroup.state === "good" ) {
              return __style.positiveColor
            }
            else if ( gpsStateGroup.state === "low" ) {
              return __style.warningColor
            }
            return __style.negativeColor
          }

          textColor: {
            if ( gpsStateGroup.state === "good" ) {
              return __style.forestColor
            }
            else if ( gpsStateGroup.state === "low" ) {
              return __style.earthColor
            }
            return __style.grapeColor
          }

          visible: {
            if ( root.mapExtentOffset > 0 && root.state !== "stakeout" ) return false

            if ( __positionKit.positionProvider && __positionKit.positionProvider.type() === "external" ) {
              // for external receivers we want to show gps panel and accuracy button
              // even when the GPS receiver is not sending position data
              return true
            }
            else {
              if ( gpsStateGroup.state !== "unavailable" ) {
                return true
              }
              else {
                return false
              }
            }
          }

          text: {
            if ( !__positionKit.positionProvider )
            {
              return ""
            }
            else if ( __positionKit.positionProvider.type() === "external" )
            {
              if ( __positionKit.positionProvider.state === MM.PositionProvider.Connecting )
              {
                return qsTr( "Connecting to %1" ).arg( __positionKit.positionProvider.name() )
              }
              else if ( __positionKit.positionProvider.state === MM.PositionProvider.WaitingToReconnect )
              {
                return __positionKit.positionProvider.stateMessage
              }
              else if ( __positionKit.positionProvider.state === MM.PositionProvider.NoConnection )
              {
                return __positionKit.positionProvider.stateMessage
              }
            }

            if ( !__positionKit.hasPosition )
            {
              return qsTr( "Connected, no position" )
            }
            else if ( Number.isNaN( __positionKit.horizontalAccuracy ) || __positionKit.horizontalAccuracy < 0 )
            {
              return qsTr( "Unknown accuracy" )
            }

            let accuracyText = __inputUtils.formatNumber( __positionKit.horizontalAccuracy, __positionKit.horizontalAccuracy > 1 ? 1 : 2 ) + " m"
            if ( __appSettings.gpsAntennaHeight > 0 )
            {
              let gpsText = Number( __appSettings.gpsAntennaHeight.toFixed( 3 ) ) + " m"
              return gpsText + " / " + accuracyText
            }
            else
            {
              return accuracyText
            }
          }
        }
      }

      Column {
        id: bottomRightButtonsGroup

        width: __style.mapItemHeight

        anchors {
          right: parent.right
          bottom: parent.bottom
        }

        spacing: __style.margin20

        MMBusyIndicator {
          id: loadingIndicator

          anchors.horizontalCenter: parent.horizontalCenter

          icon.source: __style.waitingIcon

          size: 30 * __dp
          speed: 1600

          Timer {
            id: fadeInAnimationTimer

            interval: 500
            onTriggered: {
              if ( mapCanvas.isRendering ) {
                fadeInAnimation.start()
              }
            }
          }

          SequentialAnimation {
            id: fadeInAnimation

            ScriptAction { script: loadingIndicator.running = true }
            NumberAnimation {
              target: loadingIndicator
              property: "opacity"

              from: 0
              to: 1

              duration: 200
            }
          }

          SequentialAnimation {
            id: fadeOutAnimation

            NumberAnimation {
              target: loadingIndicator
              property: "opacity"

              from: 1
              to: 0

              duration: 200
            }
            ScriptAction { script: loadingIndicator.running = false }
          }
        }

        MMMapButton {
          id: moreToolsButton

          // Do not show more menu for SinglePart point layers
          visible: internal.isInRecordState && (!internal.isPointLayer || internal.isMultiPartLayer)

          property string actionState: {
            if (!moreToolsButton.visible)
              return "invisible"

            if (internal.splitGeometryButtonVisible) {
              if (!internal.redrawGeometryButtonVisible && !internal.streamingModeButtonVisible)
                return "split"
            }

            if (internal.redrawGeometryButtonVisible) {
              if (!internal.splitGeometryButtonVisible && !internal.streamingModeButtonVisible)
                return "redraw"
            }

            if (internal.streamingModeButtonVisible) {
              if (!internal.redrawGeometryButtonVisible && !internal.splitGeometryButtonVisible)
                return "stream"
            }
            return "menu"
          }

          iconSource: {
            if (actionState === "split")
              return __style.splitGeometryIcon

            if (actionState === "redraw")
              return __style.redrawGeometryIcon

            if (actionState === "stream")
              return __style.streamingIcon

            return __style.moreIcon
          }

          onClicked: {
            if (actionState === "split")
              return root.toggleSplitting()

            if (actionState === "redraw")
              return root.toggleRedraw()

            if (actionState === "stream")
              return root.openStreamingPanel()

            moreToolsMenuLoader.item.open()
          }
        }

        MMMapButton {
          id: gpsButton

          iconSource: root.centeredToGPS ? __style.followGPSNoColorOverlayIcon : __style.gpsIcon

          onClicked: {
            if ( gpsStateGroup.state === "unavailable" ) {
              __notificationModel.addError( qsTr( "GPS currently unavailable" ) )
              return
            }
            root.centeredToGPS = true
            let screenPt = mapCanvas.mapSettings.coordinateToScreen( mapPositionSource.mapPosition )
            mapCanvas.jumpTo( screenPt )
          }
        }
      }
    }
  }

  MMListDrawer {
    id: activeLayerPanel

    drawerHeader.title: qsTr( "Choose Active Layer" )

    list.model: __recordingLayersModel

    list.delegate: MMListDelegate {
      text: model.layerName

      // TODO: why we need to set hight here?
      height: __style.menuDrawerHeight

      leftContent: MMIcon {
        source: model.iconSource
      }

      rightContent: MMIcon {
        source: __style.doneCircleIcon
        visible: __activeLayer.layerId === model.layerId
      }

      onClicked: {
        __activeProject.setActiveLayer( __recordingLayersModel.layerFromLayerId( model.layerId ) )
        activeLayerPanel.close()
      }
    }

    emptyStateDelegate: MMMessage {
      image: __style.negativeMMSymbolImage
      description: qsTr( "Could not find any editable layers in the project." )
      linkText: qsTr( "See how to enable digitizing in your project." )
      link: __inputHelp.howToEnableDigitizingLink
    }
  }

  Loader {
    id: moreToolsMenuLoader
    active: moreToolsButton.actionState === "menu"
    sourceComponent: moreToolsMenuComponent
  }

  Component {
    id: moreToolsMenuComponent

    MMListDrawer {
      id: moreToolsMenu

      drawerHeader.title: qsTr("More options")

      list.model: ObjectModel {

        MMListDelegate {
          text: qsTr( "Split geometry" )
          leftContent: MMIcon { source: __style.splitGeometryIcon }

          visible: internal.splitGeometryButtonVisible

          onClicked: {
            root.toggleSplitting()
            moreToolsMenu.close()
          }
        }

        MMListDelegate {
          text: qsTr( "Redraw geometry" )
          leftContent: MMIcon { source: __style.redrawGeometryIcon }

          visible: internal.redrawGeometryButtonVisible

          onClicked: {
            root.toggleRedraw()
            moreToolsMenu.close()
          }
        }

        MMListDelegate {
          text: qsTr("Streaming mode")

          visible: internal.streamingModeButtonVisible

          leftContent: MMIcon { source: __style.streamingIcon }
          rightContent: MMBadge {
            text: qsTr( "Active" )
            visible: root.isStreaming
          }

          hasLine: false

          onClicked: {
            root.openStreamingPanel()
            moreToolsMenu.close()
          }
        }
      }
    }
  }

  MMDiscardGeometryChangesDialog {
    id: cancelEditDialog

    state: root.state

    onDiscardChanges: {
      recordingToolsLoader.item.discardChanges()
    }
  }

  MMSplittingFailedDialog {
    id: splittingFailedDialog
  }

  MM.Compass { id: deviceCompass }

  MM.MapPosition {
    id: mapPositionSource

    mapSettings: mapCanvas.mapSettings
    positionKit: __positionKit
    onScreenPositionChanged: root.updatePosition()
  }

  MM.PositionDirection {
    id: positionDirectionSource

    positionKit: __positionKit
    compass: deviceCompass
  }

  Component {
    id: recordingToolsComponent

    MMRecordingTools {
      anchors.fill: parent

      map: mapCanvas
      positionMarkerComponent: positionMarker
      recordingMapTool.centeredToGPS: root.centeredToGPS

      activeFeature: root.state === "edit" ? internal.featurePairToEdit.feature : __inputUtils.emptyFeature()

      onCanceled: {
        root.hideInfoTextMessage()

        if ( root.state === "record" )
        {
          root.recordingCanceled()
        }
        else if ( root.state === "edit" )
        {
          root.editingGeometryCanceled()
        }
        else if ( root.state === "recordInLayer" )
        {
          root.recordInLayerFeatureCanceled()
        }

        root.state = "view"
      }

      onDone: function( featureLayerPair ) {
        root.hideInfoTextMessage()

        if ( root.state === "record" )
        {
          root.recordingFinished( featureLayerPair )
        }
        else if ( root.state === "edit" )
        {
          root.editingGeometryFinished( featureLayerPair )
        }
        else if ( root.state === "recordInLayer" )
        {
          root.recordInLayerFeatureFinished( featureLayerPair )
        }

        root.state = "view"
      }
    }
  }

  Component {
    id: stakeoutToolsComponent

    MMStakeoutTools {
      anchors.fill: parent

      map: mapCanvas
      mapExtentOffset: root.mapExtentOffset

      target: internal.stakeoutTarget
    }
  }

  Component {
    id: measurementToolsComponent

    MMMeasurementTools {
      anchors.fill: parent

      map: mapCanvas
      positionMarkerComponent: positionMarker
      onFinishMeasurement: root.finishMeasure()
    }
  }

  Component {
    id: splittingToolsComponent

    MMSplittingTools {
      anchors.fill: parent

      map: mapCanvas
      featureToSplit: internal.featurePairToEdit

      onDone: function (success) {
        // close all feature forms, show banner if it went fine or not
        root.hideInfoTextMessage()

        if ( success )
        {
          __notificationModel.addSuccess( qsTr( "Splitting done successfully" ) )
        }
        else
        {
          splittingFailedDialog.open()
        }


        root.splittingFinished()

        root.state = "view"
      }

      onCanceled: {
        // go back to feature form
        root.hideInfoTextMessage()

        root.splittingCanceled()

        root.state = "view"
      }
    }
  }

  Connections {
    target: mapCanvas.mapSettings
    function onExtentChanged() {
      scaleBar.show()
    }
  }

  Connections {
    target: __activeProject

    function onStartPositionTracking() {

      if ( !tracking.active )
      {
        root.setTracking( true )
      }
    }
  }

  QtObject {
    id: internal
    // private properties - not accessible by other components

    property var featurePairToEdit // we are editing geometry of this feature layer pair
    property bool isSpatialLayer: __inputUtils.isSpatialLayer( __activeLayer.vectorLayer )
    property bool isPointLayer: __inputUtils.isPointLayer( __activeLayer.vectorLayer )
    property bool isMultiPartLayer: __inputUtils.isMultiPartLayer( __activeLayer.vectorLayer )

    // visibility of buttons in "more" menu
    property bool splitGeometryButtonVisible: !internal.isPointLayer && !root.isStreaming && root.state === "edit"
    property bool redrawGeometryButtonVisible: root.state === "edit"
    property bool streamingModeButtonVisible: !internal.isPointLayer || internal.isMultiPartLayer

    property var extentBeforeStakeout // extent that we return to once stakeout finishes
    property bool centeredToGPSBeforeStakeout
    property var stakeoutTarget

    property bool isInRecordState: root.state === "record" || root.state === "recordInLayer" || root.state === "edit"

    // bottomMapButtonsMargin represents distance between toolbar and the bottom of the map buttons (gps and accuracy)
    // in case the toolbar has the overlaying "record" button, we need to move these two buttons a little higher
    property real bottomMapButtonsMargin: {
      let mapDrawersOffset = root.mapExtentOffset
      let mapMinMargin = __style.mapButtonsMargin
      let toolbarRequiredOffset = 0

      if ( root.state !== "view" && __inputUtils.isLineLayer( __activeLayer.vectorLayer ) ) {
        toolbarRequiredOffset = 16 * __dp
      }

      return mapDrawersOffset + mapMinMargin + toolbarRequiredOffset
    }
  }

  function record() {
    state = "record"
  }

  function recordInLayer( layer, parentpair ) {
    __activeProject.setActiveLayer( layer )
    root.centerToPair( parentpair )
    state = "recordInLayer"
  }

  function edit( featurepair ) {
    __activeProject.setActiveLayer( featurepair.layer )
    root.centerToPair( featurepair )
    root.showInfoTextMessage( qsTr( "Select some point to start editing the geometry" ) )

    internal.featurePairToEdit = featurepair
    state = "edit"
  }

  function toggleRedraw() {
    redraw(internal.featurePairToEdit)
  }

  function redraw( featurepair ) {
    __activeProject.setActiveLayer( featurepair.layer )
    root.centerToPair( featurepair )
    root.showInfoTextMessage( qsTr( "Record new geometry for the feature" ) )

    // clear feature geometry
    internal.featurePairToEdit = __inputUtils.changeFeaturePairGeometry( featurepair, __inputUtils.emptyGeometry() )

    // You should be already in state == "edit"
    if ( recordingToolsLoader.active ) {
      recordingToolsLoader.item.recordingMapTool.state = MM.RecordingMapTool.Record
    }
  }

  function toggleSplitting() {
    split(internal.featurePairToEdit)
  }

  function split( featurepair ) {
    root.centerToPair( featurepair )

    internal.featurePairToEdit = featurepair
    state = "split"
  }

  function stakeout( featurepair ) {
    internal.extentBeforeStakeout = mapCanvas.mapSettings.extent
    internal.centeredToGPSBeforeStakeout = root.centeredToGPS
    root.centeredToGPS = true
    internal.stakeoutTarget = featurepair
    state = "stakeout"
  }

  function measure() {
    state = "measure"
  }

  function toggleStreaming() {
    // start/stop the streaming mode
    if ( recordingToolsLoader.active ) {
      recordingToolsLoader.item.toggleStreaming()
    }
  }

  function stopStakeout() {
    state = "view"

    // go back to state before starting stakeout
    root.highlightPair( internal.stakeoutTarget )
    mapCanvas.mapSettings.extent = internal.extentBeforeStakeout
    root.centeredToGPS = internal.centeredToGPSBeforeStakeout
  }

  function finishMeasure() {
    state = "view"
  }

  function centerToPair( pair ) {
    __inputUtils.setExtentToFeature( pair, mapCanvas.mapSettings )
  }

  function jumpToHighlighted( mapOffset ) {
    if ( identifyHighlight.geometry === null )
      return

    if ( !__inputUtils.canExtentContainGeometry( identifyHighlight.geometry, mapCanvas.mapSettings ) ){
      return
    }

    let screenPt = __inputUtils.geometryCenterToScreenCoordinates( identifyHighlight.geometry, mapCanvas.mapSettings )
    screenPt.y += mapOffset / 2
    mapCanvas.jumpTo( screenPt )
  }

  function highlightPair( pair ) {
    let geometry = __inputUtils.extractGeometry( pair )
    identifyHighlight.geometry = __inputUtils.transformGeometryToMapWithLayer( geometry, pair.layer, mapCanvas.mapSettings )
  }

  function hideHighlight() {
    identifyHighlight.geometry = null
    updatePosition()
  }

  function centerToPosition( animate = false ) {
    if ( __positionKit.hasPosition ) {
      if ( animate )
      {
        let screenPt = mapCanvas.mapSettings.coordinateToScreen( mapPositionSource.mapPosition )
        mapCanvas.jumpTo( screenPt )
      }
      else
      {
        mapCanvas.mapSettings.setCenter( mapPositionSource.mapPosition )
      }
    }
    else {
      __notificationModel.addWarning( qsTr( "GPS currently unavailable." ) )
    }
  }

  function isPositionOutOfExtent() {
    // allow cursor movement within the central 20% of the map canvas
    return ( ( mapPositionSource.screenPosition.x < mapCanvas.width * 0.4 ) ||
            ( mapPositionSource.screenPosition.y < mapCanvas.height * 0.4 ) ||
            ( mapPositionSource.screenPosition.x > mapCanvas.width * 0.6 ) ||
            ( mapPositionSource.screenPosition.y > mapCanvas.height * 0.6 )
            )
  }

  function updatePosition() {
    if ( ! root.centeredToGPS )
      return

    switch ( root.state ) {
      case "record":
      case "recordInLayer":
      case "edit":
      case "split": {
        root.centerToPosition( false )
        break
      }

      case "view": {
        // While a feature is highlighted we want to keep it visible in the map extent
        // so in that case we skip centering to position
        if ( identifyHighlight.geometry !== null )
        {
          break
        }

        if ( root.isPositionOutOfExtent() )
        {
          root.centerToPosition( true )
        }
        break
      }

      case "stakeout": {
        mapCanvas.mapSettings.extent = __inputUtils.stakeoutPathExtent( mapPositionSource,
                                                                        internal.stakeoutTarget,
                                                                        mapCanvas.mapSettings,
                                                                        mapExtentOffset )
        break
      }

      case "inactive": {
        break
      }
    }
  }

  function refreshMap() {
    // Request map repaint
    mapCanvas.refresh()
  }

  function clear() {
    // clear all previous references to old project (if we don't clear references to the previous project,
    // highlights may end up with dangling pointers to map layers and cause crashes)

    identifyHighlight.geometry = null
  }

  function setTracking( shouldTrack ) {
    if ( shouldTrack ) {
      if ( root.trackingManager ) {
        root.trackingManager.tryAgain()
      }
      else {
        tracking.active = true
      }
    }
    else {
      trackingManager?.commitTrackedPath()
      tracking.active = false
    }
  }

  function showInfoTextMessage( message ) {
    hideInfoTextMessage()

    mapBlurInfoBoxHorizontal.text = message
    mapBlurInfoBoxVertical.text = message

    if ( mapBlurInfoBoxHorizontalGroup.useHorizontalInfoBox ) {
      mapBlurInfoBoxHorizontal.show()
    }
    else {
      mapBlurInfoBoxVertical.show()
    }
  }

  function hideInfoTextMessage() {
    mapBlurInfoBoxVertical.hide()
    mapBlurInfoBoxHorizontal.hide()
  }
}
