/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick
import "../Style.js" as Style
import "."

Rectangle {
  id: control

  required property real position // [0 - 1]

  width: parent.width
  height: 12 * __dp
  color: Style.lightGreen
  radius: height / 2

  Rectangle {
    width: parent.width * control.position
    height: parent.height
    color: Style.grass
    radius: height / 2
  }
}
