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

import "../../inputs" as MMInputs
import "../../components" as MMComponents

//
// Special type of drawer/page -> looks like page, but it is actually Drawer
//  - the reason for such a madness is that we need to open fullscreen page from within form editors.
//    Popups (like Drawer) can be easily opened on full screen even from components that are not
//    fullscreen. For page we would need to inject some StackView.
//

Drawer {
  id: root

  property alias pageHeader: page.pageHeader
  property alias button: primarybtn
  property alias list: listView

  signal searchTextChanged( string searchText )
  signal featureClicked( var featurePair )
  signal buttonClicked()

  width: ApplicationWindow.window.width
  height: ApplicationWindow.window.height

  edge: Qt.BottomEdge
  padding: 0

  contentItem: MMComponents.MMPage {
    id: page

    width: parent.width
    height: parent.height

    pageBottomMarginPolicy: MMComponents.MMPage.PaintBehindSystemBar

    onBackClicked: root.close()

    pageContent: Item {
      width: parent.width
      height: parent.height

      Column {
        id: contentColumn

        width: parent.width
        height: parent.height

        spacing: 0

        MMComponents.MMListSpacer { height: __style.spacing20 }

        MMInputs.MMSearchInput {
          id: searchBar

          width: parent.width

          placeholderText: qsTr("Search for features...")

          onSearchTextChanged: root.searchTextChanged( searchBar.searchText )
        }

        MMComponents.MMListSpacer { height: __style.spacing20 }

        MMComponents.MMListView {
          id: listView

          width: parent.width
          height: parent.height - 2 * __style.spacing20 - searchBar.height

          clip: true

          delegate: MMComponents.MMListDelegate {
            text: model.FeatureTitle

            secondaryText: model.SearchResult ? ", " + model.SearchResult.replace(/\n/g, ' ') :  ""

            onClicked: root.featureClicked( model.FeaturePair )
          }

          footer: MMComponents.MMListSpacer { height: __style.safeAreaBottom + __style.margin8 + ( primarybtn.visible ? primarybtn.height : 0 ) }
        }
      }

      MMComponents.MMButton {
        id: primarybtn

        width: parent.width
        anchors {
          bottom: parent.bottom
          bottomMargin: __style.margin8 + __style.safeAreaBottom
        }

        text: qsTr( "Add feature" )
        visible: __activeProject.projectRole !== "reader"

        onClicked: root.buttonClicked()
      }
    }
  }
}
