/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick
import QtQuick.Shapes

import mm 1.0 as MM

import "../components"
import "./components"
import "../gps"
import "../dialogs"

Item {
  id: root

  required property MMMapCanvas map
  required property MMPositionMarker positionMarkerComponent

  property var mapTool: mapTool

  signal finishMeasurement()

  Component.onCompleted: map.mapSettings.extentChanged.connect( onScreenPositionChanged )
  Component.onDestruction: map.mapSettings.extentChanged.disconnect( onScreenPositionChanged )

  MM.MeasurementMapTool {
    id: mapTool

    mapSettings: root.map.mapSettings
  }

  MM.GuidelineController {
    id: guidelineController

    mapSettings: root.map.mapSettings
    crosshairPosition: crosshair.screenPoint
    realGeometry: mapTool.recordedGeometry
  }

  MMHighlight {
    id: guideline

    height: root.map.height
    width: root.map.width

    markerColor: __style.deepOceanColor
    lineColor: __style.deepOceanColor
    lineStrokeStyle: ShapePath.DashLine
    lineWidth: MMHighlight.LineWidths.Narrow

    mapSettings: root.map.mapSettings
    geometry: guidelineController.guidelineGeometry
  }

  MMHighlight {
    id: highlight

    height: map.height
    width: map.width

    markerColor: __style.deepOceanColor
    lineColor: __style.deepOceanColor
    lineWidth: MMHighlight.LineWidths.Narrow

    mapSettings: root.map.mapSettings
    geometry: mapTool.recordedGeometry
  }

  MMHighlight {
    id: existingVerticesHighlight

    height: root.map.height
    width: root.map.width

    mapSettings: root.map.mapSettings
    geometry: mapTool.existingVertices

    markerType: MMHighlight.MarkerTypes.Circle
    markerSize: MMHighlight.MarkerSizes.Bigger
  }

  MMMeasureCrosshair {
    id: crosshair

    anchors.fill: parent
    qgsProject: __activeProject.qgsProject
    mapSettings: root.map.mapSettings

    text: __inputUtils.formatDistanceInProjectUnit( mapTool.lengthWithGuideline )
    canCloseShape: mapTool.canCloseShape

    onCloseShapeClicked: closeShape()
  }

  function addPoint()
  {
    mapTool.addPoint( crosshair.screenPoint )
  }

  function onScreenPositionChanged() {
    mapTool.updateDistance( crosshair.screenPoint );
  }

  function closeShape()
  {
    guidelineController.allowed = false
    crosshair.visible = false
    mapTool.closeShape()
  }

  function repeatMeasure()
  {
    guidelineController.allowed = true
    crosshair.visible = true
    mapTool.reset()
  }
}
