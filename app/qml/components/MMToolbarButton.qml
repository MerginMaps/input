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

Item {
  id: control

  signal clicked

  required property var iconSource
  required property string text

  property alias buttonIcon: icon

  property var type: MMToolbarButton.Button.Normal
  property bool isMenuButton: false

  enum Button { Normal, Emphasized }

  height: isMenuButton ? __style.menuDrawerHeight/2 : __style.toolbarHeight

  Rectangle {
    width: parent.width - 10 * __dp
    height: parent.height - 10 * __dp
    anchors.centerIn: parent
    clip: control.type !== MMToolbarButton.Button.Emphasized
    color: __style.transparentColor
    visible: !control.isMenuButton

    Image {
      id: icon

      source: control.iconSource
      anchors.horizontalCenter: parent.horizontalCenter
      anchors.bottom: parent.bottom
      anchors.bottomMargin: 40 * __dp + (control.type === MMToolbarButton.Button.Emphasized ? 14 * __dp : 0)

      Rectangle {
        visible: control.type === MMToolbarButton.Button.Emphasized
        anchors.centerIn: parent
        width: 60 * __dp
        height: width
        radius: width / 2
        color: __style.transparentColor
        border.color: __style.grassColor
        border.width: 16 * __dp

        // handle clicks above the toolbar
        MouseArea {
          anchors.fill: parent
          onClicked: control.clicked()
        }
      }
    }

    Text {
      id: text

      text: control.text
      width: parent.width
      color: __style.whiteColor
      font: __style.t4
      anchors.horizontalCenter: parent.horizontalCenter
      anchors.bottom: parent.bottom
      anchors.bottomMargin: 20 * __dp
      horizontalAlignment: Text.AlignHCenter
      elide: Text.ElideMiddle
    }

    MouseArea {
      anchors.fill: parent
      onClicked: control.clicked()
    }
  }

  // Menu button
  MMToolbarMenuButton {
    width: control.width
    height: __style.menuDrawerHeight
    visible: control.isMenuButton
    iconSource: control.iconSource
    text: control.text
    onClicked: control.clicked()
  }
}
