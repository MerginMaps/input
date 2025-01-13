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
import QtQuick.Layouts

import mm 1.0 as MM

import "../inputs"
import "../components" as MMComponents

MMComponents.MMPage {
  id: root

  property var selectedLayer: null
  property bool hasToolbar: false
  property bool layerIsReadOnly: selectedLayer?.readOnly ?? false

  signal featureClicked( var featurePair )
  signal addFeatureClicked( var toLayer )

  pageHeader.title: root.selectedLayer ? root.selectedLayer.name + " (" + featuresModel.layerFeaturesCount + ")": ""
  pageBottomMargin: 0

  pageContent: Item {
    width: parent.width
    height: parent.height

    MMSearchInput {
      id: searchBar

      anchors.top: parent.top
      anchors.topMargin: __style.spacing20

      width: parent.width

      delayedSearch: true
      onSearchTextChanged: featuresModel.searchExpression = searchBar.text
    }

    MMComponents.MMListView {
      id: listView

      width: parent.width

      anchors {
        top: searchBar.bottom
        bottom: parent.bottom
        topMargin: __style.spacing20
      }

      model: MM.FeaturesModel {
        id: featuresModel

        useAttributeTableSortOrder: true
        layer: root.selectedLayer
      }

      clip: true

      delegate: MMComponents.MMListDelegate {
        text: model.display?.toString()?.replace(/\n/g, ' ') ?? ''
        secondaryText: model.Description + ( model.SearchResult ? ", " + model.SearchResult.replace(/\n/g, ' ') : "" )

        onClicked: root.featureClicked( model.FeaturePair )
      }

      footer: MMComponents.MMListSpacer {
        height: __style.margin20 + ( root.hasToolbar ? 0 : __style.safeAreaBottom ) + ( addButton.visible ? addButton.height : 0 )
      }
    }

    MMComponents.MMBusyIndicator {
      anchors.centerIn: parent
      running: featuresModel.fetchingResults
    }

    MMComponents.MMButton {
      id: addButton

      width: parent.width
      anchors.bottom: parent.bottom
      anchors.bottomMargin: root.hasToolbar ? __style.margin20 : ( __style.safeAreaBottom + __style.margin8 )

      visible: __inputUtils.isNoGeometryLayer( root.selectedLayer ) && !root.layerIsReadOnly && __activeProject.projectRole !== "reader"

      text: qsTr("Add feature")

      onClicked: root.addFeatureClicked( root.selectedLayer )
    }
  }

  Component.onCompleted: {
    featuresModel.reloadFeatures()
  }
}
