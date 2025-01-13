/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import mm 1.0 as MM

import "../components" as MMComponents

// Content of the preview drawer defined in MMFormController.qml

Item {
  id: root

  property bool layerIsReadOnly: false
  property MM.AttributePreviewController controller

  // binding to ApplicationWindow.window?.xyz can not be in QtObject, it will not get recalculated on change!
  property bool windowHasEnoughHeightToShowContent: ApplicationWindow.window?.height > __style.heightBreakpointXS

  signal contentClicked()
  signal editClicked()
  signal stakeoutClicked( var feature )
  signal closeClicked()

  MouseArea {
    anchors.fill: parent
    onClicked: function( mouse ) {
      mouse.accepted = true
      root.contentClicked()
    }
  }

  implicitHeight: contentLayout.height + contentLayout.topPadding

  Item {
    id: contentLayout

    property real minPaddingAroundContent: 2 * __style.pageMargins + __style.safeAreaLeft + __style.safeAreaRight
    property real minRightPadding: __style.pageMargins + __style.safeAreaRight
    property real topPadding: __style.pageMargins
    property real rightMaxPagePadding: {
      if ( parent.width > __style.maxPageWidth + minPaddingAroundContent ) {
        return ( parent.width - minPaddingAroundContent - __style.maxPageWidth ) / 2
      }
      else {
        return 0
      }
    }

    property real leftPadding: {
      if ( parent.width > __style.maxPageWidth + minPaddingAroundContent ) {
        return ( parent.width - minPaddingAroundContent - __style.maxPageWidth ) / 2  + __style.pageMargins + __style.safeAreaLeft
      }
      else {
        return __style.pageMargins + __style.safeAreaLeft
      }
    }

    width: parent.width - leftPadding - minRightPadding
    height: contentRoot.implicitHeight
    x: leftPadding
    y: topPadding

    Column {
      id: contentRoot

      width: parent.width
      spacing: 0

      // header
      RowLayout {
        width: parent.width
        spacing: 0

        MMComponents.MMText {
          id: featureTitleText

          Layout.fillWidth: true
          Layout.maximumWidth: __style.maxPageWidth
          Layout.preferredHeight: implicitHeight

          text: root.controller.title

          font: __style.t1
          color: __style.forestColor

          wrapMode: Text.Wrap
          maximumLineCount: 2
        }

        Item {
          Layout.fillWidth: true
          Layout.minimumWidth: __style.margin16
          height: 1
        }

        MMComponents.MMRoundButton {
          id: closeBtn

          iconSource: __style.closeIcon
          bgndColor: __style.lightGreenColor

          onClicked: root.closeClicked()
        }
      }

      // photo
      MMComponents.MMListSpacer { height: __style.margin20; visible: internal.showPhoto }

      Item {
        height: visible ? 160 * __dp : 0
        width: parent.width - contentLayout.rightMaxPagePadding

        visible: internal.showPhoto

        MMComponents.MMPhoto {
          width: parent.width
          height: parent.height

          visible: internal.showPhoto

          photoUrl: root.controller.photo

          fillMode: Image.PreserveAspectCrop

          onClicked: root.contentClicked()
        }
      }

      // buttons
      MMComponents.MMListSpacer { height: __style.margin20; visible: internal.showButtons }

      Item {
        width: parent.width - contentLayout.rightMaxPagePadding
        height: childrenRect.height

        visible: internal.showButtons

        ScrollView {
          width: parent.width
          height: scrollRow.height

          ScrollBar.vertical.policy: ScrollBar.AlwaysOff
          ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

          contentHeight: availableHeight
          contentWidth: scrollRow.width > parent.width ? scrollRow.width : availableWidth

          // Scrollview does not propagate clicks to items beneath
          MouseArea {
            anchors.fill: parent
            onClicked: function( mouse ) {
              mouse.accepted = true
              root.contentClicked()
            }
          }

          Row {
            id: scrollRow

            height: stakeOutButton.height
            spacing: 12 * __dp

            MMComponents.MMButton {
              id: editButton

              text: qsTr( "Edit" )
              iconSourceLeft: __style.editIcon

              visible: internal.showEditButton

              onClicked: root.editClicked()
            }

            MMComponents.MMButton {
              id: stakeOutButton

              text: qsTr( "Stake out" )
              iconSourceLeft: __style.gpsAntennaHeightIcon
              type: MMComponents.MMButton.Secondary

              visible: internal.showStakeoutButton

              onClicked: root.stakeoutClicked( controller.featureLayerPair )
            }
          }
        }
      }

      // fields
      MMComponents.MMListSpacer { height: __style.margin20; visible: internal.showFields }

      Item {
        width: parent.width - contentLayout.rightMaxPagePadding
        height: childrenRect.height

        visible: internal.showFields

        MMComponents.MMListView {
          width: parent.width
          height: contentHeight

          spacing: __style.margin8
          interactive: false

          model: root.controller.fieldModel

          delegate: Item {
            width: ListView.view.width
            height: childrenRect.height

            Column {
              width: parent.width
              spacing: 0

              MMComponents.MMText {
                width: parent.width

                text: model.Name
                font: __style.p6
                color: __style.nightColor

                elide: Text.ElideRight
                wrapMode: Text.NoWrap
              }

              MMComponents.MMText {
                width: parent.width

                text: model.Value
                font: __style.t3
                color: __style.nightColor

                elide: Text.ElideRight
                wrapMode: Text.NoWrap

                onLinkActivated: function ( link ) {
                  Qt.openUrlExternally( link )
                }
              }

              MMComponents.MMListSpacer { height: __style.margin8 }

              MMComponents.MMLine { visible: model.index !== root.controller.fieldModel.rowCount() - 1 }
            }
          }
        }
      }

      // HTML
      MMComponents.MMListSpacer { height: __style.margin20; visible: internal.showHTML }

      Item {
        width: parent.width - contentLayout.rightMaxPagePadding
        height: childrenRect.height

        visible: internal.showHTML

        Text {
          width: parent.width
          height: 200 * __dp

          clip: true
          textFormat: Text.RichText

          wrapMode: Text.Wrap

          elide: Text.ElideRight
          text: root.controller.html

          onLinkActivated: function ( link ) {
            Qt.openUrlExternally( link )
          }
        }
      }

      MMComponents.MMListFooterSpacer { height: __style.safeAreaBottom + __style.margin20 }
    }
  }

  QtObject {
    id: internal

    property bool isPhotoType: root.controller.type === MM.AttributePreviewController.Photo
    property bool isFieldsType: root.controller.type === MM.AttributePreviewController.Fields
    property bool isHTMLType: root.controller.type === MM.AttributePreviewController.HTML
    property bool isEmptyType: root.controller.type === MM.AttributePreviewController.Empty

    property bool showEditButton: !root.layerIsReadOnly && __activeProject.projectRole !== "reader"
    property bool showStakeoutButton: __inputUtils.isPointLayerFeature( controller.featureLayerPair )
    property bool showButtons: showEditButton || showStakeoutButton

    property bool showPhoto: isPhotoType && windowHasEnoughHeightToShowContent
    property bool showFields: isFieldsType && windowHasEnoughHeightToShowContent
    property bool showHTML: isHTMLType && windowHasEnoughHeightToShowContent
  }
}
