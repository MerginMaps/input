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
import "../Style.js" as Style

Switch {
  id: control

  property string textOn
  property string textOff

  contentItem: Text {
    text: (control.textOn.length > 0 && control.textOff.length > 0) ? (control.checked ? control.textOn : control.textOff) : control.text
    font: Qt.font(Style.p5)
    color: control.enabled ? Style.forest : Style.mediumGreen
    verticalAlignment: Text.AlignVCenter
    leftPadding: control.indicator.width + control.spacing
  }

  indicator: Rectangle {
    implicitWidth: 48
    implicitHeight: 28
    x: control.leftPadding
    y: parent.height / 2 - height / 2
    radius: implicitHeight / 2
    color: control.checked ? Style.grass : Style.white

    Rectangle {
      x: control.checked ? parent.width - width - radius/2 : radius/2
      width: 20
      height: width
      radius: width / 2
      color: control.down ? Style.mediumGreen : Style.white
      border.color: control.enabled ? Style.forest : Style.mediumGreen
      border.width: 6
      anchors.verticalCenter: parent.verticalCenter
    }
  }
}
