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

import "../components"
import "../inputs"

Page {
  id: root

  property var model: null
  property var parentNodeIndex: null
  property string pageTitle: qsTr("Search layers")

  signal close()
  signal nodeClicked( var node, string nodeType, string nodeName )
  signal nodeVisibilityClicked( var node )
  signal searchTextChanged( string searchText )

  Keys.onReleased: function( event ) {
    if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
      event.accepted = true
      root.close()
    }
  }

  header: MMPageHeader {
    width: parent.width
    title: root.pageTitle
    color: __style.lightGreenColor

    onBackClicked: root.close()
  }

  Rectangle {
    anchors.fill: parent
    color: __style.lightGreenColor
  }

  MMSearchInput {
    id: searchbox

    anchors {
      left: parent.left
      leftMargin: __style.pageMargins
      right: parent.right
      rightMargin: __style.pageMargins
      top: parent.top
      topMargin: __style.margin20
    }

    onSearchTextChanged: function( searchText ) {
      root.searchTextChanged( searchText )
    }
  }

  MMLayersList {
    id: layers

    anchors {
      top: searchbox.bottom
      topMargin: __style.margin20
      left: parent.left
      leftMargin: __style.pageMargins
      right: parent.right
      rightMargin: __style.pageMargins
      bottom: parent.bottom
    }

    clip: true

    model: root.model
    parentNodeIndex: root.parentNodeIndex

    showNodePath: true
    imageProviderPath: "image://LayerTreeFlatModelPixmapProvider/"

    onNodeClicked: function( node, nodeType, nodeName ) {
      root.nodeClicked( node, nodeType, nodeName )
    }

    onNodeVisibilityClicked: function( node ) {
      root.nodeVisibilityClicked( node )
    }
  }
}
