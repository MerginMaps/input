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
      title: "Images (for dialogs 160 x 140)"
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
        Column { Image { source: __style.positionTrackingStartImage } Text { text: "positionTrackingStartImage" } }
        Column { Image { source: __style.positionTrackingRunningImage } Text { text: "positionTrackingRunningImage" } }
        Column { Image { source: __style.noMapThemesImage } Text { text: "noMapThemesImage" } }
        Column { Image { source: __style.syncImage } Text { text: "syncImage" } }
        Column { Image { source: __style.externalGpsGreenImage } Text { text: "externalGpsGreenImage" } }
        Column { Image { source: __style.externalGpsRedImage } Text { text: "externalGpsRedImage" } }
        Column { Image { source: __style.reachedDataLimitImage } Text { text: "reachedDataLimitImage" } }
        Column { Image { source: __style.positiveMMSymbolImage } Text { text: "positiveMMSymbolImage" } }
        Column { Image { source: __style.negativeMMSymbolImage } Text { text: "negativeMMSymbolImage" } }
        Column { Image { source: __style.closeAccountImage } Text { text: "closeAccountImage" } }
      }
    }

    GroupBox {
      title: "Images (for tips/bubbles 50 x 50)"
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
        Column { Image { source: __style.warnLogoImage } Text { text: "warnLogoImage" } }
        Column { Image { source: __style.attentionImage } Text { text: "attentionImage" } }
        Column { Image { source: __style.bubbleImage } Text { text: "bubbleImage" } }
      }
    }

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
        Column { Image { sourceSize.width:32; source: __style.pointLayerImage } Text { text: "pointLayerImage" } }
        Column { Image { sourceSize.width:32; source: __style.lineLayerImage } Text { text: "lineLayerImage" } }
        Column { Image { sourceSize.width:32; source: __style.polygonLayerImage } Text { text: "polygonLayerImage" } }
        Column { Image { sourceSize.width:32; source: __style.rasterLayerImage } Text { text: "rasterLayerImage" } }
        Column { Image { sourceSize.width:32; source: __style.tableLayerImage } Text { text: "tableLayerImage" } }
      }
    }
  }
}
