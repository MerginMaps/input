/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
import QtQuick 2.0
import QtQuick.Controls 2.12
import QgsQuick 0.1 as QgsQuick
import lc 1.0
import "components"

Item {
  id: root
  signal backButtonClicked
  signal layerClicked(var layerId)

  Page {
    id: layersListPage
    anchors.fill: parent

    LayerList {
      borderWidth: 1
      cellHeight: InputStyle.rowHeight
      cellWidth: width
      highlightingAllowed: false
      implicitHeight: layersListPage.height - layersPageHeader.height
      model: __browseDataLayersModel
      noLayersText: qsTr("No layers have been configured to allow browsing their features. See %1how to modify your project%2.").arg("<a href='" + __inputHelp.howToEnableBrowsingDataLink + "'>").arg("</a>")
      width: parent.width

      onListItemClicked: {
        layerClicked(layerId);
      }
    }

    header: PanelHeader {
      id: layersPageHeader
      color: InputStyle.clrPanelMain
      height: InputStyle.rowHeightHeader
      rowHeight: InputStyle.rowHeightHeader
      titleText: qsTr("Layers")
      width: parent.width
      withBackButton: true

      onBack: root.backButtonClicked()
    }
  }
}
