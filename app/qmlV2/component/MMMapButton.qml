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
import Qt5Compat.GraphicalEffects
import "../Style.js" as Style
import "."

Item {
  id: control

  width: 50
  height: 50

  signal clicked

  Rectangle {
    width: parent.width
    height: parent.height
    radius: control.height / 2
    color: Style.white

    layer.enabled: true
    layer.effect: MMShadow {}

    MMIcon {
      id: icon

      anchors.centerIn: parent
      source: Style.arrowLinkRightIcon
      color: Style.forest
    }

    MouseArea {
      anchors.fill: parent
      onClicked: control.clicked()
    }
  }
}
