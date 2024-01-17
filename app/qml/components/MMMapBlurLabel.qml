/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick
import Qt5Compat.GraphicalEffects
import "."

Item {
  id: control

  property alias sourceItem: effect.sourceItem
  property alias text: text.text

  Rectangle {
    anchors.fill: fastBlur
    color: __style.forestColor
    opacity: 0.8
    radius: 21
  }

  FastBlur {
    id: fastBlur

    x: 20 * __dp
    width: parent.width - 40 * __dp
    height: 42 * __dp

    radius: 32
    opacity: 0.8

    source: ShaderEffectSource {
      id: effect

      sourceRect: Qt.rect(fastBlur.x, control.y, fastBlur.width, fastBlur.height)
    }

    Text {
      id: text

      height: parent.height
      width: parent.width - 40 * __dp
      anchors.verticalCenter: parent.verticalCenter
      anchors.horizontalCenter: parent.horizontalCenter

      color: __style.forestColor
      font: __style.t3
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
      elide: Text.ElideRight
    }
  }
}
