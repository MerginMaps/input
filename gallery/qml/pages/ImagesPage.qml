/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

import "../../app/qml/components"
import "../"

ScrollView {
  Column {
    padding: 20
    spacing: 5

    property int rectSize: 10

    GroupBox {
      title: "Images"
      background: Rectangle {
        color: __style.lightGreenColor
        border.color: "gray"
      }
      label: Label {
        color: "black"
        text: parent.title
        padding: 5
      }

      contentData: Column {
        spacing: 10
        Column { Image { source: __style.mmLogoImage } Text { text: "mmLogoImage" } }
        Column { Image { source: __style.lutraLogoImage; sourceSize.width:120; fillMode: Image.PreserveAspectFit } Text { text: "lutraLogoImage" } }
        Column { Image { source: __style.mmSymbolImage } Text { text: "mmSymbolImage" } }

        Column { Image { source: __style.directionImage } Text { text: "directionImage" } }
        Column { Image { source: __style.trackingDirectionImage } Text { text: "trackingDirectionImage" } }
        Column { Image { source: __style.mapPinImage } Text { text: "mapPinImage" } }
        Column { Image { source: __style.warnLogoImage } Text { text: "warnLogoImage" } }
        Column { Image { source: __style.positionTrackingStartImage } Text { text: "positionTrackingStartImage" } }
        Column { Image { source: __style.positionTrackingRunningImage } Text { text: "positionTrackingRunningImage" } }
        Column { Image { source: __style.noMapThemesImage } Text { text: "noMapThemesImage" } }
      }
    }

    GroupBox {
      title: "Layers (QGIS)"
      background: Rectangle {
        color: __style.lightGreenColor
        border.color: "gray"
      }
      label: Label {
        color: "black"
        text: parent.title
        padding: 5
      }

      contentData: Column {
        spacing: 10
        Column { Image { sourceSize.width:32; source: __style.actionFolderImage } Text { text: "actionFolderImage" } }
        Column { Image { sourceSize.width:32; source: __style.annotationLayerImage } Text { text: "annotationLayerImage" } }
        Column { Image { sourceSize.width:32; source: __style.geometryCollectionLayerImage } Text { text: "geometryCollectionLayerImage" } }
        Column { Image { sourceSize.width:32; source: __style.groupImage } Text { text: "groupImage" } }
        Column { Image { sourceSize.width:32; source: __style.layerImage } Text { text: "layerImage" } }
        Column { Image { sourceSize.width:32; source: __style.lineLayerImage } Text { text: "lineLayerImage" } }
        Column { Image { sourceSize.width:32; source: __style.meshLayerImage } Text { text: "meshLayerImage" } }
        Column { Image { sourceSize.width:32; source: __style.pointCloudLayerImage } Text { text: "pointCloudLayerImage" } }
        Column { Image { sourceSize.width:32; source: __style.polygonLayerImage } Text { text: "polygonLayerImage" } }
        Column { Image { sourceSize.width:32; source: __style.rasterLayerImage } Text { text: "rasterLayerImage" } }
        Column { Image { sourceSize.width:32; source: __style.tableLayerImage } Text { text: "tableLayerImage" } }
        Column { Image { sourceSize.width:32; source: __style.vectorTileLayerImage } Text { text: "vectorTileLayerImage" } }
        Column { Image { sourceSize.width:32; source: __style.indicatorBadLayerImage } Text { text: "indicatorBadLayerImage" } }
      }
    }
  }
}
